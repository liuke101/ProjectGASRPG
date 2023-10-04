// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtPackageReader.h"
#include "ExtContentBrowser.h"
#include "ExtAssetData.h"

#include "HAL/FileManager.h"
#include "UObject/Class.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetData.h"
#include "Logging/MessageLog.h"
//#include "AssetRegistry.h"

FExtPackageReader::FExtPackageReader()
{
	Loader = nullptr;
	PackageFileSize = 0;
	this->SetIsLoading(true);
	this->SetIsPersistent(true);
}

FExtPackageReader::~FExtPackageReader()
{
	if (Loader)
	{
		delete Loader;
	}
}

bool FExtPackageReader::OpenPackageFile(const FString& InPackageFilename, EOpenPackageResult* OutErrorCode)
{
	PackageFilename = InPackageFilename;
	Loader = IFileManager::Get().CreateFileReader(*PackageFilename);
	return OpenPackageFile(OutErrorCode);
}

bool FExtPackageReader::OpenPackageFile(FArchive* InLoader, EOpenPackageResult* OutErrorCode)
{
	Loader = InLoader;
	PackageFilename = Loader->GetArchiveName();
	return OpenPackageFile(OutErrorCode);
}

bool FExtPackageReader::OpenPackageFile(EOpenPackageResult* OutErrorCode)
{
	auto SetPackageErrorCode = [&](const EOpenPackageResult InErrorCode)
	{
		if (OutErrorCode)
		{
			*OutErrorCode = InErrorCode;
		}
	};

	if( Loader == nullptr )
	{
		// Couldn't open the file
		SetPackageErrorCode(EOpenPackageResult::NoLoader);
		return false;
	}

	// Read package file summary from the file
	*Loader << PackageFileSummary;

	// Validate the summary.

	// Make sure this is indeed a package
	if( PackageFileSummary.Tag != PACKAGE_FILE_TAG || Loader->IsError() )
	{
		// Unrecognized or malformed package file
		SetPackageErrorCode(EOpenPackageResult::MalformedTag);
		return false;
	}

	// Don't read packages without header
	if (PackageFileSummary.TotalHeaderSize == 0)
	{
		SetPackageErrorCode(EOpenPackageResult::FailedToLoad);
		return false;
	}

	// Don't read packages that are too old
	if( PackageFileSummary.IsFileVersionTooOld())
	{
		SetPackageErrorCode(EOpenPackageResult::VersionTooOld);
		return false;
	}

	// Don't read packages that were saved with an package version newer than the current one.
	if(PackageFileSummary.IsFileVersionTooNew() || PackageFileSummary.GetFileVersionLicenseeUE() > GPackageFileLicenseeUEVersion)
	{
		SetPackageErrorCode(EOpenPackageResult::VersionTooNew);
		return false;
	}

	// Check serialized custom versions against latest custom versions.
	TArray<FCustomVersionDifference> Diffs = FCurrentCustomVersions::Compare(PackageFileSummary.GetCustomVersionContainer().GetAllVersions(), *PackageFilename);
	for (FCustomVersionDifference Diff : Diffs)
	{
		if (Diff.Type == ECustomVersionDifference::Missing)
		{
			SetPackageErrorCode(EOpenPackageResult::CustomVersionMissing);
			return false;
		}
		else if (Diff.Type == ECustomVersionDifference::Newer)
		{
			SetPackageErrorCode(EOpenPackageResult::VersionTooNew);
			return false;
		}
	}

	//make sure the filereader gets the correct version number (it defaults to latest version)
	SetUEVer(PackageFileSummary.GetFileVersionUE());
	SetLicenseeUEVer(PackageFileSummary.GetFileVersionLicenseeUE());
	SetEngineVer(PackageFileSummary.SavedByEngineVersion);
	Loader->SetUEVer(PackageFileSummary.GetFileVersionUE());
	Loader->SetLicenseeUEVer(PackageFileSummary.GetFileVersionLicenseeUE());
	Loader->SetEngineVer(PackageFileSummary.SavedByEngineVersion);

	SetByteSwapping(Loader->ForceByteSwapping());

	const FCustomVersionContainer& PackageFileSummaryVersions = PackageFileSummary.GetCustomVersionContainer();
	SetCustomVersions(PackageFileSummaryVersions);
	Loader->SetCustomVersions(PackageFileSummaryVersions);

	PackageFileSize = Loader->TotalSize();

	SetPackageErrorCode(EOpenPackageResult::Success);
	return true;
}

bool FExtPackageReader::StartSerializeSection(int64 Offset)
{
	check(Loader);
	if (Offset <= 0 || Offset > PackageFileSize)
	{
		return false;
	}
	ClearError();
	Loader->ClearError();
	Seek(Offset);
	return !IsError();
}


namespace UA::AssetRegistry
{
	class FNameMapAwareArchive : public FArchiveProxy
	{
		TArray<FNameEntryId>	NameMap;

	public:

		FNameMapAwareArchive(FArchive& Inner)
			: FArchiveProxy(Inner)
		{}

		FORCEINLINE virtual FArchive& operator<<(FName& Name) override
		{
			FArchive& Ar = *this;
			int32 NameIndex;
			Ar << NameIndex;
			int32 Number = 0;
			Ar << Number;

			if (NameMap.IsValidIndex(NameIndex))
			{
				// if the name wasn't loaded (because it wasn't valid in this context)
				FNameEntryId MappedName = NameMap[NameIndex];

				// simply create the name from the NameMap's name and the serialized instance number
				Name = FName::CreateFromDisplayId(MappedName, Number);
			}
			else
			{
				Name = FName();
				SetCriticalError();
			}

			return *this;
		}

		void SerializeNameMap(const FPackageFileSummary& PackageFileSummary)
		{
			Seek(PackageFileSummary.NameOffset);
			NameMap.Reserve(PackageFileSummary.NameCount);
			FNameEntrySerialized NameEntry(ENAME_LinkerConstructor);
			for (int32 Idx = NameMap.Num(); Idx < PackageFileSummary.NameCount; ++Idx)
			{
				*this << NameEntry;
				NameMap.Emplace(FName(NameEntry).GetDisplayIndex());
			}
		}
	};
	FString ReconstructFullClassPath(FArchive& BinaryArchive, const FString& PackageName, const FPackageFileSummary& PackageFileSummary, const FString& AssetClassName,
		const TArray<FObjectImport>* InImports = nullptr, const TArray<FObjectExport>* InExports = nullptr)
	{
		FName ClassFName(*AssetClassName);
		FLinkerTables LinkerTables;
		if (!InImports || !InExports)
		{
			FNameMapAwareArchive NameMapArchive(BinaryArchive);
			NameMapArchive.SerializeNameMap(PackageFileSummary);

			// Load the linker tables
			if (!InImports)
			{
				BinaryArchive.Seek(PackageFileSummary.ImportOffset);
				for (int32 ImportMapIndex = 0; ImportMapIndex < PackageFileSummary.ImportCount; ++ImportMapIndex)
				{
					NameMapArchive << LinkerTables.ImportMap.Emplace_GetRef();
				}
			}
			if (!InExports)
			{
				BinaryArchive.Seek(PackageFileSummary.ExportOffset);
				for (int32 ExportMapIndex = 0; ExportMapIndex < PackageFileSummary.ExportCount; ++ExportMapIndex)
				{
					NameMapArchive << LinkerTables.ExportMap.Emplace_GetRef();
				}
			}
		}
		if (InImports)
		{
			LinkerTables.ImportMap = *InImports;
		}
		if (InExports)
		{
			LinkerTables.ExportMap = *InExports;
		}

		FString ClassPathName;

		// Now look through the exports' classes and find the one matching the asset class
		for (const FObjectExport& Export : LinkerTables.ExportMap)
		{
			if (Export.ClassIndex.IsImport())
			{
				if (LinkerTables.ImportMap[Export.ClassIndex.ToImport()].ObjectName == ClassFName)
				{
					ClassPathName = LinkerTables.GetImportPathName(Export.ClassIndex.ToImport());
					break;
				}
			}
			else if (Export.ClassIndex.IsExport())
			{
				if (LinkerTables.ExportMap[Export.ClassIndex.ToExport()].ObjectName == ClassFName)
				{
					ClassPathName = LinkerTables.GetExportPathName(PackageName, Export.ClassIndex.ToExport());
					break;
				}
			}
		}
		if (ClassPathName.IsEmpty())
		{
			//UE_LOG(LogAssetRegistry, Error, TEXT("Failed to find an import or export matching asset class short name \"%s\"."), *AssetClassName);
			// Just pass through the short class name
			ClassPathName = AssetClassName;
		}


		return ClassPathName;
	}

};


#define ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING(MessageKey, PackageFileName) \
	do \
	{\
		FFormatNamedArguments CorruptPackageWarningArguments; \
		CorruptPackageWarningArguments.Add(TEXT("FileName"), FText::FromString(PackageFileName)); \
		FMessageLog("AssetRegistry").Warning(FText::Format(NSLOCTEXT("AssetRegistry", MessageKey, "Cannot read AssetRegistry Data in {FileName}, skipping it. Error: " MessageKey "."), CorruptPackageWarningArguments)); \
	} while (false)

bool FExtPackageReader::ReadAssetRegistryData (TArray<FAssetData*>& AssetDataList)
{
	if (!StartSerializeSection(PackageFileSummary.AssetRegistryDataOffset))
	{
		return false;
	}

	FString PackageName;
	if (!FPackageName::TryConvertFilenameToLongPackageName(PackageFilename, PackageName))
	{
		// Path was possibly unmounted
		return false;
	}
	FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
	const bool bIsMapPackage = (PackageFileSummary.GetPackageFlags() & PKG_ContainsMap) != 0;

	// ? To avoid large patch sizes, we have frozen cooked package format at the format before VER_UE4_ASSETREGISTRY_DEPENDENCYFLAGS
	bool bPreDependencyFormat = PackageFileSummary.GetFileVersionUE() < VER_UE4_ASSETREGISTRY_DEPENDENCYFLAGS;// || !!(PackageFileSummary.PackageFlags & PKG_FilterEditorOnly);

	int64 DependencyDataOffset;
	if (!bPreDependencyFormat)
	{
		*this << DependencyDataOffset;
	}

	// Load the object count
	int32 ObjectCount = 0;
	*this << ObjectCount;
	const int32 MinBytesPerObject = 1;

	// Check invalid object count
	if (this->IsError() || ObjectCount < 0 || PackageFileSize < this->Tell() + ObjectCount * MinBytesPerObject)
	{
		return false;
	}

	// Worlds that were saved before they were marked public do not have asset data so we will synthesize it here to make sure we see all legacy umaps
	// We will also do this for maps saved after they were marked public but no asset data was saved for some reason. A bug caused this to happen for some maps.
	if (bIsMapPackage)
	{
		const bool bLegacyPackage = PackageFileSummary.GetFileVersionUE() < VER_UE4_PUBLIC_WORLDS;
		const bool bNoMapAsset = (ObjectCount == 0);
		if (bLegacyPackage || bNoMapAsset)
		{
			FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
			AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), FName(*AssetName), FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("World")), FAssetDataTagMap(), PackageFileSummary.ChunkIDs, PackageFileSummary.GetPackageFlags()));
		}
	}

	const int32 MinBytesPerTag = 1;
	// UAsset files usually only have one asset, maps and redirectors have multiple
	for(int32 ObjectIdx = 0; ObjectIdx < ObjectCount; ++ObjectIdx)
	{
		FString ObjectPath;
		FString ObjectClassName;
		int32 TagCount = 0;
		*this << ObjectPath;
		*this << ObjectClassName;
		*this << TagCount;

		// check invalid tag count
		if (this->IsError() || TagCount < 0 || PackageFileSize < this->Tell() + TagCount * MinBytesPerTag)
		{
			return false;
		}

		FAssetDataTagMap TagsAndValues;
		TagsAndValues.Reserve(TagCount);

		for(int32 TagIdx = 0; TagIdx < TagCount; ++TagIdx)
		{
			FString Key;
			FString Value;
			*this << Key;
			*this << Value;

			// check invalid tag
			if (this->IsError())
			{
				return false;
			}

			if (!Key.IsEmpty() && !Value.IsEmpty())
			{
				TagsAndValues.Add(FName(*Key), Value);
			}
		}

		if (ObjectPath.StartsWith(TEXT("/"), ESearchCase::CaseSensitive))
		{
			// This should never happen, it means that package A has an export with an outer of package B
			ECB_LOG(Warning, TEXT("Package %s has invalid export %s, resave source package!"), *PackageName, *ObjectPath);
			continue;
		}

		if (!ensureMsgf(!ObjectPath.Contains(TEXT("."), ESearchCase::CaseSensitive), TEXT("Cannot make FAssetData for sub object %s!"), *ObjectPath))
		{
			continue;
		}

		FString AssetName = ObjectPath;

		// Before world were RF_Public, other non-public assets were added to the asset data table in map packages.
		// Here we simply skip over them
		if ( bIsMapPackage && PackageFileSummary.GetFileVersionUE() < VER_UE4_PUBLIC_WORLDS )
		{
			if ( AssetName != FPackageName::GetLongPackageAssetName(PackageName) )
			{
				continue;
			}
		}
		 
		ObjectClassName = UA::AssetRegistry::ReconstructFullClassPath(*this, PackageName, PackageFileSummary,
			ObjectClassName, &ImportMap, &ExportMap);

		// Create a new FAssetData for this asset and update it with the gathered data
		AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), FName(*AssetName), FTopLevelAssetPath(ObjectClassName), MoveTemp(TagsAndValues), PackageFileSummary.ChunkIDs, PackageFileSummary.GetPackageFlags()));
	}

	return true;
}

bool FExtPackageReader::ReadAssetRegistryData(FExtAssetData& OutAssetData)
{
	if (!StartSerializeSection(PackageFileSummary.AssetRegistryDataOffset))
	{
		return false;
	}

#if ECB_LEGACY
	// Determine the package name and path
	FString PackageName = FPackageName::FilenameToLongPackageName(PackageFilename);
	FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
#endif
	const bool bIsMapPackage = (PackageFileSummary.GetPackageFlags() & PKG_ContainsMap) != 0;

	// Load the object count
	int32 ObjectCount = 0;
	{
		// ? To avoid large patch sizes, we have frozen cooked package format at the format before VER_UE4_ASSETREGISTRY_DEPENDENCYFLAGS
		bool bPreDependencyFormat = PackageFileSummary.GetFileVersionUE() < VER_UE4_ASSETREGISTRY_DEPENDENCYFLAGS;// || !!(PackageFileSummary.PackageFlags & PKG_FilterEditorOnly);

		int64 DependencyDataOffset;
		if (!bPreDependencyFormat)
		{
			*this << DependencyDataOffset;
		}

		// Load the object count
		*this << ObjectCount;
		const int32 MinBytesPerObject = 1;

		// Check invalid object count
		if (this->IsError() || ObjectCount < 0 || PackageFileSize < this->Tell() + ObjectCount * MinBytesPerObject)
		{
			return false;
		}
	}

	OutAssetData.AssetCount = ObjectCount;

#if ECB_TODO // do we need really special treatment of legacy map asset?

	// Worlds that were saved before they were marked public do not have asset data so we will synthesize it here to make sure we see all legacy umaps
	// We will also do this for maps saved after they were marked public but no asset data was saved for some reason. A bug caused this to happen for some maps.
	if (bIsMapPackage)
	{
		const bool bLegacyPackage = PackageFileSummary.GetFileVersionUE4() < VER_UE4_PUBLIC_WORLDS;
		const bool bNoMapAsset = (ObjectCount == 0);
		if (bLegacyPackage || bNoMapAsset)
		{
			FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
			AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), FName(*AssetName), FName(TEXT("World")), FAssetDataTagMap(), PackageFileSummary.ChunkIDs, PackageFileSummary.PackageFlags));
		}
	}
#endif

	const int32 MinBytesPerTag = 1;
	// UAsset files usually only have one asset, maps and redirectors have multiple
	//for (int32 ObjectIdx = 0; ObjectIdx < ObjectCount; ++ObjectIdx)
	if (ObjectCount > 0) // Only care about first
	{
		FString ObjectPath;
		FString ObjectClassName;
		int32 TagCount = 0;
		*this << ObjectPath;
		*this << ObjectClassName;
		*this << TagCount;

		// check invalid tag count
		if (this->IsError() || TagCount < 0 || PackageFileSize < this->Tell() + TagCount * MinBytesPerTag)
		{
			return false;
		}

		ECB_LOG(Display, TEXT("\t\t%s'%s' (%d Tags)"), *ObjectClassName, *ObjectPath, TagCount); // Output : StaticMesh'SM_Test_LOD0' (14 Tags)
		OutAssetData.AssetName = FName(*ObjectPath);
		OutAssetData.AssetClass = FName(*ObjectClassName);

		FAssetDataTagMap TagsAndValues;
		TagsAndValues.Reserve(TagCount);

		for (int32 TagIdx = 0; TagIdx < TagCount; ++TagIdx)
		{
			FString Key;
			FString Value;
			*this << Key;
			*this << Value;

			// check invalid tag
			if (this->IsError())
			{
				return false;
			}

			if (!Key.IsEmpty() && !Value.IsEmpty())
			{
				TagsAndValues.Add(FName(*Key), Value);

				//ECB_LOG(Display, TEXT("\t\t\t\"%s\": \"%s\""), *Key, *Value);
			}
		}
		OutAssetData.TagsAndValues = FAssetDataTagMapSharedView(MoveTemp(TagsAndValues));

#if ECB_TODO

		if (ObjectPath.StartsWith(TEXT("/"), ESearchCase::CaseSensitive))
		{
			// This should never happen, it means that package A has an export with an outer of package B
			ECB_LOG(Warning, TEXT("Package %s has invalid export %s, resave source package!"), *PackageName, *ObjectPath);
			continue;
		}

		if (!ensureMsgf(!ObjectPath.Contains(TEXT("."), ESearchCase::CaseSensitive), TEXT("Cannot make FAssetData for sub object %s!"), *ObjectPath))
		{
			continue;
		}

		FString AssetName = ObjectPath;

		// Before world were RF_Public, other non-public assets were added to the asset data table in map packages.
		// Here we simply skip over them
		if (bIsMapPackage && PackageFileSummary.GetFileVersionUE4() < VER_UE4_PUBLIC_WORLDS)
		{
			if (AssetName != FPackageName::GetLongPackageAssetName(PackageName))
			{
				continue;
			}
		}

		// Create a new FAssetData for this asset and update it with the gathered data
		AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), FName(*AssetName), FName(*ObjectClassName), MoveTemp(TagsAndValues), PackageFileSummary.ChunkIDs, PackageFileSummary.PackageFlags));
#endif

	}

	return true;
}

bool FExtPackageReader::ReadAssetDataFromThumbnailCache(TArray<FAssetData*>& AssetDataList)
{
	if (!StartSerializeSection(PackageFileSummary.ThumbnailTableOffset))
	{
		return false;
	}

	// Determine the package name and path
	FString PackageName = FPackageName::FilenameToLongPackageName(PackageFilename);
	FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

	// Load the thumbnail count
	int32 ObjectCount = 0;
	*this << ObjectCount;

	// Iterate over every thumbnail entry and harvest the objects classnames
	for(int32 ObjectIdx = 0; ObjectIdx < ObjectCount; ++ObjectIdx)
	{
		// Serialize the classname
		FString AssetClassName;
		*this << AssetClassName;

		// Serialize the object path.
		FString ObjectPathWithoutPackageName;
		*this << ObjectPathWithoutPackageName;

		// Serialize the rest of the data to get at the next object
		int32 FileOffset = 0;
		*this << FileOffset;

		FString GroupNames;
		FString AssetName;

		if (!ensureMsgf(!ObjectPathWithoutPackageName.Contains(TEXT("."), ESearchCase::CaseSensitive), TEXT("Cannot make FAssetData for sub object %s!"), *ObjectPathWithoutPackageName))
		{
			continue;
		}

		// Create a new FAssetData for this asset and update it with the gathered data
		AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), FName(*ObjectPathWithoutPackageName), FTopLevelAssetPath(AssetClassName), FAssetDataTagMap(), PackageFileSummary.ChunkIDs, PackageFileSummary.GetPackageFlags()));
	}

	return true;
}

bool FExtPackageReader::ReadAssetDataFromThumbnailCache(FExtAssetData& OutAssetData)
{
	if (!StartSerializeSection(PackageFileSummary.ThumbnailTableOffset))
	{
		return false;
	}

#if ECB_LEGACY
	// Determine the package name and path
	FString PackageName = FPackageName::FilenameToLongPackageName(PackageFilename);
	FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
#endif

	// Load the thumbnail count
	int32 ObjectCount = 0;
	*this << ObjectCount;

	OutAssetData.ThumbCount = ObjectCount;

	// Iterate over every thumbnail entry and harvest the objects classnames
	//for (int32 ObjectIdx = 0; ObjectIdx < ObjectCount; ++ObjectIdx)
	if (ObjectCount > 0)
	{
		// Serialize the classname
		FString AssetClassName;
		*this << AssetClassName;

		// Serialize the object path.
		FString ObjectPathWithoutPackageName;
		*this << ObjectPathWithoutPackageName;

		// Serialize the rest of the data to get at the next object
		int32 FileOffset = 0;
		*this << FileOffset;

		bool bHaveValidClassName = !AssetClassName.IsEmpty() && AssetClassName != TEXT("???");
		if (OutAssetData.AssetClass == NAME_None && bHaveValidClassName)
		{
			OutAssetData.AssetClass = FName(*AssetClassName);
		}

		const bool bHaveValidAssetName = !ObjectPathWithoutPackageName.IsEmpty();
		if (OutAssetData.AssetName == NAME_None && bHaveValidAssetName)
		{
			OutAssetData.AssetName = FName(*ObjectPathWithoutPackageName);
		}

		if (FileOffset > 0 && FileOffset < PackageFileSize)
		{
			OutAssetData.SetHasThumbnail(true);
		}

#if ECB_LEGACY
		FString GroupNames;
		FString AssetName;

		if (!ensureMsgf(!ObjectPathWithoutPackageName.Contains(TEXT("."), ESearchCase::CaseSensitive), TEXT("Cannot make FAssetData for sub object %s!"), *ObjectPathWithoutPackageName))
		{
			continue;
		}

		// Create a new FAssetData for this asset and update it with the gathered data
		AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), FName(*ObjectPathWithoutPackageName), FName(*AssetClassName), FAssetDataTagMap(), PackageFileSummary.ChunkIDs, PackageFileSummary.PackageFlags));
#endif
	}

	return true;
}

bool FExtPackageReader::ReadDependencyData(FExtAssetData& OutAssetData)
{
	FExtPackageDependencyData DependencyData;
	{
		DependencyData.PackageName = OutAssetData.PackageName;// FName(*FPackageName::FilenameToLongPackageName(PackageFilename));
		DependencyData.PackageData.DiskSize = PackageFileSize;
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		DependencyData.PackageData.PackageGuid = PackageFileSummary.Guid;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		if (!SerializeNameMap())
		{
			return false;
		}
		if (!SerializeImportMap())
		{
			return false;
		}
		if (!SerializeSoftPackageReferenceList(DependencyData.SoftPackageReferenceList))
		{
			return false;
		}
		if (!SerializeSearchableNamesMap(DependencyData))
		{
			return false;
		}

		SerializeExportMap();

		DependencyData.ImportMap = ImportMap;
		DependencyData.ExportMap = ExportMap;
	}

	FName LinkerName = *PackageFilename;
	auto& HardDependentPackages = OutAssetData.HardDependentPackages;

	for (int32 i = 0; i < ImportMap.Num(); ++i)
	{
		FObjectImport& import = ImportMap[i];

		FName DependentPackageName = NAME_None;
		if (!import.OuterIndex.IsNull())
		{
			// Find the package which contains this import.  import.SourceLinker is cleared in EndLoad, so we'll need to do this manually now.
			FPackageIndex OutermostLinkerIndex = import.OuterIndex;
			for (FPackageIndex LinkerIndex = import.OuterIndex; !LinkerIndex.IsNull(); )
			{
				OutermostLinkerIndex = LinkerIndex;
				LinkerIndex = DependencyData.ImpExp(LinkerIndex).OuterIndex;
			}
			check(!OutermostLinkerIndex.IsNull());
			DependentPackageName = DependencyData.ImpExp(OutermostLinkerIndex).ObjectName;
		}

		if (DependentPackageName == NAME_None && import.ClassName == NAME_Package)
		{
			DependentPackageName = import.ObjectName;
		}

		if (DependentPackageName != NAME_None && DependentPackageName != LinkerName)
		{
			HardDependentPackages.Add(DependentPackageName);
		}

		if (import.ClassPackage != NAME_None && import.ClassPackage != LinkerName)
		{
			HardDependentPackages.Add(import.ClassPackage);
		}
	}

	if (HardDependentPackages.Num())
	{
		ECB_LOG(Display, TEXT("----------FPackageReader::ReadDependencyData-----------------"));
		ECB_LOG(Display, TEXT("\tPackages referenced by %s:"), *LinkerName.ToString());
		int32 i = 0;
		for (const FName& Dependent : HardDependentPackages)
		{
			ECB_LOG(Display, TEXT("\t\t%d) %s"), i, *Dependent.ToString());
			i++;
		}
	}

	// Get SoftReferences
	{
		ECB_LOG(Display, TEXT("----------FPackageReader::GetSoftReferences-----------------"));
		OutAssetData.SoftReferencesList.Append(DependencyData.SoftPackageReferenceList);

		int32 SoftIdx = 0;
		for (const FName& SoftPackageName : OutAssetData.SoftReferencesList)
		{
			ECB_LOG(Display, TEXT("\t\t%d) %s"), SoftIdx, *SoftPackageName.ToString());
			SoftIdx++;
		}
	}

	return true;
}

bool FExtPackageReader::ReadThumbnail(FObjectThumbnail& OutThumbnail)
{
	if (PackageFileSummary.ThumbnailTableOffset > 0)
	{
		// Seek the the part of the file where the thumbnail table lives
		Seek(PackageFileSummary.ThumbnailTableOffset);

		// Load the thumbnail count
		int32 ThumbnailCount = 0;
		*this << ThumbnailCount;

		// Load the names and file offsets for the thumbnails in this package
		if (ThumbnailCount > 0)
		{
			FString ObjectClassName;
			*this << ObjectClassName;

			FString ObjectPathWithoutPackageName;
			*this << ObjectPathWithoutPackageName;

			// File offset to image data
			int32 FileOffset = 0;
			*this << FileOffset;

			if (FileOffset > 0)
			{
				Seek(FileOffset);

				// Load the image data
				OutThumbnail.Serialize(*this);

				return true;
			}
		}
	}
	return false;
}

bool FExtPackageReader::ReadAssetRegistryDataIfCookedPackage(TArray<FAssetData*>& AssetDataList, TArray<FString>& CookedPackageNamesWithoutAssetData)
{
	if (!!(GetPackageFlags() & PKG_FilterEditorOnly))
	{
		const FString PackageName = FPackageName::FilenameToLongPackageName(PackageFilename);
		
		bool bFoundAtLeastOneAsset = false;

		// If the packaged is saved with the right version we have the information
		// which of the objects in the export map as the asset.
		// Otherwise we need to store a temp minimal data and then force load the asset
		// to re-generate its registry data
		if (UEVer() >= VER_UE4_COOKED_ASSETS_IN_EDITOR_SUPPORT)
		{
			const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

			SerializeNameMap();
			SerializeImportMap();
			SerializeExportMap();
			for (FObjectExport& Export : ExportMap)
			{
				if (Export.bIsAsset)
				{
					// We need to get the class name from the import/export maps
					FString ObjectClassName;
					if (Export.ClassIndex.IsNull())
					{
						ObjectClassName = UClass::StaticClass()->GetPathName();
					}
					else if (Export.ClassIndex.IsExport())
					{
						const FObjectExport& ClassExport = ExportMap[Export.ClassIndex.ToExport()];
						ObjectClassName = PackageName;
						ObjectClassName += '.';
						ClassExport.ObjectName.AppendString(ObjectClassName);
					}
					else if (Export.ClassIndex.IsImport())
					{
						const FObjectImport& ClassImport = ImportMap[Export.ClassIndex.ToImport()];
						const FObjectImport& ClassPackageImport = ImportMap[ClassImport.OuterIndex.ToImport()];
						ClassPackageImport.ObjectName.AppendString(ObjectClassName);
						ObjectClassName += '.';
						ClassImport.ObjectName.AppendString(ObjectClassName);
					}

					AssetDataList.Add(new FAssetData(FName(*PackageName), FName(*PackagePath), Export.ObjectName, FTopLevelAssetPath(ObjectClassName), FAssetDataTagMap(), TArray<int32>(), GetPackageFlags()));
					bFoundAtLeastOneAsset = true;
				}
			}
		}
		if (!bFoundAtLeastOneAsset)
		{
			CookedPackageNamesWithoutAssetData.Add(PackageName);
		}
		return true;
	}

	return false;
}

bool FExtPackageReader::ReadDependencyData(FExtPackageDependencyData& OutDependencyData)
{
	OutDependencyData.PackageName = FName(*FPackageName::FilenameToLongPackageName(PackageFilename));
	OutDependencyData.PackageData.DiskSize = PackageFileSize;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	OutDependencyData.PackageData.PackageGuid = PackageFileSummary.Guid;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	SerializeNameMap();
	SerializeImportMap();
	SerializeSoftPackageReferenceList(OutDependencyData.SoftPackageReferenceList);
	SerializeSearchableNamesMap(OutDependencyData);

	return true;
}

bool FExtPackageReader::SerializeNameMap()
{
	if( PackageFileSummary.NameCount > 0 )
	{
		if (!StartSerializeSection(PackageFileSummary.NameOffset))
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeNameMapInvalidNameOffset", PackageFilename);
			return false;
		}

		const int MinSizePerNameEntry = 1;
		if (PackageFileSize < Tell() + PackageFileSummary.NameCount * MinSizePerNameEntry)
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeNameMapInvalidNameCount", PackageFilename);
			return false;
		}

		for ( int32 NameMapIdx = 0; NameMapIdx < PackageFileSummary.NameCount; ++NameMapIdx )
		{
			// Read the name entry from the file.
			FNameEntrySerialized NameEntry(ENAME_LinkerConstructor);
			*this << NameEntry;
			if (IsError())
			{
				ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeNameMapInvalidName", PackageFilename);
				return false;
			}
			NameMap.Add(FName(NameEntry));
		}
	}

	return true;
}

bool FExtPackageReader::SerializeImportMap()
{
	if (ImportMap.Num() > 0)
	{
		return true;
	}

	if (PackageFileSummary.ImportCount > 0)
	{
		if (!StartSerializeSection(PackageFileSummary.ImportOffset))
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeImportMapInvalidImportOffset", PackageFilename);
			return false;
		}

		const int MinSizePerImport = 1;
		if (PackageFileSize < Tell() + PackageFileSummary.ImportCount * MinSizePerImport)
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeImportMapInvalidImportCount", PackageFilename);
			return false;
		}
		ImportMap.Reserve(PackageFileSummary.ImportCount);
		for (int32 ImportMapIdx = 0; ImportMapIdx < PackageFileSummary.ImportCount; ++ImportMapIdx)
		{
			*this << ImportMap.Emplace_GetRef();
			if (IsError())
			{
				ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeImportMapInvalidImport", PackageFilename);
				ImportMap.Reset();
				return false;
			}
		}
	}

	return true;
}

bool FExtPackageReader::SerializeExportMap()
{
	if (ExportMap.Num() > 0)
	{
		return true;
	}

	if (PackageFileSummary.ExportCount > 0)
	{
		if (!StartSerializeSection(PackageFileSummary.ExportOffset))
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeExportMapInvalidExportOffset", PackageFilename);
			return false;
		}

		const int MinSizePerExport = 1;
		if (PackageFileSize < Tell() + PackageFileSummary.ExportCount * MinSizePerExport)
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeExportMapInvalidExportCount", PackageFilename);
			return false;
		}
		ExportMap.Reserve(PackageFileSummary.ExportCount);
		for (int32 ExportMapIdx = 0; ExportMapIdx < PackageFileSummary.ExportCount; ++ExportMapIdx)
		{
			*this << ExportMap.Emplace_GetRef();
			if (IsError())
			{
				ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeExportMapInvalidExport", PackageFilename);
				ExportMap.Reset();
				return false;
			}
		}
	}

	return true;
}

bool FExtPackageReader::SerializeSoftPackageReferenceList(TArray<FName>& OutSoftPackageReferenceList)
{
	if (UEVer() >= VER_UE4_ADD_STRING_ASSET_REFERENCES_MAP && PackageFileSummary.SoftPackageReferencesOffset > 0 && PackageFileSummary.SoftPackageReferencesCount > 0)
	{
		if (!StartSerializeSection(PackageFileSummary.SoftPackageReferencesOffset))
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeSoftPackageReferenceListInvalidReferencesOffset", PackageFilename);
			return false;
		}

		const int MinSizePerSoftPackageReference = 1;
		if (PackageFileSize < Tell() + PackageFileSummary.SoftPackageReferencesCount * MinSizePerSoftPackageReference)
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeSoftPackageReferenceListInvalidReferencesCount", PackageFilename);
			return false;
		}
		if (UEVer() < VER_UE4_ADDED_SOFT_OBJECT_PATH)
		{
			for (int32 ReferenceIdx = 0; ReferenceIdx < PackageFileSummary.SoftPackageReferencesCount; ++ReferenceIdx)
			{
				FString PackageName;
				*this << PackageName;
				if (IsError())
				{
					ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeSoftPackageReferenceListInvalidReferencePreSoftObjectPath", PackageFilename);
					return false;
				}

				if (UEVer() < VER_UE4_KEEP_ONLY_PACKAGE_NAMES_IN_STRING_ASSET_REFERENCES_MAP)
				{
					PackageName = FPackageName::GetNormalizedObjectPath(PackageName);
					if (!PackageName.IsEmpty())
					{
						PackageName = FPackageName::ObjectPathToPackageName(PackageName);
					}
				}

				OutSoftPackageReferenceList.Add(FName(*PackageName));
			}
		}
		else
		{
			for (int32 ReferenceIdx = 0; ReferenceIdx < PackageFileSummary.SoftPackageReferencesCount; ++ReferenceIdx)
			{
				FName PackageName;
				*this << PackageName;
				if (IsError())
				{
					ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeSoftPackageReferenceListInvalidReference", PackageFilename);
					return false;
				}

				OutSoftPackageReferenceList.Add(PackageName);
			}
		}
	}

	return true;
}

bool FExtPackageReader::SerializeSearchableNamesMap(FExtPackageDependencyData& OutDependencyData)
{
	if (UEVer() >= VER_UE4_ADDED_SEARCHABLE_NAMES && PackageFileSummary.SearchableNamesOffset > 0)
	{
		if (!StartSerializeSection(PackageFileSummary.SearchableNamesOffset))
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeSearchableNamesMapInvalidOffset", PackageFilename);
			return false;
		}

		OutDependencyData.SerializeSearchableNamesMap(*this);
		if (IsError())
		{
			ECB_PACKAGEREADER_CORRUPTPACKAGE_WARNING("SerializeSearchableNamesMapInvalidSearchableNamesMap", PackageFilename);
			return false;
		}
	}

	return true;
}

void FExtPackageReader::Serialize( void* V, int64 Length )
{
	check(Loader);
	Loader->Serialize( V, Length );
}

bool FExtPackageReader::Precache( int64 PrecacheOffset, int64 PrecacheSize )
{
	check(Loader);
	return Loader->Precache( PrecacheOffset, PrecacheSize );
}

void FExtPackageReader::Seek( int64 InPos )
{
	check(Loader);
	Loader->Seek( InPos );
}

int64 FExtPackageReader::Tell()
{
	check(Loader);
	return Loader->Tell();
}

int64 FExtPackageReader::TotalSize()
{
	check(Loader);
	return Loader->TotalSize();
}

uint32 FExtPackageReader::GetPackageFlags() const
{
	return PackageFileSummary.GetPackageFlags();
}

const FPackageFileSummary& FExtPackageReader::GetPackageFileSummary() const
{
	return PackageFileSummary;
}

int32 FExtPackageReader::GetSoftPackageReferencesCount() const
{
	if (UEVer() >= VER_UE4_ADD_STRING_ASSET_REFERENCES_MAP && PackageFileSummary.SoftPackageReferencesOffset > 0 && PackageFileSummary.SoftPackageReferencesCount > 0)
	{
		return PackageFileSummary.SoftPackageReferencesCount;
	}
	return 0;
}

FArchive& FExtPackageReader::operator<<( FName& Name )
{
	check(Loader);

	int32 NameIndex;
	FArchive& Ar = *this;
	Ar << NameIndex;

	if( !NameMap.IsValidIndex(NameIndex) )
	{
		ECB_LOG(Fatal, TEXT("Bad name index %i/%i"), NameIndex, NameMap.Num() );
	}

	// if the name wasn't loaded (because it wasn't valid in this context)
	if (NameMap[NameIndex] == NAME_None)
	{
		int32 TempNumber;
		Ar << TempNumber;
		Name = NAME_None;
	}
	else
	{
		int32 Number;
		Ar << Number;
		// simply create the name from the NameMap's name and the serialized instance number
		Name = FName(NameMap[NameIndex], Number);
	}

	return *this;
}

FName FExtPackageDependencyData::GetImportPackageName(int32 ImportIndex)
{
	FName Result;
	for (FPackageIndex LinkerIndex = FPackageIndex::FromImport(ImportIndex); !LinkerIndex.IsNull();)
	{
		FObjectResource* Resource = &ImpExp(LinkerIndex);
		LinkerIndex = Resource->OuterIndex;
		if (LinkerIndex.IsNull())
		{
			Result = Resource->ObjectName;
		}
	}

	return Result;
}


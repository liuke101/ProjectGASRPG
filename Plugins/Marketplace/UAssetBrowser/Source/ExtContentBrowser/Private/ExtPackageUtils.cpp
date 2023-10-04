// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtPackageUtils.h"
#include "ExtContentBrowser.h"
#include "ExtAssetData.h"
#include "ExtPackageReader.h"

#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "UObject/ObjectResource.h"
#include "UObject/LinkerLoad.h"
#include "UObject/CoreRedirects.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Commandlets/Commandlet.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Serialization/ArchiveCountMem.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "GameFramework/WorldSettings.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "PackageHelperFunctions.h"
#include "PackageUtilityWorkers.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ObjectThumbnail.h"
#include "Misc/PackageName.h"
#include "Misc/ConfigCacheIni.h"

#include "CollectionManagerTypes.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"

#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Serialization/ArchiveStackTrace.h"
#include "Misc/OutputDeviceHelper.h"
#include "Misc/OutputDeviceFile.h"
#include "UObject/UObjectThreadContext.h"
#include "UObject/MetaData.h"

struct FExportInfo
{
	FObjectExport Export;
	int32 ExportIndex;
	FString PathName;
	FString OuterPathName;

	FExportInfo(FLinkerLoad* Linker, int32 InIndex)
		: Export(Linker->ExportMap[InIndex]), ExportIndex(InIndex)
		, OuterPathName(TEXT("NULL"))
	{
		PathName = Linker->GetExportPathName(ExportIndex);
		SetOuterPathName(Linker);
	}

	void SetOuterPathName(FLinkerLoad* Linker)
	{
		if (!Export.OuterIndex.IsNull())
		{
			OuterPathName = Linker->GetPathName(Export.OuterIndex);
		}
	}
};

namespace
{
	enum EExportSortType
	{
		EXPORTSORT_ExportSize,
		EXPORTSORT_ExportIndex,
		EXPORTSORT_ObjectPathname,
		EXPORTSORT_OuterPathname,
		EXPORTSORT_MAX
	};

	struct FObjectExport_Sorter
	{
		static EExportSortType SortPriority[EXPORTSORT_MAX];

		// Comparison method
		bool operator()(const FExportInfo& A, const FExportInfo& B) const
		{
			int32 Result = 0;

			for (int32 PriorityType = 0; PriorityType < EXPORTSORT_MAX; PriorityType++)
			{
				switch (SortPriority[PriorityType])
				{
				case EXPORTSORT_ExportSize:
					Result = B.Export.SerialSize - A.Export.SerialSize;
					break;

				case EXPORTSORT_ExportIndex:
					Result = A.ExportIndex - B.ExportIndex;
					break;

				case EXPORTSORT_ObjectPathname:
					Result = A.PathName.Len() - B.PathName.Len();
					if (Result == 0)
					{
						Result = FCString::Stricmp(*A.PathName, *B.PathName);
					}
					break;

				case EXPORTSORT_OuterPathname:
					Result = A.OuterPathName.Len() - B.OuterPathName.Len();
					if (Result == 0)
					{
						Result = FCString::Stricmp(*A.OuterPathName, *B.OuterPathName);
					}
					break;

				case EXPORTSORT_MAX:
					return !!Result;
				}

				if (Result != 0)
				{
					break;
				}
			}
			return Result < 0;
		}
	};

	EExportSortType FObjectExport_Sorter::SortPriority[EXPORTSORT_MAX] =
	{ EXPORTSORT_ExportIndex, EXPORTSORT_ExportSize, EXPORTSORT_OuterPathname, EXPORTSORT_ObjectPathname };
}

/** Given a package filename, creates a linker and a temporary package. The file name does not need to point to a package under the current project content folder */
FLinkerLoad* CreateLinker(FUObjectSerializeContext* LoadContext, const FString& InFileName, const FString& InPackageName)
{
	UPackage* Package = FindObjectFast<UPackage>(nullptr, *InPackageName);
	if (!Package)
	{
		Package = CreatePackage(*InPackageName);
	}

	uint32 LoadFlags = LOAD_NoVerify;// | LOAD_NoWarn | LOAD_Quiet;
	FLinkerLoad* Linker = FLinkerLoad::CreateLinker(LoadContext, Package, FPackagePath::FromLocalPath(InFileName), LOAD_NoVerify);

	if (Linker && Package)
	{
		Package->SetPackageFlags(PKG_ForDiffing);
	}

 	return Linker;
}

/** Resets linkers on packages after they have finished loading */
void ResetLoaders(const FString& InPackageName)
{
	if (IsGarbageCollectingAndLockingUObjectHashTables())
		return;

	UPackage* ExistingPackage = FindObject<UPackage>(nullptr, *InPackageName, /*bool ExactClass*/true);
	if (ExistingPackage != nullptr)
	{
		ResetLoaders(ExistingPackage);
		ExistingPackage->ClearFlags(RF_WasLoaded);
		ExistingPackage->bHasBeenFullyLoaded = false;
		ExistingPackage->GetMetaData()->RemoveMetaDataOutsidePackage();
	}
}

/////////////////////////////////////////////////////////////
// FExtPackageUtilities implementation
//

#if ECB_LEGACY
bool FExtPackageUtils::ParsePackage(struct FExtAssetData& OutExtAssetData)
{
	FString FileName = OutExtAssetData.PackageFilePath.ToString();
	FString PackageName = OutExtAssetData.PackageName.ToString();

	// Try reset the loaders for the packages we want to load so that we don't find the wrong version of the file
	ResetLoaders(PackageName);

	FLinkerLoad* Linker = nullptr;
	UPackage* Package = nullptr;
	FArchiveStackTraceReader* Reader = nullptr;

	{
		TGuardValue<bool> GuardAllowUnversionedContentInEditor(GAllowUnversionedContentInEditor, true);
		TGuardValue<int32> GuardAllowCookedContentInEditor(GAllowCookedDataInEditorBuilds, 1);
		TRefCountPtr<FUObjectSerializeContext> LoadContext(FUObjectThreadContext::Get().GetSerializeContext());
		BeginLoad(LoadContext);
		Linker = CreateLinker(LoadContext, FileName, PackageName);
		EndLoad(Linker ? Linker->GetSerializeContext() : LoadContext.GetReference());
	}

	if (!Linker)
	{
		OutExtAssetData.InvalidReason = FExtAssetData::EInvalidReason::FailedToLoadPackage;
		return false;
	}

	// Text format is not supported for now
	if (Linker->IsTextFormat())
	{
		OutExtAssetData.InvalidReason = FExtAssetData::EInvalidReason::TextForamtNotSupported;
		return false;
	}

#if ECB_DISABLE//ECB_DEBUG
	// Generate package report for debugging
	{
		uint32 InfoFlags = PKGINFO_None;
		
		// names
		//InfoFlags |= PKGINFO_Names;
		// imports
		//InfoFlags |= PKGINFO_Imports;
		// exports
		//InfoFlags |= PKGINFO_Exports;
		// simple
		//InfoFlags |= PKGINFO_Compact;
		// depends
		InfoFlags |= PKGINFO_Depends;
		// paths
		//InfoFlags |= PKGINFO_Paths;
		// thumbnails
		InfoFlags |= PKGINFO_Thumbs;
		// lazy
		//InfoFlags |= PKGINFO_Lazy;
		// asset registry
		InfoFlags |= PKGINFO_AssetRegistry;

		const bool bHideOffsets = false; // Determines whether FObjectExport::SerialOffset will be included in the output.
		GeneratePackageReport(Linker, InfoFlags, bHideOffsets);
	}
#endif

	// Parsing package
	{
		// Version Info
		{
			OutExtAssetData.FileVersionUE4 = Linker->UE4Ver();
			OutExtAssetData.SavedByEngineVersion = Linker->Summary.SavedByEngineVersion;
			OutExtAssetData.CompatibleWithEngineVersion = Linker->Summary.CompatibleWithEngineVersion;
		}

		int64 PackageFileSize = Linker->GetLoader_Unsafe()->TotalSize();

		// AssetRegistry
		if (Linker->Summary.AssetRegistryDataOffset > 0 && Linker->Summary.AssetRegistryDataOffset < PackageFileSize)
		{
			{
				const int32 NextOffset = Linker->Summary.WorldTileInfoDataOffset ? Linker->Summary.WorldTileInfoDataOffset : Linker->Summary.TotalHeaderSize;
				const int32 AssetRegistrySize = NextOffset - Linker->Summary.AssetRegistryDataOffset;
				//ECB_LOG(Display, TEXT("Asset Registry Size: %10i"), AssetRegistrySize);
			}

			// Seek to the AssetRegistry table of contents
			Linker->GetLoader_Unsafe()->Seek(Linker->Summary.AssetRegistryDataOffset);

			// Load the number of assets in the tag map
			int32 AssetCount = 0;
			*Linker << AssetCount;

			//ECB_LOG(Display, TEXT("Number of assets with Asset Registry data: %d"), AssetCount);

			// Only care about first asset
			// for (int32 AssetIdx = 0; AssetIdx < AssetCount; ++AssetIdx)
			if (AssetCount > 0)
			{
				// Display the asset class and path
				FString ObjectPath;
				FString ObjectClassName;
				int32 TagCount = 0;

				*Linker << ObjectPath;
				*Linker << ObjectClassName;
				*Linker << TagCount;

				//ECB_LOG(Display, TEXT("\t\t%s'%s' (%d Tags)"), *ObjectClassName, *ObjectPath, TagCount); // Output : StaticMesh'SM_Test_LOD0' (14 Tags)
				OutExtAssetData.AssetName = FName(*ObjectPath);
				OutExtAssetData.AssetClass = FName(*ObjectClassName);

				// Get all tags on this asset
				FAssetDataTagMap NewTagsAndValues;
				for (int32 TagIdx = 0; TagIdx < TagCount; ++TagIdx)
				{
					FString Key;
					FString Value;
					*Linker << Key;
					*Linker << Value;

					//ECB_LOG(Display, TEXT("\t\t\t\"%s\": \"%s\""), *Key, *Value);
					NewTagsAndValues.Add(FName(*Key), Value);
				}
				OutExtAssetData.TagsAndValues = FAssetDataTagMapSharedView(MoveTemp(NewTagsAndValues));
			}
		}

		// Thumbnail Data
		if (Linker->Summary.ThumbnailTableOffset > 0 && Linker->Summary.ThumbnailTableOffset < PackageFileSize)
		{
			Linker->GetLoader_Unsafe()->Seek(Linker->Summary.ThumbnailTableOffset);

			// Load the thumbnail count
			int32 ThumbnailCount = 0;
			*Linker << ThumbnailCount;

			// Load the names and file offsets for the thumbnails in this package
			//for (int32 CurThumbnailIndex = 0; CurThumbnailIndex < ThumbnailCount; ++CurThumbnailIndex)
			if (ThumbnailCount > 0)
			{
				int32 CurThumbnailIndex = 0;

				FString ObjectClassName;
				*Linker << ObjectClassName;

				// Object path
				FString ObjectPath;
				*Linker << ObjectPath;

				//ECB_LOG(Display, TEXT("\t\t%d) %s'%s' (%d thumbs)"), CurThumbnailIndex, *ObjectClassName, *ObjectPath, ThumbnailCount);

				bool bHaveValidClassName = !ObjectClassName.IsEmpty() && ObjectClassName != TEXT("???");
				if (OutExtAssetData.AssetClass == NAME_None && bHaveValidClassName)
				{
					OutExtAssetData.AssetClass = FName(*ObjectClassName);
				}

				const bool bHaveValidAssetName = !ObjectPath.IsEmpty();
				if (OutExtAssetData.AssetName == NAME_None && bHaveValidAssetName)
				{
					OutExtAssetData.AssetName = FName(*ObjectPath);
				}

				// File offset to image data
				int32 FileOffset = 0;
				*Linker << FileOffset;

				if (FileOffset > 0 && FileOffset < PackageFileSize)
				{
					OutExtAssetData.bHasThumbnail = true;
				}
			}
		}

		// Dependencies
		{
			FName LinkerName = Linker->LinkerRoot->GetFName();
			TSet<FName>& DependentPackages = OutExtAssetData.HardDependentPackages;
			for (int32 i = 0; i < Linker->ImportMap.Num(); ++i)
			{
				FObjectImport& import = Linker->ImportMap[i];

				FName DependentPackageName = NAME_None;
				if (!import.OuterIndex.IsNull())
				{
					// Find the package which contains this import.  import.SourceLinker is cleared in EndLoad, so we'll need to do this manually now.
					FPackageIndex OutermostLinkerIndex = import.OuterIndex;
					for (FPackageIndex LinkerIndex = import.OuterIndex; !LinkerIndex.IsNull(); )
					{
						OutermostLinkerIndex = LinkerIndex;
						LinkerIndex = Linker->ImpExp(LinkerIndex).OuterIndex;
					}
					check(!OutermostLinkerIndex.IsNull());
					DependentPackageName = Linker->ImpExp(OutermostLinkerIndex).ObjectName;
				}

				if (DependentPackageName == NAME_None && import.ClassName == NAME_Package)
				{
					DependentPackageName = import.ObjectName;
				}

				if (DependentPackageName != NAME_None && DependentPackageName != LinkerName)
				{
					DependentPackages.Add(DependentPackageName);
				}

				if (import.ClassPackage != NAME_None && import.ClassPackage != LinkerName)
				{
					DependentPackages.Add(import.ClassPackage);
				}
			}

			if (DependentPackages.Num())
			{
				ECB_LOG(Display, TEXT("-----------FExtPackageUtilities::ParsePackage----------------"));
				ECB_LOG(Display, TEXT("\tPackages referenced by %s:"), *LinkerName.ToString());
				int32 i = 0;
				for (const FName& Dependent : DependentPackages)
				{
					ECB_LOG(Display, TEXT("\t\t%i) %s"), i++, *Dependent.ToString());
				}
			}
		}

		// Soft References
		if (Linker->Summary.SoftPackageReferencesCount > 0)
		{
			OutExtAssetData.SoftPackageReferencesCount = Linker->Summary.SoftPackageReferencesCount;
		}
	}

	// Reset linker to free memory 
	ResetLoaders(PackageName);

	return true;
}

bool FExtPackageUtils::LoadThumbnail(const FExtAssetData& InExtAssetData, FObjectThumbnail& InOutThumbnail)
{
	bool bThumbnailLoaded = false;

	FString FileName = InExtAssetData.PackageFilePath.ToString();
	FString PackageName = InExtAssetData.PackageName.ToString();

	// Try reset the loaders for the packages we want to load so that we don't find the wrong version of the file
	ResetLoaders(PackageName);

	FLinkerLoad* Linker = nullptr;
	{
		TGuardValue<bool> GuardAllowUnversionedContentInEditor(GAllowUnversionedContentInEditor, true);
		TGuardValue<int32> GuardAllowCookedContentInEditor(GAllowCookedDataInEditorBuilds, 1);
		TRefCountPtr<FUObjectSerializeContext> LoadContext(FUObjectThreadContext::Get().GetSerializeContext());
		BeginLoad(LoadContext);
		Linker = CreateLinker(LoadContext, FileName, PackageName);
		EndLoad(Linker ? Linker->GetSerializeContext() : LoadContext.GetReference());
	}

	if (Linker)
	{
		if (Linker->SerializeThumbnails(true) && Linker->LinkerRoot->HasThumbnailMap())
		{
			FThumbnailMap& LinkerThumbnails = Linker->LinkerRoot->AccessThumbnailMap();
			for (TMap<FName, FObjectThumbnail>::TIterator It(LinkerThumbnails); It; ++It)
			{
				FName& ObjectFullName = It.Key();
				InOutThumbnail = It.Value();

				//ECB_LOG(Display, TEXT("[LoadThumbnail]  %s: %ix%i\t\tImage Data:%i bytes"), *ObjectFullName.ToString(), InOutThumbnail.GetImageWidth(), InOutThumbnail.GetImageHeight(), InOutThumbnail.GetCompressedDataSize());

				bThumbnailLoaded = true;
			}
		}

		ResetLoaders(PackageName);
	}

	return bThumbnailLoaded;
}


void FExtPackageUtils::LoadSoftReferences(const FExtAssetData& InExtAssetData, TArray<FName>& OutSoftPackageReferenceList)
{
	ECB_LOG(Display, TEXT("[LoadSoftReferences] Soft Package References Count: %10i"), InExtAssetData.SoftPackageReferencesCount);

	if (InExtAssetData.SoftPackageReferencesCount <= 0)
	{
		return;
	}

	FString FileName = InExtAssetData.PackageFilePath.ToString();

	{
		FPackageReader Reader;
		//Reader.OpenPackageFile(*Linker->GetLoader_Unsafe());
		if (Reader.OpenPackageFile(FileName))//, EOpenPackageResult * OutErrorCode)
		{
			Reader.SerializeNameMap();
			Reader.SerializeSoftPackageReferenceList(OutSoftPackageReferenceList);

			for (int32 SoftIdx = 0; SoftIdx < OutSoftPackageReferenceList.Num(); ++SoftIdx)
			{
				FName SoftPackageName = OutSoftPackageReferenceList[SoftIdx];
				ECB_LOG(Display, TEXT("\t\t%d) %s"), SoftIdx, *SoftPackageName.ToString());
			}
		}
	}
}

#endif

bool FExtPackageUtils::ParsePackageWithPackageReader(struct FExtAssetData& OutAssetData)
{
	FString PackageFilePath = OutAssetData.PackageFilePath.ToString();
	FString PackageName = OutAssetData.PackageName.ToString();

	// Try reset the loaders for the packages we want to load so that we don't find the wrong version of the file
	ResetLoaders(PackageName);

	FExtPackageReader PackageReader;
	FExtPackageReader::EOpenPackageResult ErrorCode;
	if (!PackageReader.OpenPackageFile(PackageFilePath, &ErrorCode))
	{
		FExtAssetData::EInvalidReason InvalidReason = FExtAssetData::EInvalidReason::Unknown;

		switch (ErrorCode)
		{
			case FExtPackageReader::EOpenPackageResult::FailedToLoad	:
			ECB_LOG(Display, TEXT("Failed to load %s, VersionTooOld!"), *PackageFilePath);
			InvalidReason = FExtAssetData::EInvalidReason::PackageFileFailedToLoad;
			break;
			case FExtPackageReader::EOpenPackageResult::VersionTooOld:
				ECB_LOG(Display, TEXT("Failed to open %s, VersionTooOld!"), *PackageFilePath);
				InvalidReason = FExtAssetData::EInvalidReason::PackageFileVersionEmptyOrTooOld;
				break;
			case FExtPackageReader::EOpenPackageResult::VersionTooNew:
				ECB_LOG(Display, TEXT("Failed to open %s, VersionTooNew!"), *PackageFilePath);
				InvalidReason = FExtAssetData::EInvalidReason::PackageFileOrCustomVersionTooNew;
			break; 
			case FExtPackageReader::EOpenPackageResult::NoLoader:
				ECB_LOG(Display, TEXT("Failed to open %s, NoLoader!"), *PackageFilePath);
				break;
			case FExtPackageReader::EOpenPackageResult::MalformedTag:
				ECB_LOG(Display, TEXT("Failed to open %s, MalformedTag!"), *PackageFilePath);
				InvalidReason = FExtAssetData::EInvalidReason::PackageFileMalformed;
				break;
			
			case FExtPackageReader::EOpenPackageResult::CustomVersionMissing:
				ECB_LOG(Display, TEXT("Failed to open %s, CustomVersionMissing!"), *PackageFilePath);
				InvalidReason = FExtAssetData::EInvalidReason::PackageFileCustomVersionMissing;
				break;
			default:
				InvalidReason = FExtAssetData::EInvalidReason::Unknown;
				ECB_LOG(Display, TEXT("Failed to open %s!"), *PackageFilePath);
		}
		OutAssetData.InvalidReason = InvalidReason;
		OutAssetData.FileSize = PackageReader.TotalSize();

		ResetLoaders(PackageName);
		return false;
	}

	const bool bCooked = !!(PackageReader.GetPackageFlags() & PKG_FilterEditorOnly);
	if (bCooked)
	{
		ECB_LOG(Display, TEXT("Cooked asset is not supported: %s!"), *PackageFilePath);
		OutAssetData.InvalidReason = FExtAssetData::EInvalidReason::CookedPackageNotSupported;

		ResetLoaders(PackageName);
		return false;
	}

	// Version Info
	{
		OutAssetData.FileVersionUE4 = PackageReader.UEVer().ToValue();
		OutAssetData.SavedByEngineVersion = PackageReader.GetPackageFileSummary().SavedByEngineVersion;
		OutAssetData.CompatibleWithEngineVersion = PackageReader.GetPackageFileSummary().CompatibleWithEngineVersion;
	}

	// AssetRegistry
	{
		PackageReader.ReadAssetRegistryData(OutAssetData);
	}

	// Thumbnail Data
	{
		PackageReader.ReadAssetDataFromThumbnailCache(OutAssetData);
	}

	// Dependencies and soft references
	{
		PackageReader.ReadDependencyData(OutAssetData);
	}

	{
		OutAssetData.FileSize = PackageReader.TotalSize();
	}

	// Reset linker to free memory 
	ResetLoaders(PackageName);

	return true;
}

bool FExtPackageUtils::LoadThumbnailWithPackageReader(const FExtAssetData& InAssetData, FObjectThumbnail& InOutThumbnail)
{
	FString PackageFilePath = InAssetData.PackageFilePath.ToString();
	FString PackageName = InAssetData.PackageName.ToString();

	// Try reset the loaders for the packages we want to load so that we don't find the wrong version of the file
	ResetLoaders(PackageName);

	FExtPackageReader PackageReader;
	if (PackageReader.OpenPackageFile(PackageFilePath))
	{

		bool bResult = PackageReader.ReadThumbnail(InOutThumbnail);

		// Reset linker to free memory 
		ResetLoaders(PackageName);

		return bResult;
	}

	// Reset linker to free memory 
	ResetLoaders(PackageName);

	return false;
}

void FExtPackageUtils::UnloadPackage(const TArray<FString>& InFilePaths, TArray<UPackage*>* OutUnloadPackage)
{
	for (const FString& FilePath : InFilePaths)
	{
		FString PackageName;
		if (FPackageName::TryConvertFilenameToLongPackageName(FilePath, PackageName))
		{
			if (UPackage* Package = FindPackage(nullptr, *PackageName))
			{
				if (OutUnloadPackage)
				{
					OutUnloadPackage->Add(Package);
				}

				// Detach the linker if loaded
#if ECB_DISABLE
				if (!Package->IsFullyLoaded())
				{
					FlushAsyncLoading();
					Package->FullyLoad();
				}
#endif
				ResetLoaders(PackageName);
			}
		}
	}	
}

void FExtPackageUtils::GetDirectories(const FString& InBaseDirectory, TArray<FString>& OutPathList)
{
	struct FFolderVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				Directories.Add(FilenameOrDirectory);
			}
			return true;
		}

		TArray<FString> Directories;
	};

	class FFolderStatVisitor : public IPlatformFile::FDirectoryStatVisitor
	{
	public:
		virtual bool Visit(const TCHAR* FilenameOrDirectory, const FFileStatData& StatData) override
		{
			if (StatData.bIsDirectory)
			{
				Directories.Add(FilenameOrDirectory);
			}
			return true;
		}
		TArray<FString> Directories;
	};

	if (IFileManager::Get().DirectoryExists(*InBaseDirectory))
	{
		FFolderVisitor FolderVisitor;
		//FFolderStatVisitor FolderVisitor;
		//IFileManager::Get().IterateDirectoryStat(*InBaseDirectory, FolderVisitor);
		IFileManager::Get().IterateDirectory(*InBaseDirectory, FolderVisitor);

		OutPathList.Append(FolderVisitor.Directories);
	}
}

void FExtPackageUtils::GetDirectoriesRecursively(const FString& InBaseDirectory, TArray<FString>& OutPathList)
{
	struct Local
	{
		static void GetChildDirectories(const TArray<FString>& InBaseDirs, TArray<FString>& OutPathList)
		{
			for (const FString& BaseDir : InBaseDirs)
			{
				TArray<FString> ChildDirs;
				FExtPackageUtils::GetDirectories(BaseDir, ChildDirs);
				OutPathList.Append(ChildDirs);
				GetChildDirectories(ChildDirs, OutPathList);
			}
		}
	};

	double GetAllDirectoriesStartTime = FPlatformTime::Seconds();

	if (IFileManager::Get().DirectoryExists(*InBaseDirectory))
	{
		Local::GetChildDirectories({ InBaseDirectory }, OutPathList);
	}

	ECB_LOG(Display, TEXT("GetDirectoriesRecursively completed in %0.4f seconds"), FPlatformTime::Seconds() - GetAllDirectoriesStartTime);
}


#if ECB_LEGACY
void FExtPackageUtils::GetDirectories(const FString& InBaseDirectory, TArray<FString>& OutPathList, bool bResursively)
{
	double GetAllDirectoriesStartTime = FPlatformTime::Seconds();

	struct FFolderVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				Directories.Add(FilenameOrDirectory);
			}
			return true;
		}

		TArray<FString> Directories;
	};

	class FFolderStatVisitor : public IPlatformFile::FDirectoryStatVisitor
	{
	public:
		virtual bool Visit(const TCHAR* FilenameOrDirectory, const FFileStatData& StatData) override
		{
			if (StatData.bIsDirectory)
			{
				Directories.Add(FilenameOrDirectory);
			}
			return true;
		}
		TArray<FString> Directories;
	};


	if (IFileManager::Get().DirectoryExists(*InBaseDirectory))
	{
		//FFolderVisitor FolderVisitor;
		FFolderStatVisitor FolderVisitor;
		if (bResursively)
		{
			//IFileManager::Get().IterateDirectoryRecursively(*InBaseDirectory, FolderVisitor);
			IFileManager::Get().IterateDirectoryStatRecursively(*InBaseDirectory, FolderVisitor);
		}
		else
		{
			//IFileManager::Get().IterateDirectory(*InBaseDirectory, FolderVisitor);
			IFileManager::Get().IterateDirectoryStat(*InBaseDirectory, FolderVisitor);
		}
		
		OutPathList.Append(FolderVisitor.Directories);
	}

	ECB_LOG(Display, TEXT("GetAllDirectories completed in %0.4f seconds"), FPlatformTime::Seconds() - GetAllDirectoriesStartTime);
}
#endif

void FExtPackageUtils::GetAllPackages(const FString& InRootDirectory, TArray<FString>& OutFileList, bool bResursively)
{
	struct FPackageVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory && FExtAssetSupport::IsSupportedPackageFilename(FilenameOrDirectory))
			{
				PackageFiles.Add(FilenameOrDirectory);
			}
			return true;
		}

		TArray<FString> PackageFiles;
	};

	if (IFileManager::Get().DirectoryExists(*InRootDirectory))
	{
		FPackageVisitor PackageVisitor;
		if (bResursively)
		{
			IFileManager::Get().IterateDirectoryRecursively(*InRootDirectory, PackageVisitor);
		}
		else
		{
			IFileManager::Get().IterateDirectory(*InRootDirectory, PackageVisitor);
		}

		OutFileList = PackageVisitor.PackageFiles;
	}
}

void FExtPackageUtils::GetAllPackages(const FName& InRootDirectory, TArray<FName>& OutFileList)
{
	struct FPackageVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory && FExtAssetSupport::IsSupportedPackageFilename(FilenameOrDirectory))
			{
				PackageFiles.Add(FilenameOrDirectory);
			}
			return true;
		}

		TArray<FName> PackageFiles;
	};

	FString BaseDir = InRootDirectory.ToString();

	if (IFileManager::Get().DirectoryExists(*BaseDir))
	{
		FPackageVisitor PackageVisitor;
		IFileManager::Get().IterateDirectory(*BaseDir, PackageVisitor);

		OutFileList = PackageVisitor.PackageFiles;
	}
}

bool FExtPackageUtils::HasPackages(const FString& InDirectory, bool bResursively)
{
	struct FPackageVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory && FExtAssetSupport::IsSupportedPackageFilename(FilenameOrDirectory))
			{
				bFoundPackage = true;
				return false;
			}
			return true;
		}

		bool bFoundPackage = false;
	};

	if (IFileManager::Get().DirectoryExists(*InDirectory))
	{
		FPackageVisitor PackageVisitor;
		if (bResursively)
		{
			IFileManager::Get().IterateDirectoryRecursively(*InDirectory, PackageVisitor);
		}
		else
		{
			IFileManager::Get().IterateDirectory(*InDirectory, PackageVisitor);
		}

		return PackageVisitor.bFoundPackage;
	}

	return false;
}

bool FExtPackageUtils::DoesPackageExist(const FString PackageName)
{
	bool bPackageExist = false;

	// Script package should be loaded 
	// todo: or skip if it's soft referenced ?
	if (FPackageName::IsScriptPackage(PackageName))
	{
		bPackageExist = NULL != FindPackage(NULL, *PackageName);

		if (!bPackageExist) // Not found, double check if redirected
		{
			const FCoreRedirect* ValueRedirect = nullptr;
			FCoreRedirectObjectName OldPackageName(PackageName);
			FCoreRedirectObjectName NewPackageName;

			bPackageExist = FCoreRedirects::RedirectNameAndValues(ECoreRedirectFlags::Type_Package, OldPackageName, NewPackageName, &ValueRedirect);
		}
	}
	else
	{
		// assume it's long package name
		bPackageExist = FPackageName::DoesPackageExist(PackageName);
	}

	return bPackageExist;
}

// PackageUtilities.cpp
void FExtPackageUtils::GeneratePackageReport(FLinkerLoad* Linker, uint32 InfoFlags, bool bHideOffsets)
{
	check(Linker);

	if (Linker->IsTextFormat())
	{
		ECB_LOG(Warning, TEXT("\tPackageReports are not currently supported for text based assets"));
		return;
	}

	// Display information about the package.
	FName LinkerName = Linker->LinkerRoot->GetFName();
	// Display summary info.
	ECB_LOG(Display, TEXT("********************************************"));
	ECB_LOG(Display, TEXT("Package '%s' Summary"), *LinkerName.ToString());
	ECB_LOG(Display, TEXT("--------------------------------------------"));

	ECB_LOG(Display, TEXT("\t         Filename: %s"), *Linker->GetPackagePath().GetLocalFullPath());
	ECB_LOG(Display, TEXT("\t     File Version: %i"), Linker->UEVer().ToValue());
	ECB_LOG(Display, TEXT("\t   Engine Version: %s"), *Linker->Summary.SavedByEngineVersion.ToString());
	ECB_LOG(Display, TEXT("\t   Compat Version: %s"), *Linker->Summary.CompatibleWithEngineVersion.ToString());
	ECB_LOG(Display, TEXT("\t     PackageFlags: %X"), Linker->Summary.GetPackageFlags());
	ECB_LOG(Display, TEXT("\t        NameCount: %d"), Linker->Summary.NameCount);
	ECB_LOG(Display, TEXT("\t       NameOffset: %d"), Linker->Summary.NameOffset);
	ECB_LOG(Display, TEXT("\t      ImportCount: %d"), Linker->Summary.ImportCount);
	ECB_LOG(Display, TEXT("\t     ImportOffset: %d"), Linker->Summary.ImportOffset);
	ECB_LOG(Display, TEXT("\t      ExportCount: %d"), Linker->Summary.ExportCount);
	ECB_LOG(Display, TEXT("\t     ExportOffset: %d"), Linker->Summary.ExportOffset);
	ECB_LOG(Display, TEXT("\tCompression Flags: %X"), Linker->Summary.CompressionFlags);
	ECB_LOG(Display, TEXT("\t  Custom Versions:\n%s"), *Linker->Summary.GetCustomVersionContainer().ToString("\t\t"));

	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ECB_LOG(Display, TEXT("\t             Guid: %s"), *Linker->Summary.Guid.ToString());
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	ECB_LOG(Display, TEXT("\t   PersistentGuid: %s"), *Linker->Summary.PersistentGuid.ToString());
	ECB_LOG(Display, TEXT("\t      Generations:"));
	for (int32 i = 0; i < Linker->Summary.Generations.Num(); ++i)
	{
		const FGenerationInfo& generationInfo = Linker->Summary.Generations[i];
		ECB_LOG(Display, TEXT("\t\t\t%d) ExportCount=%d, NameCount=%d "), i, generationInfo.ExportCount, generationInfo.NameCount);
	}

	if ((InfoFlags & PKGINFO_Names) != 0)
	{
		ECB_LOG(Display, TEXT("--------------------------------------------"));
		ECB_LOG(Display, TEXT("Name Map"));
		ECB_LOG(Display, TEXT("========"));
		for (int32 i = 0; i < Linker->NameMap.Num(); ++i)
		{
			FName name = FName::CreateFromDisplayId(Linker->NameMap[i], 0);
			ECB_LOG(Display, TEXT("\t%d: Name '%s' [Internal: %s, %d]"), i, *name.ToString(), *name.GetPlainNameString(), name.GetNumber());
			//ECB_LOG(Display, TEXT("\t%d: Name '%s' Comparison Index %d Display Index %d [Internal: %s, %d]"), i, *name.ToString(), name.GetComparisonIndex().ToUnstableInt(), name.GetDisplayIndex().ToUnstableInt(), *name.GetPlainNameString(), name.GetNumber());
		}
	}

	// if we _only_ want name info, skip this part completely
	if (InfoFlags != PKGINFO_Names)
	{
		if ((InfoFlags & PKGINFO_Imports) != 0)
		{
			ECB_LOG(Display, TEXT("--------------------------------------------"));
			ECB_LOG(Display, TEXT("Import Map"));
			ECB_LOG(Display, TEXT("=========="));
		}

		TArray<FName> DependentPackages;
		for (int32 i = 0; i < Linker->ImportMap.Num(); ++i)
		{
			FObjectImport& import = Linker->ImportMap[i];

			FName PackageName = NAME_None;
			FName OuterName = NAME_None;
			if (!import.OuterIndex.IsNull())
			{
				if ((InfoFlags & PKGINFO_Paths) != 0)
				{
					OuterName = *Linker->GetPathName(import.OuterIndex);
				}
				else
				{
					OuterName = Linker->ImpExp(import.OuterIndex).ObjectName;
				}

				// Find the package which contains this import.  import.SourceLinker is cleared in EndLoad, so we'll need to do this manually now.
				FPackageIndex OutermostLinkerIndex = import.OuterIndex;
				for (FPackageIndex LinkerIndex = import.OuterIndex; !LinkerIndex.IsNull(); )
				{
					OutermostLinkerIndex = LinkerIndex;
					LinkerIndex = Linker->ImpExp(LinkerIndex).OuterIndex;
				}
				check(!OutermostLinkerIndex.IsNull());
				PackageName = Linker->ImpExp(OutermostLinkerIndex).ObjectName;
			}

			if ((InfoFlags & PKGINFO_Imports) != 0)
			{
				ECB_LOG(Display, TEXT("\t*************************"));
				ECB_LOG(Display, TEXT("\tImport %d: '%s'"), i, *import.ObjectName.ToString());
				ECB_LOG(Display, TEXT("\t\t       Outer: '%s' (%d)"), *OuterName.ToString(), import.OuterIndex.ForDebugging());
				ECB_LOG(Display, TEXT("\t\t     Package: '%s'"), *PackageName.ToString());
				ECB_LOG(Display, TEXT("\t\t       Class: '%s'"), *import.ClassName.ToString());
				ECB_LOG(Display, TEXT("\t\tClassPackage: '%s'"), *import.ClassPackage.ToString());
				ECB_LOG(Display, TEXT("\t\t     XObject: %s"), import.XObject ? TEXT("VALID") : TEXT("NULL"));
				ECB_LOG(Display, TEXT("\t\t SourceIndex: %d"), import.SourceIndex);

				// dump depends info
				if ((InfoFlags & PKGINFO_Depends) != 0)
				{
					ECB_LOG(Display, TEXT("\t\t  All Depends:"));

					TSet<FDependencyRef> AllDepends;
					Linker->GatherImportDependencies(i, AllDepends);
					int32 DependsIndex = 0;
					for (TSet<FDependencyRef>::TConstIterator It(AllDepends); It; ++It)
					{
						const FDependencyRef& Ref = *It;
						if (Ref.Linker)
						{
							ECB_LOG(Display, TEXT("\t\t\t%i) %s"), DependsIndex++, *Ref.Linker->GetExportFullName(Ref.ExportIndex));
						}
						else
						{
							ECB_LOG(Display, TEXT("\t\t\t%i) NULL"), DependsIndex++);
						}
					}
				}
			}

			if (PackageName == NAME_None && import.ClassName == NAME_Package)
			{
				PackageName = import.ObjectName;
			}

			if (PackageName != NAME_None && PackageName != LinkerName)
			{
				DependentPackages.AddUnique(PackageName);
			}

			if (import.ClassPackage != NAME_None && import.ClassPackage != LinkerName)
			{
				DependentPackages.AddUnique(import.ClassPackage);
			}
		}

		if (DependentPackages.Num())
		{
			ECB_LOG(Display, TEXT("--------------------------------------------"));
			ECB_LOG(Display, TEXT("\tPackages referenced by %s:"), *LinkerName.ToString());
			for (int32 i = 0; i < DependentPackages.Num(); i++)
			{
				ECB_LOG(Display, TEXT("\t\t%i) %s"), i, *DependentPackages[i].ToString());
			}
		}
	}

	if ((InfoFlags & PKGINFO_Exports) != 0)
	{
		ECB_LOG(Display, TEXT("--------------------------------------------"));
		ECB_LOG(Display, TEXT("Export Map"));
		ECB_LOG(Display, TEXT("=========="));

		TArray<FExportInfo> SortedExportMap;
		SortedExportMap.Empty(Linker->ExportMap.Num());
		for (int32 i = 0; i < Linker->ExportMap.Num(); ++i)
		{
			new(SortedExportMap) FExportInfo(Linker, i);
		}

		FString SortingParms;
		if (FParse::Value(FCommandLine::Get(), TEXT("SORT="), SortingParms))
		{
			TArray<FString> SortValues;
			SortingParms.ParseIntoArray(SortValues, TEXT(","), true);

			for (int32 i = 0; i < EXPORTSORT_MAX; i++)
			{
				if (i < SortValues.Num())
				{
					const FString Value = SortValues[i];
					if (Value == TEXT("index"))
					{
						FObjectExport_Sorter::SortPriority[i] = EXPORTSORT_ExportIndex;
					}
					else if (Value == TEXT("size"))
					{
						FObjectExport_Sorter::SortPriority[i] = EXPORTSORT_ExportSize;
					}
					else if (Value == TEXT("name"))
					{
						FObjectExport_Sorter::SortPriority[i] = EXPORTSORT_ObjectPathname;
					}
					else if (Value == TEXT("outer"))
					{
						FObjectExport_Sorter::SortPriority[i] = EXPORTSORT_OuterPathname;
					}
				}
				else
				{
					FObjectExport_Sorter::SortPriority[i] = EXPORTSORT_MAX;
				}
			}
		}

		SortedExportMap.Sort(FObjectExport_Sorter());

		if ((InfoFlags & PKGINFO_Compact) == 0)
		{
			for (const auto& ExportInfo : SortedExportMap)
			{
				ECB_LOG(Display, TEXT("\t*************************"));
				const FObjectExport& Export = ExportInfo.Export;

				ECB_LOG(Display, TEXT("\tExport %d: '%s'"), ExportInfo.ExportIndex, *Export.ObjectName.ToString());

				// find the name of this object's class
				FPackageIndex ClassIndex = Export.ClassIndex;
				FName ClassName = ClassIndex.IsNull() ? FName(NAME_Class) : Linker->ImpExp(ClassIndex).ObjectName;

				// find the name of this object's parent...for UClasses, this will be the parent class
				// for UFunctions, this will be the SuperFunction, if it exists, etc.
				FString ParentName;
				if (!Export.SuperIndex.IsNull())
				{
					if ((InfoFlags & PKGINFO_Paths) != 0)
					{
						ParentName = *Linker->GetPathName(Export.SuperIndex);
					}
					else
					{
						ParentName = Linker->ImpExp(Export.SuperIndex).ObjectName.ToString();
					}
				}

				// find the name of this object's parent...for UClasses, this will be the parent class
				// for UFunctions, this will be the SuperFunction, if it exists, etc.
				FString TemplateName;
				if (!Export.TemplateIndex.IsNull())
				{
					if ((InfoFlags & PKGINFO_Paths) != 0)
					{
						TemplateName = *Linker->GetPathName(Export.TemplateIndex);
					}
					else
					{
						TemplateName = Linker->ImpExp(Export.TemplateIndex).ObjectName.ToString();
					}
				}

				// find the name of this object's Outer.  For UClasses, this will generally be the
				// top-level package itself.  For properties, a UClass, etc.
				FString OuterName;
				if (!Export.OuterIndex.IsNull())
				{
					if ((InfoFlags & PKGINFO_Paths) != 0)
					{
						OuterName = *Linker->GetPathName(Export.OuterIndex);
					}
					else
					{
						OuterName = Linker->ImpExp(Export.OuterIndex).ObjectName.ToString();
					}
				}

				ECB_LOG(Display, TEXT("\t\t         Class: '%s' (%i)"), *ClassName.ToString(), ClassIndex.ForDebugging());
				ECB_LOG(Display, TEXT("\t\t        Parent: '%s' (%d)"), *ParentName, Export.SuperIndex.ForDebugging());
				ECB_LOG(Display, TEXT("\t\t      Template: '%s' (%d)"), *TemplateName, Export.TemplateIndex.ForDebugging());
				ECB_LOG(Display, TEXT("\t\t         Outer: '%s' (%d)"), *OuterName, Export.OuterIndex.ForDebugging());
				ECB_LOG(Display, TEXT("\t\t   ObjectFlags: 0x%08X"), (uint32)Export.ObjectFlags);
				ECB_LOG(Display, TEXT("\t\t          Size: %d"), Export.SerialSize);
				if (!bHideOffsets)
				{
					ECB_LOG(Display, TEXT("\t\t      Offset: %d"), Export.SerialOffset);
				}
				ECB_LOG(Display, TEXT("\t\t       Object: %s"), Export.Object ? TEXT("VALID") : TEXT("NULL"));
				if (!bHideOffsets)
				{
					ECB_LOG(Display, TEXT("\t\t    HashNext: %d"), Export.HashNext);
				}
				ECB_LOG(Display, TEXT("\t\t   bNotForClient: %d"), Export.bNotForClient);
				ECB_LOG(Display, TEXT("\t\t   bNotForServer: %d"), Export.bNotForServer);

				// dump depends info
				if (InfoFlags & PKGINFO_Depends)
				{
					if (ExportInfo.ExportIndex < Linker->DependsMap.Num())
					{
						TArray<FPackageIndex>& Depends = Linker->DependsMap[ExportInfo.ExportIndex];
						ECB_LOG(Display, TEXT("\t\t  DependsMap:"));

						for (int32 DependsIndex = 0; DependsIndex < Depends.Num(); DependsIndex++)
						{
							ECB_LOG(Display, TEXT("\t\t\t%i) %s (%i)"),
								DependsIndex,
								*Linker->GetFullImpExpName(Depends[DependsIndex]),
								Depends[DependsIndex].ForDebugging()
							);
						}

						TSet<FDependencyRef> AllDepends;
						Linker->GatherExportDependencies(ExportInfo.ExportIndex, AllDepends);
						ECB_LOG(Display, TEXT("\t\t  All Depends:"));
						int32 DependsIndex = 0;
						for (TSet<FDependencyRef>::TConstIterator It(AllDepends); It; ++It)
						{
							const FDependencyRef& Ref = *It;
							if (Ref.Linker)
							{
								ECB_LOG(Display, TEXT("\t\t\t%i) %s (%i)"),
									DependsIndex++,
									*Ref.Linker->GetExportFullName(Ref.ExportIndex),
									Ref.ExportIndex);
							}
							else
							{
								ECB_LOG(Display, TEXT("\t\t\t%i) NULL (%i)"),
									DependsIndex++,
									Ref.ExportIndex);
							}
						}
					}
				}
			}
		}
		else
		{
			for (const auto& ExportInfo : SortedExportMap)
			{
				const FObjectExport& Export = ExportInfo.Export;
				ECB_LOG(Display, TEXT("  %8i %10i %32s %s"), ExportInfo.ExportIndex, Export.SerialSize,
					*(Linker->GetExportClassName(ExportInfo.ExportIndex).ToString()),
					(InfoFlags & PKGINFO_Paths) != 0 ? *Linker->GetExportPathName(ExportInfo.ExportIndex) : *Export.ObjectName.ToString());
			}
		}
	}

	if ((InfoFlags & PKGINFO_Text) != 0)
	{
		ECB_LOG(Display, TEXT("--------------------------------------------"));
		ECB_LOG(Display, TEXT("Gatherable Text Data Map"));
		ECB_LOG(Display, TEXT("=========="));

		if (Linker->SerializeGatherableTextDataMap(true))
		{
			ECB_LOG(Display, TEXT("Number of Text Data Entries: %d"), Linker->GatherableTextDataMap.Num());

			for (int32 i = 0; i < Linker->GatherableTextDataMap.Num(); ++i)
			{
				const FGatherableTextData& GatherableTextData = Linker->GatherableTextDataMap[i];
				ECB_LOG(Display, TEXT("Entry %d:"), 1 + i);
				ECB_LOG(Display, TEXT("\t   String: %s"), *GatherableTextData.SourceData.SourceString.ReplaceCharWithEscapedChar());
				ECB_LOG(Display, TEXT("\tNamespace: %s"), *GatherableTextData.NamespaceName);
				ECB_LOG(Display, TEXT("\t   Key(s): %d"), GatherableTextData.SourceSiteContexts.Num());
				for (const FTextSourceSiteContext& TextSourceSiteContext : GatherableTextData.SourceSiteContexts)
				{
					ECB_LOG(Display, TEXT("\t\t%s from %s"), *TextSourceSiteContext.KeyName, *TextSourceSiteContext.SiteDescription);
				}
			}
		}
		else
		{
			if (Linker->Summary.GatherableTextDataOffset > 0)
			{
				ECB_LOG(Warning, TEXT("Failed to load gatherable text data for package %s!"), *LinkerName.ToString());
			}
		}
	}


	if ((InfoFlags & PKGINFO_Thumbs) != 0)
	{
		ECB_LOG(Display, TEXT("--------------------------------------------"));
		ECB_LOG(Display, TEXT("Thumbnail Data"));
		ECB_LOG(Display, TEXT("=========="));

		if (Linker->SerializeThumbnails(true))
		{
			if (Linker->LinkerRoot->HasThumbnailMap())
			{
				FThumbnailMap& LinkerThumbnails = Linker->LinkerRoot->AccessThumbnailMap();

				int32 MaxObjectNameSize = 0;
				for (TMap<FName, FObjectThumbnail>::TIterator It(LinkerThumbnails); It; ++It)
				{
					FName& ObjectPathName = It.Key();
					MaxObjectNameSize = FMath::Max(MaxObjectNameSize, ObjectPathName.ToString().Len());
				}

				int32 ThumbIdx = 0;
				for (TMap<FName, FObjectThumbnail>::TIterator It(LinkerThumbnails); It; ++It)
				{
					FName& ObjectFullName = It.Key();
					FObjectThumbnail& Thumb = It.Value();

					ECB_LOG(Display, TEXT("\t\t%i) %*s: %ix%i\t\tImage Data:%i bytes"), ThumbIdx++, MaxObjectNameSize, *ObjectFullName.ToString(), Thumb.GetImageWidth(), Thumb.GetImageHeight(), Thumb.GetCompressedDataSize());
				}
			}
			else
			{
				ECB_LOG(Warning, TEXT("%s has no thumbnail map!"), *LinkerName.ToString());
			}
		}
		else
		{
			if (Linker->Summary.ThumbnailTableOffset > 0)
			{
				ECB_LOG(Warning, TEXT("Failed to load thumbnails for package %s!"), *LinkerName.ToString());
			}
		}
	}

	if ((InfoFlags & PKGINFO_Lazy) != 0)
	{
		ECB_LOG(Display, TEXT("--------------------------------------------"));
		ECB_LOG(Display, TEXT("Lazy Pointer Data"));
		ECB_LOG(Display, TEXT("==============="));
	}

	if ((InfoFlags & PKGINFO_AssetRegistry) != 0)
	{
		ECB_LOG(Display, TEXT("--------------------------------------------"));

		{
			const int32 NextOffset = Linker->Summary.WorldTileInfoDataOffset ? Linker->Summary.WorldTileInfoDataOffset : Linker->Summary.TotalHeaderSize;
			const int32 AssetRegistrySize = NextOffset - Linker->Summary.AssetRegistryDataOffset;
			ECB_LOG(Display, TEXT("Asset Registry Size: %10i"), AssetRegistrySize);
		}

		ECB_LOG(Display, TEXT("Asset Registry Data"));
		ECB_LOG(Display, TEXT("=========="));

		if (Linker->Summary.AssetRegistryDataOffset > 0)
		{
			// Seek to the AssetRegistry table of contents
			Linker->GetLoader_Unsafe()->Seek(Linker->Summary.AssetRegistryDataOffset);
			TArray<FAssetData*> AssetDatas;
			UE::AssetRegistry::EReadPackageDataMainErrorCode ErrorCode;
			int64 DependencyDataOffset;
			UE::AssetRegistry::ReadPackageDataMain(*Linker->GetLoader_Unsafe(), LinkerName.ToString(), Linker->Summary, DependencyDataOffset, AssetDatas, ErrorCode);

			ECB_LOG(Display, TEXT("Number of assets with Asset Registry data: %d"), AssetDatas.Num());

			// If there are any Asset Registry tags, print them
			int AssetIdx = 0;
			for (FAssetData* AssetData : AssetDatas)
			{
				// Display the asset class and path
				ECB_LOG(Display, TEXT("\t\t%d) %s'%s' (%d Tags)"), AssetIdx++, *AssetData->AssetClassPath.ToString(), *AssetData->GetSoftObjectPath().ToString(), AssetData->TagsAndValues.Num());

				// Display all tags on the asset
				for (const TPair<FName, FAssetTagValueRef>& Pair : AssetData->TagsAndValues)
				{
					ECB_LOG(Display, TEXT("\t\t\t\"%s\": \"%s\""), *Pair.Key.ToString(), *Pair.Value.AsString());
				}

				delete AssetData;
			}
		}
	}
}

#if ECB_LEGACY
bool FExtPackageUtilities::LoadThumbnailsFromPackage(const FString& InPackageFileName, const TSet< FName >& InObjectFullNames, FThumbnailMap& InOutThumbnails)
{
	// Create a file reader to load the file
	TUniquePtr< FArchive > FileReader(IFileManager::Get().CreateFileReader(*InPackageFileName));
	if (FileReader == nullptr)
	{
		// Couldn't open the file
		return false;
	}

	// Read package file summary from the file
	FPackageFileSummary FileSummary;
	(*FileReader) << FileSummary;

	// Make sure this is indeed a package
	if (FileSummary.Tag != PACKAGE_FILE_TAG || FileReader->IsError())
	{
		// Unrecognized or malformed package file
		return false;
	}


	// Does the package contains a thumbnail table?
	if (FileSummary.ThumbnailTableOffset == 0)
	{
		// No thumbnails to be loaded
		return false;
	}


	// Seek the the part of the file where the thumbnail table lives
	FileReader->Seek(FileSummary.ThumbnailTableOffset);

	int32 LastFileOffset = -1;
	// Load the thumbnail table of contents
	TMap< FName, int32 > ObjectNameToFileOffsetMap;
	{
		// Load the thumbnail count
		int32 ThumbnailCount = 0;
		*FileReader << ThumbnailCount;

		// Load the names and file offsets for the thumbnails in this package
		for (int32 CurThumbnailIndex = 0; CurThumbnailIndex < ThumbnailCount; ++CurThumbnailIndex)
		{
			bool bHaveValidClassName = false;
			FString ObjectClassName;
			*FileReader << ObjectClassName;

			// Object path
			FString ObjectPathWithoutPackageName;
			*FileReader << ObjectPathWithoutPackageName;

			FString ObjectPath;

			// handle UPackage thumbnails differently from usual assets
			if (ObjectClassName == UPackage::StaticClass()->GetName())
			{
				ObjectPath = ObjectPathWithoutPackageName;
			}
			else
			{
				//				ObjectPath = (FPackageName::FilenameToLongPackageName(InPackageFileName) + TEXT(".") + ObjectPathWithoutPackageName);

				ObjectPath = FPaths::Combine(TEXT("/Temp"), *FPaths::GetPath(InPackageFileName.Mid(InPackageFileName.Find(TEXT(":"), ESearchCase::CaseSensitive) + 1)), *FPaths::GetBaseFilename(InPackageFileName));

			}

			// If the thumbnail was stored with a missing class name ("???") when we'll catch that here
			if (ObjectClassName.Len() > 0 && ObjectClassName != TEXT("???"))
			{
				bHaveValidClassName = true;
			}
			else
			{
				// Class name isn't valid.  Probably legacy data.  We'll try to fix it up below.
			}


			if (!bHaveValidClassName)
			{
				// Try to figure out a class name based on input assets.  This should really only be needed
				// for packages saved by older versions of the editor (VER_CONTENT_BROWSER_FULL_NAMES)
				for (TSet<FName>::TConstIterator It(InObjectFullNames); It; ++It)
				{
					const FName& CurObjectFullNameFName = *It;

					FString CurObjectFullName;
					CurObjectFullNameFName.ToString(CurObjectFullName);

					if (CurObjectFullName.EndsWith(ObjectPath))
					{
						// Great, we found a path that matches -- we just need to add that class name
						const int32 FirstSpaceIndex = CurObjectFullName.Find(TEXT(" "));
						check(FirstSpaceIndex != -1);
						ObjectClassName = CurObjectFullName.Left(FirstSpaceIndex);

						// We have a useful class name now!
						bHaveValidClassName = true;
						break;
					}
				}
			}


			// File offset to image data
			int32 FileOffset = 0;
			*FileReader << FileOffset;

			if (FileOffset != -1 && FileOffset < LastFileOffset)
			{
				ECB_LOG(Warning, TEXT("Loaded thumbnail '%s' out of order!: FileOffset:%i    LastFileOffset:%i"), *ObjectPath, FileOffset, LastFileOffset);
			}


			if (bHaveValidClassName)
			{
				// Create a full name string with the object's class and fully qualified path
				const FString ObjectFullName(ObjectClassName + TEXT(" ") + ObjectPath);


				// Add to our map
				ObjectNameToFileOffsetMap.Add(FName(*ObjectFullName), FileOffset);
			}
			else
			{
				// Oh well, we weren't able to fix the class name up.  We won't bother making this
				// thumbnail available to load
			}
		}
	}


	// @todo CB: Should sort the thumbnails to load by file offset to reduce seeks [reviewed; pre-qa release]
	for (TSet<FName>::TConstIterator It(InObjectFullNames); It; ++It)
	{
		const FName& CurObjectFullName = *It;

		// Do we have this thumbnail in the file?
		// @todo thumbnails: Backwards compat
		const int32* pFileOffset = ObjectNameToFileOffsetMap.Find(CurObjectFullName);
		if (pFileOffset != NULL)
		{
			// Seek to the location in the file with the image data
			FileReader->Seek(*pFileOffset);

			// Load the image data
			FObjectThumbnail LoadedThumbnail;
			LoadedThumbnail.Serialize(*FileReader);

			// Store the data!
			InOutThumbnails.Add(CurObjectFullName, LoadedThumbnail);
		}
		else
		{
			// Couldn't find the requested thumbnail in the file!
		}
	}


	return true;
}

bool FExtPackageUtilities::ConditionallyLoadThumbnailsFromPackage(const FString& InPackageFileName, const TSet< FName >& InObjectFullNames, FThumbnailMap& InOutThumbnails)
{
	// First check to see if any of the requested thumbnails are already in memory
	TSet< FName > ObjectFullNamesToLoad;
	ObjectFullNamesToLoad.Empty(InObjectFullNames.Num());
#if ECB_LEGACY
	for (TSet<FName>::TConstIterator It(InObjectFullNames); It; ++It)
	{
		const FName& CurObjectFullName = *It;

		// Do we have this thumbnail in our cache already?
		// @todo thumbnails: Backwards compat
		const FObjectThumbnail* FoundThumbnail = FindCachedThumbnailInPackage(InPackageFileName, CurObjectFullName);
		if (FoundThumbnail != NULL)
		{
			// Great, we already have this thumbnail in memory!  Copy it to our output map.
			InOutThumbnails.Add(CurObjectFullName, *FoundThumbnail);
		}
		else
		{
			ObjectFullNamesToLoad.Add(CurObjectFullName);
		}
	}


	// Did we find all of the requested thumbnails in our cache?
	if (ObjectFullNamesToLoad.Num() == 0)
	{
		// Done!
		return true;
	}
#endif
	// OK, go ahead and load the remaining thumbnails!
	return LoadThumbnailsFromPackage(InPackageFileName, ObjectFullNamesToLoad, InOutThumbnails);
}
#endif

// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtAssetData.h"
#include "ExtContentBrowser.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtPackageUtils.h"
#include "ExtContentBrowserUtils.h"
#include "SExtContentBrowser.h"
#include "SContentBrowserPathPicker.h"

#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "AssetRegistry/AssetDataTagMap.h"
#include "Engine/AssetManager.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "AssetRegistry/ARFilter.h"
#include "ObjectTools.h"
#include "Blueprint/BlueprintSupport.h"
#include "Framework/Application/SlateApplication.h"
#include "ContentBrowserModule.h"
#include "UObject/UObjectIterator.h"
#include "HAL/RunnableThread.h"
#include "PackageTools.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "UObject/CoreRedirects.h"
#include "Misc/RedirectCollector.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Engine/PrimaryAssetLabel.h"
#include "Async/Async.h"
#include "Interfaces/IMainFrameModule.h"
#include "CollectionManagerTypes.h"
#include "CollectionManagerModule.h"
#include "ICollectionManager.h"
#include "DesktopPlatformModule.h"
#include "FileUtilities/ZipArchiveWriter.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "CollectionManagerModule.h"

#include "InstancedReferenceSubobjectHelper.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/MetaData.h"

#include "IContentBrowserDataModule.h"

// Serialization
#include "AssetRegistry/AssetDataTagMapSerializationDetails.h"
#include "Serialization/LargeMemoryWriter.h"
#include "Serialization/ArchiveProxy.h"
#include "UObject/NameBatchSerialization.h"

#if ECB_WIP_IMPORT_FOLDER_COLOR_OVERRIDE
#include "AssetViewUtils.h"
#endif

// Drag and Drop
#include "Input/DragAndDrop.h"
#include "LevelEditorViewport.h"


#define LOCTEXT_NAMESPACE "ExtContentBrowser"

const FGuid FExtAssetRegistryVersion::GUID(0xDC449DF5, 0x817F4F19, 0x899A33C9, 0x0AB230B4);
FCustomVersionRegistration GRegisterExtAssetRegistryCustomVersion(FExtAssetRegistryVersion::GUID, FExtAssetRegistryVersion::LatestVersion, TEXT("ExtAssetRegistryVer"));

FString FExtAssetSupport::AssetPackageExtension = TEXT(".uasset");
FString FExtAssetSupport::MapPackageExtension = TEXT(".umap");
FString FExtAssetSupport::TextAssetPackageExtension = TEXT(".utxt");
FString FExtAssetSupport::TextMapPackageExtension = TEXT(".utxtmap");

namespace FPathsUtil
{
	FString WithEndSlash(const FString& InPath)
	{
		FString PathWithEndSlash(InPath);
		FPaths::NormalizeDirectoryName(PathWithEndSlash);
		PathWithEndSlash.Append(TEXT("/"));

		return PathWithEndSlash;
	}

	bool IsSubOrSamePath(const FString& InSubPath, const FString& InRootPath)
	{
		FString SubPath(InSubPath);
		FPaths::NormalizeDirectoryName(SubPath);

		FString RootPath(InRootPath);
		FPaths::NormalizeDirectoryName(RootPath);

		if (SubPath.Equals(RootPath))
		{
			return true;
		}

		RootPath.Append(TEXT("/"));

		if (SubPath.StartsWith(RootPath))
		{
			return true;
		}
		return false;
	}

	bool IsFileInDir(const FString& InFile, const FString& InRootPath)
	{
		FString RootPath = WithEndSlash(InRootPath);

		return (InFile.StartsWith(RootPath));
	}

	// return: num of paths been merged
	int32 SortAndMergeDirs(const TArray<FString>& InPathsToCombine, TArray<FString>& OutCombinedPaths)
	{
		TArray<FString> PathsToCombine = InPathsToCombine;
		PathsToCombine.Sort();
		const int32 NumPaths = PathsToCombine.Num();

		TArray<int32> IndicesToRemove;

		for (int32 IndexA = 0; IndexA < NumPaths - 1; ++IndexA)
		{
			FString& A = PathsToCombine[IndexA];

			for (int32 IndexB = NumPaths - 1; IndexB > IndexA; --IndexB)
			{
				if (IndicesToRemove.Contains(IndexB))
				{
					continue;
				}

				FString& B = PathsToCombine[IndexB];

				//if (B.StartsWith(A, ESearchCase::IgnoreCase)) // todo: Mac -> Casesensitive
				if (FPathsUtil::IsSubOrSamePath(B, A))
				{
					IndicesToRemove.Add(IndexB);
				}
			}
		}

		if (IndicesToRemove.Num() > 0)
		{
			for (int32 Index = 0; Index < IndicesToRemove.Num(); ++Index)
			{
				PathsToCombine.RemoveAt(IndicesToRemove[Index]);
			}
		}

		OutCombinedPaths = PathsToCombine;
		return IndicesToRemove.Num();
	}
}

namespace FExtAssetCoreUtil
{
	bool FExtAssetFeedbackContext::ReceivedUserCancel()
	{
		if (!bTaskWasCancelledCache)
		{
			bTaskWasCancelledCache = FExtFeedbackContextEditor::ReceivedUserCancel();
		}
		return bTaskWasCancelledCache;
	}

	///////////////////////////////////////////

	FExtAssetWorkReporter::FExtAssetWorkReporter(const TSharedPtr<IExtAssetProgressReporter>& InReporter, const FText& InDescription, float InAmountOfWork, float InIncrementOfWork, bool bInterruptible)
		: Reporter(InReporter)
		, DefaultIncrementOfWork(InIncrementOfWork)
	{
		if (Reporter.IsValid())
		{
			Reporter->BeginWork(InDescription, InAmountOfWork, bInterruptible);
		}
	}

	FExtAssetWorkReporter::~FExtAssetWorkReporter()
	{
		if (Reporter.IsValid())
		{
			Reporter->EndWork();
		}
	}

	void FExtAssetWorkReporter::ReportNextStep(const FText& InMessage, float InIncrementOfWork)
	{
		if (Reporter.IsValid())
		{
			Reporter->ReportProgress(InIncrementOfWork, InMessage);
		}
	}

	void FExtAssetWorkReporter::ReportNextStep(const FText& InMessage)
	{
		ReportNextStep(InMessage, DefaultIncrementOfWork);
	}

	bool FExtAssetWorkReporter::IsWorkCancelled() const
	{
		if (Reporter.IsValid())
		{
			return Reporter->IsWorkCancelled();
		}
		return false;
	}

	///////////////////////////////////////////

	void FExtAssetProgressUIReporter::BeginWork(const FText& InTitle, float InAmountOfWork, bool bInterruptible/* = true*/)
	{
		ProgressTasks.Emplace(new FScopedSlowTask(InAmountOfWork, InTitle, true, FeedbackContext.IsValid() ? *FeedbackContext.Get() : *GWarn));
		ProgressTasks.Last()->MakeDialog(bInterruptible);
	}

	void FExtAssetProgressUIReporter::EndWork()
	{
		if (ProgressTasks.Num() > 0)
		{
			ProgressTasks.Pop();
		}
	}

	void FExtAssetProgressUIReporter::ReportProgress(float Progress, const FText& InMessage)
	{
		if (ProgressTasks.Num() > 0)
		{
			TSharedPtr<FScopedSlowTask>& ProgressTask = ProgressTasks.Last();
			ProgressTask->EnterProgressFrame(Progress, InMessage);
		}
	}

	bool FExtAssetProgressUIReporter::IsWorkCancelled()
	{
		if (!bIsCancelled && ProgressTasks.Num() > 0)
		{
			const TSharedPtr<FScopedSlowTask>& ProgressTask = ProgressTasks.Last();
			bIsCancelled |= ProgressTask->ShouldCancel();
		}
		return bIsCancelled;
	}

	FFeedbackContext* FExtAssetProgressUIReporter::GetFeedbackContext() const
	{
		return FeedbackContext.IsValid() ? FeedbackContext.Get() : GWarn;
	}

	///////////////////////////////////////////

	void FExtAssetProgressTextReporter::BeginWork(const FText& InTitle, float InAmountOfWork, bool bInterruptible/* = true*/)
	{
		ECB_LOG(Display, TEXT("Start: %s ..."), *InTitle.ToString());
		++TaskDepth;
	}

	void FExtAssetProgressTextReporter::EndWork()
	{
		if (TaskDepth > 0)
		{
			--TaskDepth;
		}
	}

	void FExtAssetProgressTextReporter::ReportProgress(float Progress, const FText& InMessage)
	{
		if (TaskDepth > 0)
		{
			ECB_LOG(Display, TEXT("Doing %s ..."), *InMessage.ToString());
		}
	}

	bool FExtAssetProgressTextReporter::IsWorkCancelled()
	{
		return false;
	}

	FFeedbackContext* FExtAssetProgressTextReporter::GetFeedbackContext() const
	{
		return FeedbackContext.Get();
	}
}

////////////////////////////////////////////////////
// FExtConfigUtil
//

struct FExtConfigUtil
{
	static FString GetConfigDirByContentRoot(const FString& InAssetContentRoot)
	{
		static FString EmptyString("");

		FString ConfigDir = InAssetContentRoot / TEXT("../Saved/Config/") / ANSI_TO_TCHAR(FPlatformProperties::PlatformName());

		FPaths::CollapseRelativeDirectories(ConfigDir);

		if (IFileManager::Get().DirectoryExists(*ConfigDir))
		{
			return ConfigDir;
		}

		return EmptyString;
	}

	static bool LoadPathColors(const FString& InAssetContentRoot, const FString& InConfigDir, TMap<FName, FLinearColor>& OutPathColors, bool bRemapPackagePathToFilePath = true)
	{
		FString IniFile = InConfigDir / TEXT("EditorPerProjectUserSettings.ini");
		ECB_LOG(Display, TEXT("[LoadPathColors] in %s"), *IniFile);

		FConfigFile ConfigFile;
		if (FConfigCacheIni::LoadExternalIniFile(ConfigFile, TEXT("EditorPerProjectUserSettings"), TEXT(""), *InConfigDir, /*bIsBaseIniName*/false, /*Platform*/nullptr, /*bForceReload*/true, /*bWriteDestIni*/false))
		{
			if (const FConfigSection* Sec = ConfigFile.Find(TEXT("PathColor")))
			{
				ECB_LOG(Display, TEXT("  Found Sec in %s"), *IniFile);
				for (FConfigSectionMap::TConstIterator SecIt(*Sec); SecIt; ++SecIt)
				{
					ECB_LOG(Display, TEXT("  %s=%s"), *SecIt.Key().ToString(), *SecIt.Value().GetValue());

					const FString& PackageName = SecIt.Key().ToString();
					const FString& ColorString = SecIt.Value().GetValue();
					FLinearColor Color;
					if (Color.InitFromString(ColorString))
					{
						if (bRemapPackagePathToFilePath)
						{
							FString FolderPath;
							FString PackageRoot = FExtAssetDataUtil::GetPackageRootFromFullPackagePath(PackageName);
							if (FExtAssetDataUtil::RemapGamePackageToFullFilePath(PackageRoot, PackageName, InAssetContentRoot, FolderPath, /*bPackageIsFolder*/true))
							{
								ECB_LOG(Display, TEXT("  => %s=%s"), *FolderPath, *Color.ToString());
								OutPathColors.Add(*FolderPath, Color);
							}
						}
						else
						{
							OutPathColors.Add(*PackageName, Color);
						}						
					}
				}

				return true;
			}
		}
		return false;
	}
};

/////////////////////////////////////////////////////////////
// FExtAssetData implementation
//

FExtAssetData::FExtAssetData()
	: bParsed(false)
	, bValid(false)
	, bHasThumbnail(false)
	, bCompatible(false)
{
	InvalidReason = EInvalidReason::NotParsed;
	AssetContentType = EContentType::Orphan;
}

FExtAssetData::FExtAssetData(const FString& InPackageFilePath, bool bDelayParse/* = false*/)
	: PackageFilePath(*InPackageFilePath)
	, bParsed(false)
	, bValid(false)
	, bHasThumbnail(false)
	, bCompatible(false)
{
	InvalidReason = EInvalidReason::NotParsed;
	AssetContentType = EContentType::Orphan;

	if (PreParse() && !bDelayParse)
	{
		Parse();
	}
}

FExtAssetData::FExtAssetData(const UObject* InAsset, bool bAllowBlueprintClass)
{
#if ECB_TODO // todo: treat imported asset as external uasset package
	FString AssetPackageName = InAsset->GetOutermost()->GetFName();
	FString AssetFilePath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(AssetPackageName)));
	PackageFilePath = *AssetFilePath;
	Parse();
#endif
}


FExtAssetData::FExtAssetData(const FName& InPackageName)
	: InvalidReason(EInvalidReason::NotParsed)
{
	if (FExtAssetData* CachedAssetData = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetByPackageName(InPackageName))
	{
		*this = *CachedAssetData;
		return;
	}

	struct Local
	{
		static bool ConvertPackageNameToFilePathByAssetRegistry(const FName& InPackageName, FString& OutFullFilePath)
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			TArray<FAssetData> AssetDatas;
			AssetRegistry.GetAssetsByPackageName(*InPackageName.ToString(), AssetDatas, /*bIncludeOnlyOnDiskAssets*/true);
			if (AssetDatas.Num() > 0 && AssetDatas[0].IsValid())
			{
				FString FileExtension = FPackageName::GetAssetPackageExtension();

				if (AssetDatas[0].AssetClassPath.GetPackageName() == UWorld::StaticClass()->GetFName())
				{
					FileExtension = FPackageName::GetMapPackageExtension();
				}

				ECB_LOG(Display, TEXT("[ConvertTempPackageNameToFullFilePath] Packagename: %s, AssetDatas: %d, AssetClassPath: %s, Extension: %s"), *InPackageName.ToString(), AssetDatas.Num(), *AssetDatas[0].AssetClassPath.ToString(), *FileExtension);
				if (AssetDatas.Num() > 1)
				{
					ECB_LOG(Display, TEXT("   More than 1 asset data found. AssetDatas[1]: GetFullName: %s, AssetClassPath: %s"), *AssetDatas[1].GetFullName(), *AssetDatas[1].AssetClassPath.ToString());
				}

				FString RelativePackageFilePath;
				if (FPackageName::TryConvertLongPackageNameToFilename(AssetDatas[0].PackageName.ToString(), RelativePackageFilePath, FileExtension))
				{
					const FString AbsolutePackageFilePath = FPaths::ConvertRelativePathToFull(RelativePackageFilePath);
					OutFullFilePath = AbsolutePackageFilePath;

					ECB_LOG(Display, TEXT("   => FullFilePath: %s"), *AbsolutePackageFilePath);
					return true;
				}
			}
			return false;
		}
	};

	FString AbsolutePackageFilePath;
	if (Local::ConvertPackageNameToFilePathByAssetRegistry(InPackageName, AbsolutePackageFilePath))
	{
		FExtAssetData ExtAssetData(AbsolutePackageFilePath);
		*this = ExtAssetData;
		return;
	}

	PackageName = InPackageName;
	PackagePath = FName(*FPackageName::GetLongPackagePath(PackageName.ToString()));
}

bool FExtAssetData::PreParse()
{
	bValid = true;

	// Normalize path
	FString NormalizedFilePath = PackageFilePath.ToString();

	// PackageFilePath
	{
		FPaths::NormalizeFilename(NormalizedFilePath);
		PackageFilePath = FName(*NormalizedFilePath);

		if (PackageFilePath == NAME_None)
		{
			bValid = false;
			InvalidReason = EInvalidReason::FilePathEmptyOrInvalid;
		}
	}

	const FString& FilePath = NormalizedFilePath;

	// PackageName, PackagePath, AssetName, AssetClass
	if (bValid)
	{
		FString TempPackageName = ConvertFilePathToTempPackageName(FilePath);
		PackageName = FName(*TempPackageName);

		if (FPackageName::DoesPackageNameContainInvalidCharacters(TempPackageName))
		{
			bValid = false;
			InvalidReason = EInvalidReason::FilePathContainsInvalidCharacter;
		}

		PackagePath = FName(*FPackageName::GetLongPackagePath(PackageName.ToString()));

		// AssetName Fallback in case it's an invalid uasset, for searching and display purpose
		AssetName = *FPaths::GetBaseFilename(FilePath);

		// ObjectPath Fallback
		FString ObjectPathStr;
		PackageName.AppendString(ObjectPathStr);
		ObjectPathStr.AppendChar('.');
		AssetName.AppendString(ObjectPathStr);
		ObjectPath = FName(*ObjectPathStr);

		// AssetClass Fallback
		AssetClass = NAME_None;
	}

	// Other Fallback
	AssetContentRoot = NAME_None;
	AssetRelativePath = NAME_None;
	AssetContentType = EContentType::Unknown;

	FileVersionUE4 = 0;
	AssetCount = 0;
	ThumbCount = 0;
	FileSize = 0;
	bHasThumbnail = false;;
	bCompatible = false;

	if (!bValid)
	{
		ECB_LOG(Warning, TEXT("PreParse %s failed. Failed Reason: %s"), *PackageFilePath.ToString(), *GetInvalidReason());
		bParsed = true;
	}

	return bValid;
}

bool FExtAssetData::DoParse()
{
	bValid = true;

	const FString FilePath = PackageFilePath.ToString();

	// If disk file exist?
	{
		const bool bFileExist = IFileManager::Get().FileExists(*FilePath);
		if (!bFileExist)
		{
			bValid = false;
			InvalidReason = EInvalidReason::FileNotFound;
		}
	}
	
	// Parse AssetName, AssetClass, AssetRegistryTags, bHasThumbanil
	bValid = bValid && FExtPackageUtils::ParsePackageWithPackageReader(*this);
	if (bValid)
	{	
		if (AssetCount < 1)
		{
			ECB_LOG(Display, TEXT("No Asset Data Found: %s"), *FilePath);
			bValid = false;
			InvalidReason = EInvalidReason::NoAssetData;
		}

		else if (AssetName == NAME_None)
		{
			ECB_LOG(Display, TEXT("AssetName is empty: %s"), *FilePath);
			bValid = false;
			InvalidReason = EInvalidReason::AssetNameEmpty;
		}

		else if (AssetClass == NAME_None)
		{
			ECB_LOG(Display, TEXT("AssetClass is empty: %s"), *FilePath);
			bValid = false;
			InvalidReason = EInvalidReason::ClassNameEmpty;
		}

		// Redirector is not supported (for now)
		else if (AssetClass == UObjectRedirector::StaticClass()->GetFName())
		{
			ECB_LOG(Display, TEXT("Redirctor found: %s"), *FilePath);
#if !ECB_FEA_REDIRECTOR_SUPPORT
			bValid = false;
			InvalidReason = EInvalidReason::RedirectorNotSupported;
#endif
		}

		// MapBuildDataRegistry is not supported
#if ECB_DISABLE
		else if (AssetClass == UMapBuildDataRegistry::StaticClass()->GetFName()
			|| AssetClass == UPrimaryAssetLabel::StaticClass()->GetFName())
		{
			bValid = false;
			InvalidReason = EInvalidReason::GenericAssetTypeNotSupported;
		}
#endif

		// Class asset is not supported
		else if (AssetClass == NAME_Class)
		{
			ECB_LOG(Display, TEXT("Class asset found: %s"), *FilePath);
			bValid = false;
			InvalidReason = EInvalidReason::ClassAssetNotSupported;
		}
	}

	// ObjectPath
	if (bValid)
	{
		FString ObjectPathStr;
		PackageName.AppendString(ObjectPathStr);
		ObjectPathStr.AppendChar('.');
		AssetName.AppendString(ObjectPathStr);
		ObjectPath = FName(*ObjectPathStr);
	}

	// Compatibility
	if (bValid)
	{
		CheckIfCompatibleWithCurrentEngineVersion();
		if (!bCompatible)
		{
			bValid = false;
			InvalidReason = EInvalidReason::NotCompatiableWithCurrentEngineVersion;
		}
	}

	// Asset Content Root and Relative Path
	//if (bValid)
	{
		FString RelativePath;
		FString BaseDir;

		TSet<FName> AllDependencies;
#if ECB_WIP_IMPORT_ORPHAN
		{
			AllDependencies.Append(HardDependentPackages);
			AllDependencies.Append(SoftReferencesList);
		}
#endif

		if (FExtContentBrowserSingleton::GetAssetRegistry().ParseAssetContentRoot(FilePath, RelativePath, BaseDir, AssetContentType, &AllDependencies))
		{
			AssetContentRoot = *BaseDir;
			AssetRelativePath = *RelativePath;

			//AssetContentRootDirName = *FPaths::GetPathLeaf(BaseDir);
			//ECB_LOG(Display, TEXT("[Parse] %s relative to %s(%s)."), *RelativePath, *AssetContentRootDirName.ToString(), *BaseDir);
		}
	}

	if (!bValid)
	{
		ECB_LOG(Warning, TEXT("Parse %s failed. Failed Reason: %s"), *FilePath, *GetInvalidReason());
	}

	bParsed = true;; // treat invalid as not parsed (todo: re-parse when file changed)

	return  bValid;
}

bool FExtAssetData::Parse()
{
	return DoParse();
}

bool FExtAssetData::ReParse()
{
	if (PreParse())
	{
		return Parse();
	}
	return false;
}

bool FExtAssetData::HasThumbnail() const
{
	return bHasThumbnail;
}

bool FExtAssetData::HasContentRoot() const
{
	return AssetContentRoot != NAME_None && AssetRelativePath != NAME_None;
}

bool FExtAssetData::HasValidThumbnail() const
{
	FObjectThumbnail ObjectThumbnail;
	if (LoadThumbnail(ObjectThumbnail))
	{
		return ObjectThumbnail.GetImageWidth() > 0 && ObjectThumbnail.GetImageHeight() > 0 && ObjectThumbnail.GetCompressedDataSize() > 0;
	}

	return false;
}

bool FExtAssetData::CanImportFast() const
{
	return IsValid() && IsCompatibleWithCurrentEngineVersion() && HasContentRoot();
}

bool FExtAssetData::LoadThumbnail(FObjectThumbnail& OutThumbnail) const
{
	return FExtPackageUtils::LoadThumbnailWithPackageReader(*this, OutThumbnail);
}

#if ECB_LEGACY
void FExtAssetData::LoadSoftReferences(TArray<FName>& OutSoftReferences) const
{
	FExtPackageUtils::LoadSoftReferences(*this, OutSoftReferences);
}
#endif

FString FExtAssetData::GetSavedEngineVersionForDisplay() const
{
	return SavedByEngineVersion.ToString(EVersionComponent::Patch);
}

const FString& FExtAssetData::GetAssetContentTypeForDisplay() const
{
	static const FString Project(TEXT("Project"));
	static const FString VaultCache(TEXT("VaultCache"));
	static const FString Plugin(TEXT("Plugin"));
	static const FString PartialProject(TEXT("PartialProject"));
	static const FString Orphan(TEXT("Orphan"));
	static const FString Unknown(TEXT("Unknown"));
	static const FString Empty(TEXT(""));

	if (AssetContentType == EContentType::Project)
	{
		return Project;
	}
	else if (AssetContentType == EContentType::VaultCache)
	{
		return VaultCache;
	}
	else if (AssetContentType == EContentType::Plugin)
	{
		return Plugin;
	}
	else if (AssetContentType == EContentType::PartialProject)
	{
		return PartialProject;
	}
	else if (AssetContentType == EContentType::Orphan)
	{
		return Orphan;
	}
	else if (AssetContentType == EContentType::Unknown)
	{
		return Unknown;
	}
	return Empty;
}

UClass* FExtAssetData::GetIconClass(bool* bOutIsClassType) const
{
	if (bOutIsClassType)
	{
		*bOutIsClassType = false;
	}

	UClass* ResultAssetClass = FindObjectSafe<UClass>(nullptr, *AssetClass.ToString());
	if (!ResultAssetClass)
	{
		return nullptr;
	}

	if (ResultAssetClass == UClass::StaticClass())
	{
		if (bOutIsClassType)
		{
			*bOutIsClassType = true;
		}

		return FindObject<UClass>(nullptr, *AssetName.ToString());
	}

	if (ResultAssetClass->IsChildOf<UBlueprint>())
	{
		if (bOutIsClassType)
		{
			*bOutIsClassType = true;
		}

		// We need to use the asset data to get the parent class as the blueprint may not be loaded
		FString ParentClassName;
		if (!GetTagValue(FBlueprintTags::NativeParentClassPath, ParentClassName))
		{
			GetTagValue(FBlueprintTags::ParentClassPath, ParentClassName);
		}
		if (!ParentClassName.IsEmpty())
		{
			UObject* Outer = nullptr;
			ResolveName(Outer, ParentClassName, false, false);
			return FindObject<UClass>(nullptr, *ParentClassName);
		}
	}

	// Default to using the class for the asset type
	return ResultAssetClass;
}

FSoftObjectPath FExtAssetData::GetSoftObjectPath() const
{
	if (IsTopLevelAsset())
	{
		return FSoftObjectPath(PackageName, AssetName, FString());
	}
	else
	{
		TStringBuilder<FName::StringBufferSize> Builder;
		AppendObjectPath(Builder);
		return FSoftObjectPath(Builder.ToView());
	}
}

FString FExtAssetData::GetFolderPath() const
{
	return FPaths::GetPath(PackageFilePath.ToString());
}

FString FExtAssetData::ConvertFilePathToTempPackageName(const FString& InFilePath)
{
	FString FilePath = InFilePath;
	FPaths::NormalizeFilename(FilePath);
	FilePath.ReplaceInline(TEXT(":"), TEXT(""));
	while (FilePath.Find(TEXT("//")) != INDEX_NONE)
	{
		FilePath.ReplaceInline(TEXT("//"), TEXT("/"));
	}

	FString TempPackageName = FPaths::Combine(FExtAssetContants::ExtTempPackagePath, *FPaths::GetPath(FilePath), *FPaths::GetBaseFilename(FilePath));

	FString SanitizedPackageName = ObjectTools::SanitizeInvalidChars(TempPackageName, INVALID_LONGPACKAGE_CHARACTERS);
	
	return SanitizedPackageName;
}

FString& FExtAssetData::GetSandboxPackagePath()
{
	static FString SandboxPackagePath = FExtAssetContants::ExtSandboxPackagePath; // TEXT("/UAssetBrowser/Sandbox");
	return SandboxPackagePath;
}

FString& FExtAssetData::GetSandboxPackagePathWithSlash()
{
	static FString SandboxPackagePathWithSlash = FExtAssetContants::ExtSandboxPackagePath + TEXT("/");
	return SandboxPackagePathWithSlash;
}

FString& FExtAssetData::GetSandboxDir()
{
	const FString ProjectIntermediateDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir());
	static FString SandboxDir = FPaths::Combine(ProjectIntermediateDir, TEXT("UAssetBrowser"), TEXT("Sandbox"));
	return SandboxDir;
}

FString& FExtAssetData::GetUAssetBrowserTempDir()
{
	const FString ProjectIntermediateDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir());
	static FString TempDir = FPaths::Combine(ProjectIntermediateDir, TEXT("UAssetBrowser"), TEXT("Temp"));
	return TempDir;
}

FString& FExtAssetData::GetImportSessionTempDir()
{
	const FString ProjectIntermediateDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir());
	static FString SessionTempDir = FPaths::Combine(ProjectIntermediateDir, TEXT("UAssetBrowser"), TEXT("Import"), TEXT("Sessions"));
	return SessionTempDir;
}

FString FExtAssetData::GetZipExportSessionTempDir()
{
	const FString ProjectIntermediateDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir());
	const FString SessionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	FString SessionTempDir = FPaths::Combine(ProjectIntermediateDir, TEXT("UAssetBrowser"), TEXT("Export"), TEXT("Sessions"), SessionId);
	return SessionTempDir;
}

void FExtAssetData::CheckIfCompatibleWithCurrentEngineVersion()
{
	bCompatible = FileVersionUE4 <= GPackageFileUEVersion.ToValue() && FEngineVersion::Current().IsCompatibleWith(CompatibleWithEngineVersion);
}

bool FExtAssetData::IsCompatibleWithCurrentEngineVersion() const
{
	return bCompatible;
}

FString FExtAssetData::GetInvalidReason() const
{
	switch (InvalidReason)
	{
	case EInvalidReason::Valid:	return TEXT("Valid");

	case EInvalidReason::FileNotFound: return TEXT("File Not Found");
	case EInvalidReason::FilePathEmptyOrInvalid: return TEXT("Empty Or Invalid File Path");
	case EInvalidReason::FilePathContainsInvalidCharacter: return TEXT("File Path Contains Invalid Character");

	case EInvalidReason::FailedToLoadPackage: return TEXT("Failed To Load Package");

	case EInvalidReason::PackageFileVersionEmptyOrTooOld: return TEXT("File Version Empty Or Too Old");
	case EInvalidReason::PackageFileOrCustomVersionTooNew: return TEXT("File Version Too New");
	case EInvalidReason::PackageFileMalformed: return TEXT("File Malformed");
	case EInvalidReason::PackageFileCustomVersionMissing: return TEXT("File Custom Version Missing");
	case EInvalidReason::PackageFileFailedToLoad: return TEXT("File Failed ToLoad");


	case EInvalidReason::NoAssetData: return TEXT("AssetData Empty");
	case EInvalidReason::AssetNameEmpty: return TEXT("AssetName Empty");
	case EInvalidReason::ClassNameEmpty: return TEXT("ClassName Empty");

	case EInvalidReason::RedirectorNotSupported: return TEXT("Redirector Not Supported");
	case EInvalidReason::GenericAssetTypeNotSupported: return FString::Printf(TEXT("%s Not Supported"), *AssetClass.ToString());
	case EInvalidReason::TextForamtNotSupported: return TEXT("TextForamt Not Supported");
	case EInvalidReason::CookedPackageNotSupported: return TEXT("Cooked Package Not Supported");
	case EInvalidReason::NotCompatiableWithCurrentEngineVersion: return TEXT("Not Compatiable With Current Engine Version");
	default: return TEXT("Unknown invalid reason.");
	}
}

void FExtAssetData::PrintAssetData() const
{
	ECB_LOG(Display, TEXT("    FExtAssetData for %s"), *PackageFilePath.ToString());
	ECB_LOG(Display, TEXT("    ============================="));
	ECB_LOG(Display, TEXT("        FileVersionUE4:           %d"), FileVersionUE4);
	ECB_LOG(Display, TEXT("        SavedByEngineVersion:     %s"), *SavedByEngineVersion.ToString());
	ECB_LOG(Display, TEXT("        CompatibleEngineVersion:  %s"), *CompatibleWithEngineVersion.ToString());
	ECB_LOG(Display, TEXT("        PackageName:              %s"), *PackageName.ToString());
	ECB_LOG(Display, TEXT("        PackagePath:              %s"), *PackagePath.ToString());
	ECB_LOG(Display, TEXT("        ObjectPath:               %s"), *ObjectPath.ToString());
	ECB_LOG(Display, TEXT("        AssetName:                %s"), *AssetName.ToString());
	ECB_LOG(Display, TEXT("        AssetClass:               %s"), *AssetClass.ToString());
	ECB_LOG(Display, TEXT("        AssetContentRootDir:      %s"), *AssetContentRoot.ToString());
	ECB_LOG(Display, TEXT("        AssetRelativePath:        %s"), *AssetRelativePath.ToString());
	ECB_LOG(Display, TEXT("        bParsed:                  %d"), bParsed);
	ECB_LOG(Display, TEXT("        bValid:                   %d"), bValid);
	ECB_LOG(Display, TEXT("        bCompatible:              %d"), bCompatible);
	
	ECB_LOG(Display, TEXT("        TagsAndValues:            %d"), TagsAndValues.Num());
	ECB_LOG(Display, TEXT("        -----------------------------"));
	int32 Index = 0;
	for (const auto& TagValue : TagsAndValues)
	{
		ECB_LOG(Display, TEXT("            %d) %s : %s"), Index, *TagValue.Key.ToString(), *TagValue.Value.AsString());
		Index++;
	}

	ECB_LOG(Display, TEXT("        HardDependentPackages:    %d"), HardDependentPackages.Num());
	ECB_LOG(Display, TEXT("        -----------------------------"));
	Index = 0;
	for (const FName& HardDependent : HardDependentPackages)
	{
		ECB_LOG(Display, TEXT("                 %d) %s"), Index, *HardDependent.ToString());
		Index++;
	}

	ECB_LOG(Display, TEXT("        SoftReferencesList:       %d"), SoftReferencesList.Num());
	ECB_LOG(Display, TEXT("        -----------------------------"));
	Index = 0;
	for (const FName& SoftReference : SoftReferencesList)
	{
		ECB_LOG(Display, TEXT("                 %d) %s"), Index, *SoftReference.ToString());
		Index++;
	}
}

bool FExtAssetData::IsUAsset() const
{
	return PackageFilePath != NAME_None && PackageName != NAME_None && PackageName.ToString().StartsWith(FExtAssetContants::ExtTempPackagePath);
}

bool FExtAssetData::IsTopLevelAsset() const
{
#if WITH_EDITORONLY_DATA
	if (OptionalOuterPath.IsNone())
	{
		// If no outer path, then path is PackageName.AssetName so we must be top level
		return true;
	}

	TStringBuilder<FName::StringBufferSize> Builder;
	AppendObjectPath(Builder);

	int32 SubObjectIndex;
	FStringView(Builder).FindChar(SUBOBJECT_DELIMITER_CHAR, SubObjectIndex);
	return SubObjectIndex == INDEX_NONE;
#else
	// Non-top-level assets only appear in the editor
	return true;
#endif
}

bool FExtAssetData::IsUMap() const
{
	return PackageFilePath != NAME_None && FPaths::GetExtension(PackageFilePath.ToString(), /*IncludeDot*/ true).Equals(FExtAssetSupport::MapPackageExtension, ESearchCase::IgnoreCase);
}

template <typename ValueType>
bool FExtAssetData::GetTagValue(const FName InTagName, ValueType& OutTagValue) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(InTagName);
	if (FoundValue.IsSet())
	{
		FMemory::Memzero(&OutTagValue, sizeof(ValueType));
		LexFromString(OutTagValue, *FoundValue.GetValue());
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// FExtAssetRegistry

FExtAssetRegistry::FExtAssetRegistry()
{
}

FExtAssetRegistry::~FExtAssetRegistry()
{
	Shutdown(); // Todo: should be called manually from module shutdown?
}


void FExtAssetRegistry::BroadcastAssetUpdatedEvent(const FExtAssetData& UpdatedAsset)
{
	AssetUpdatedEvent.Broadcast(UpdatedAsset);
}

bool FExtAssetRegistry::GetAndTrimFolderGatherResult()
{
	const bool bRefreshPathViewWhileScan = true; //todo: move to options maybe?

	bool bRootDirGathered = false;
	bool bSubDirGathered = false;

	if (BackgroundFolderGatherer.IsValid())
	{
		TArray<FExtFolderGatherResult> RootDirGatherResults;
		double RootDirGatherTime;
		
		TArray<FExtFolderGatherResult> SubDirGatherResults;
		TArray<FExtAssetContentRootGatherResult> AssetContentRootGatherResults;

		bRootDirGathered = BackgroundFolderGatherer->GetAndTrimGatherResult(RootDirGatherResults, RootDirGatherTime, SubDirGatherResults, AssetContentRootGatherResults);
		bSubDirGathered = SubDirGatherResults.Num() > 0;
		bool bAssetContentRootGathered = AssetContentRootGatherResults.Num() > 0;
		
		if (bAssetContentRootGathered)
		{
			for (const auto& GatherResult : AssetContentRootGatherResults)
			{
				const auto& AssetContentRoot = GatherResult.AssetContentRoot;
				const auto& AssetContentRootHost = GatherResult.AssetContentRootHost;
				const auto& AssetContentRootConfigDir = GatherResult.AssetContentRootConfigDir; 
				const auto& AssetContentType = GatherResult.AssetContentType;
				AddAssetContentRoot(AssetContentRoot.ToString(), AssetContentRootHost.ToString(), AssetContentRootConfigDir.ToString(), AssetContentType);
			}
		}

		const bool bHasFolderGatheredToParse = bRootDirGathered || (bRefreshPathViewWhileScan && bSubDirGathered);

		if (bHasFolderGatheredToParse)
		{
			TArray<FExtFolderGatherResult>& GatherResults = bRootDirGathered ? RootDirGatherResults : SubDirGatherResults;

			for (const auto& GatherResult : GatherResults)
			{
				const auto& RoootFolder = GatherResult.RootPath;
				const auto& ParentFolder = GatherResult.ParentPath;
				const auto& SubFolders = GatherResult.SubPaths;
				const auto& RecurseSubFolders = GatherResult.RecurseSubPaths;
				const auto& FolderPakages = GatherResult.FolderPakages;

				if (bRootDirGathered)
				{
					ECB_LOG(Display, TEXT("BackgroundFolderGathered for %s, SubFolders: %d, RecurseSubFolders: %d, FolderPakages: %d"), *ParentFolder.ToString(), SubFolders.Num(), RecurseSubFolders.Num(), FolderPakages.Num());
				}
				else
				{
					//ECB_LOG(Display, TEXT("[Sub] BackgroundFolderGathered for %s, root: %s, %d, %d"), *ParentFolder.ToString(), *RoootFolder.ToString(), RecurseSubFolders.Num(), FolderPakages.Num());
				}
				if (!BackgroundGatheringFolders.Contains(RoootFolder))
				{
					//ECB_LOG(Display, TEXT("   => %s Removed. Skip gathered result."), *RoootFolder.ToString());
					continue;
				}

				{
					auto& CurrentCachedSubPaths = State.CachedSubPaths.FindOrAdd(ParentFolder);
					//if (CurrentCachedSubPaths.Num() != SubFolders.Num()) // already cached?
					{
						CurrentCachedSubPaths.Empty();
						CurrentCachedSubPaths.Append(SubFolders);
					}
					if (bRootDirGathered)
					{
						ECB_LOG(Display, TEXT("  => %d subfolders."), CurrentCachedSubPaths.Num());
					}
				}

				for (auto& Pair : RecurseSubFolders)
				{
					auto& Parent = Pair.Key;
					auto& Subs = Pair.Value;
					auto& CurrentCachedSubPaths = State.CachedSubPaths.FindOrAdd(Parent);
					//if (CurrentCachedSubPaths.Num() != SubFolders.Num()) // already cached?
					{
						CurrentCachedSubPaths.Empty();
						CurrentCachedSubPaths.Append(Subs);
					}
				}
				if (bRootDirGathered)
				{
					ECB_LOG(Display, TEXT("  => %d recursivley subfolders."), RecurseSubFolders.Num());
				}

				for (auto& Pair : FolderPakages)
				{
					State.CachedFilePathsByFolder.FindOrAdd(Pair.Key) = Pair.Value;
				}
				
				if (bRootDirGathered)
				{
					BackgroundGatheringFolders.Remove(ParentFolder);
					FolderFinishGatheringEvent.Broadcast(ParentFolder.ToString(), RoootFolder.ToString());
				}
				else
				{
					FolderFinishGatheringEvent.Broadcast(ParentFolder.ToString(), RoootFolder.ToString());
					BackgroundGatheringSubFolder = ParentFolder;
				}
			}

			if (bRootDirGathered)
			{
				ECB_LOG(Display, TEXT("# Background folder discovery finished in %0.4f seconds"), RootDirGatherTime);
			}
		}
	}

	//return bRefreshPathViewWhileScan ? (bMainGathered || bSubDirGathered) : bMainGathered;
	return bRootDirGathered;
}

bool FExtAssetRegistry::IsFolderBackgroundGathering(const FString& InFolder) const
{
	return BackgroundGatheringFolders.Contains(*InFolder);
}

bool FExtAssetRegistry::GetAndTrimAssetGatherResult()
{
	bool bHasGatherResult = false;

	if (BackgroundAssetGatherer.IsValid())
	{
		FExtAssetGatherResult GatherResult;
		double GatherTime = 0.;
		int32 PathsLeftInBatch = 0;

		bHasGatherResult = BackgroundAssetGatherer->GetAndTrimGatherResult(GatherResult, GatherTime, PathsLeftInBatch);

		if (bHasGatherResult)
		{
			const TMap<FName, FExtAssetData>& ParsedAssets = GatherResult.GetParsedAssets();
			for (const auto& Pair : ParsedAssets)
			{
				const FName& FilePath = Pair.Key;
				const FExtAssetData& AssetData = Pair.Value;

				ECB_LOG(Display, TEXT("BackgroundAssetGatherer for %s, bIsValide?: %d"), *FilePath.ToString(), AssetData.IsValid());

				FExtAssetData** FoundAssetData = State.CachedAssetsByFilePath.Find(FilePath);
				const bool bAlreadyCached = FoundAssetData != nullptr;

				bool bNeedBrodcast = false;

				if (!bAlreadyCached)
				{
					FExtAssetData* NewAssetData = new FExtAssetData(AssetData);
					State.CacheNewAsset(NewAssetData);

					// todo: Update CachedAssetsByFolder, move to CacheNewAsset?
// 					{
// 						const FString Dir = NewAssetData->GetFolderPath();
// 						ECB_LOG(Display, TEXT("    => Dir: %s."), *Dir);
// 						
// 						TArray<FExtAssetData*>& CachedAssets = State.CachedAssetsByFolder.FindOrAdd(*Dir);
// 						CachedAssets.AddUnique(NewAssetData);
// 					}

					ECB_LOG(Display, TEXT("    => Cache as New Asset. %s"), *NewAssetData->PackageName.ToString());
					AssetGatheredEvent.Broadcast(*NewAssetData, PathsLeftInBatch);
				}
				else if (!(*FoundAssetData)->IsParsed())
				{
					*(*FoundAssetData) = AssetData;
					State.ReCacheParsedAsset(*FoundAssetData);
					ECB_LOG(Display, TEXT("    => ReCacheParsedAsset. %s"), *(*FoundAssetData)->PackageName.ToString());
					AssetGatheredEvent.Broadcast(**FoundAssetData, PathsLeftInBatch);
				}
			}

			if (PathsLeftInBatch == 0)
			{
				ECB_LOG(Display, TEXT("# Background asset discovery finished in %0.4f seconds"), GatherTime);
			}
		}

		if (bHasGatherResult/*Sth. Gathered*/ || (bIsGatheringAssets && !bHasGatherResult) /*Was Gathering*/) 
		{
			const FAssetGatherProgressUpdateData ProgressUpdateData(
				NumToGatherAssets/*NumFilteredAssets*/,						// NumTotalAssets
				NumToGatherAssets - PathsLeftInBatch,	// NumAssetsProcessedByAssetRegistry
				0,						// NumAssetsPendingDataLoad
				false/*bIsGatheringAssets*/						// bIsDiscoveringAssetFiles
			);
			AssetGatherProgressUpdatedEvent.Broadcast(ProgressUpdateData);

			ECB_LOG(Display, TEXT("    AssetGatherProgressUpdatedEvent => NumToGatherAssets: %d, PathsLeftInBatch: %d, bIsGatheringAssets: %d")
				, NumToGatherAssets, PathsLeftInBatch, bIsGatheringAssets);
		}
		bIsGatheringAssets = PathsLeftInBatch != 0;
	}

	return bHasGatherResult;
}

bool FExtAssetRegistry::GetAssets(const struct FARFilter& InFilter, TArray<FExtAssetData>& OutAssetData)
{
	double GetAssetsStartTime = FPlatformTime::Seconds();

	if (!IsFilterValid(InFilter, /*bAllowRecursion = */ true) || InFilter.IsEmpty())
	{
		return false;
	}

	// Expand recursion on filter
	FARFilter Filter;
	ExpandRecursiveFilter(InFilter, Filter);

	// Verify filter input. If all assets are needed, use GetAllAssets() instead.
	if (!IsFilterValid(Filter, /*bAllowRecursion = */ false) || InFilter.IsEmpty())
	{
		return false;
	}

	// Prepare a set of each filter component for fast searching
	TSet<FName> FilterPackagePaths(Filter.PackagePaths);
	TSet<FTopLevelAssetPath> FilterClassPaths(Filter.ClassPaths);
	TSet<FName> FilterPackageNames(Filter.PackageNames);

	// Form a set of assets matched by each filter	
	TArray<TSet<FExtAssetData*>> DiskFilterSets;

	bool bCanceledInFilterByPathsPhase = false;

	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	const bool bIgnoreInvalidAssets = !ExtContentBrowserSetting->DisplayInvalidAssets;

	// On disk package paths
	if (FilterPackagePaths.Num())
	{
		TSet<FExtAssetData*>& PathFilter = DiskFilterSets[DiskFilterSets.AddDefaulted()];
		{
			TArray<FExtAssetData*> FilteredAssets;
			for (FName PackagePath : FilterPackagePaths)
			{
				const TArray<FExtAssetData*> PathAssets = GetCachedAssetsByFolder(PackagePath);
				if (PathAssets.Num() > 0)
				{
					for (FExtAssetData* AssetData : PathAssets)
					{
#if ECB_FEA_SHOW_INVALID
						const bool bShouldIgnore = bIgnoreInvalidAssets && !AssetData->IsValid();
#else
						const bool bShouldIgnore = !AssetData->IsValid();
#endif
						if (!bShouldIgnore)
						{
							FilteredAssets.Add(AssetData);
						}
					}
				}
			}
			PathFilter.Append(FilteredAssets);
		}
	}

	// On disk classes
	if (FilterClassPaths.Num())
	{
		TSet<FExtAssetData*>& ClassFilter = DiskFilterSets[DiskFilterSets.AddDefaulted()];

		TArray<FExtAssetData*> FilteredAssets;
		for (FTopLevelAssetPath ClassPath : FilterClassPaths)
		{
			const TSet<FName>& InThosePaths = FilterPackagePaths;
			FName ClassName = ClassPath.GetAssetName();
			const TArray<FExtAssetData*> ClassAssets = GetCachedAssetsByClass(ClassName);
			if (ClassAssets.Num() > 0)
			{
				for (FExtAssetData* AssetData : ClassAssets)
				{
#if ECB_FEA_SHOW_INVALID
					const bool bShouldIgnore = bIgnoreInvalidAssets && !AssetData->IsValid();
#else
					const bool bShouldIgnore = !AssetData->IsValid();
#endif
					if (!bShouldIgnore)
					{
						FilteredAssets.Add(AssetData);
					}
				}
			}
		}
		ClassFilter.Append(FilteredAssets);
	}

	// On disk package names
	if (FilterPackageNames.Num())
	{
		TSet<FExtAssetData*>& PackageNameFilter = DiskFilterSets[DiskFilterSets.AddDefaulted()];

		for (FName PackageName : FilterPackageNames)
		{
			const FName* FilePath = State.CachedPackageNameToFilePathMap.Find(PackageName);
			if (FilePath != nullptr)
			{
				FExtAssetData* AssetData = GetCachedAssetByFilePath(*FilePath);
				if (AssetData/* && AssetData->IsValid()*/)
				{
					PackageNameFilter.Add(AssetData);
				}
			}
		}
	}

	// If we have any filter sets, add the assets which are contained in the sets to OutAssetData
	if (DiskFilterSets.Num() > 0)
	{
		// Initialize the combined filter set to the first set, in case we can skip combining.
		TSet<FExtAssetData*>* CombinedFilterSetPtr = &DiskFilterSets[0];
		TSet<FExtAssetData*> IntersectedFilterSet;

		// If we have more than one set, we must combine them. We take the intersection
		if (DiskFilterSets.Num() > 1)
		{
			IntersectedFilterSet = *CombinedFilterSetPtr;
			CombinedFilterSetPtr = &IntersectedFilterSet;

			for (int32 SetIdx = 1; SetIdx < DiskFilterSets.Num() && IntersectedFilterSet.Num() > 0; ++SetIdx)
			{
				// If the other set is smaller, swap it so we iterate the smaller set
				TSet<FExtAssetData*> OtherFilterSet = DiskFilterSets[SetIdx];
				if (OtherFilterSet.Num() < IntersectedFilterSet.Num())
				{
					Swap(OtherFilterSet, IntersectedFilterSet);
				}

				for (auto It = IntersectedFilterSet.CreateIterator(); It; ++It)
				{
					if (!OtherFilterSet.Contains(*It))
					{
						It.RemoveCurrent();
						continue;
					}
				}
			}
		}

		// Iterate over the final combined filter set to load and add to OutAssetData
		for (FExtAssetData* AssetData : *CombinedFilterSetPtr)
		{
			OutAssetData.Add(*AssetData);
		}
	}

	ECB_LOG(Display, TEXT("GetAssets completed in %0.4f seconds"), FPlatformTime::Seconds() - GetAssetsStartTime);

	return true;
}

void FExtAssetRegistry::RunAssetsThroughFilter(TArray<FExtAssetData>& AssetDataList, const FARFilter& Filter) const
{
	if (!Filter.IsEmpty())
	{
		TSet<FTopLevelAssetPath> RequestedClassNames;
		if (Filter.bRecursiveClasses && Filter.ClassPaths.Num() > 0)
		{
			// First assemble a full list of requested classes from the ClassTree
			// GetSubClasses includes the base classes
			GetSubClasses(Filter.ClassPaths, Filter.RecursiveClassPathsExclusionSet, RequestedClassNames);
		}

		// todo:
#if 1
		for (int32 AssetDataIdx = AssetDataList.Num() - 1; AssetDataIdx >= 0; --AssetDataIdx)
		{
			const FExtAssetData& AssetData = AssetDataList[AssetDataIdx];

			// Package Names
			if (Filter.PackageNames.Num() > 0)
			{
				bool bPassesPackageNames = false;
				for (int32 NameIdx = 0; NameIdx < Filter.PackageNames.Num(); ++NameIdx)
				{
					if (Filter.PackageNames[NameIdx] == AssetData.PackageName)
					{
						bPassesPackageNames = true;
						break;
					}
				}

				if (!bPassesPackageNames)
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// Package Paths
			if (Filter.PackagePaths.Num() > 0)
			{
				bool bPassesPackagePaths = false;
				if (Filter.bRecursivePaths)
				{
					FString AssetPackagePath = AssetData.PackageFilePath.ToString();
					for (int32 PathIdx = 0; PathIdx < Filter.PackagePaths.Num(); ++PathIdx)
					{
						const FString Path = Filter.PackagePaths[PathIdx].ToString();
						if (AssetPackagePath.StartsWith(Path))
						{
							// Only match the exact path or a path that starts with the target path followed by a slash
							if (Path.Len() == 1 || Path.Len() == AssetPackagePath.Len() || AssetPackagePath.Mid(Path.Len(), 1) == TEXT("/"))
							{
								bPassesPackagePaths = true;
								break;
							}
						}
					}
				}
				else
				{
					// Non-recursive. Just request data for each requested path.
					for (int32 PathIdx = 0; PathIdx < Filter.PackagePaths.Num(); ++PathIdx)
					{
						if (Filter.PackagePaths[PathIdx] == *AssetData.GetFolderPath())
						{
							bPassesPackagePaths = true;
							break;
						}
					}
				}

				if (!bPassesPackagePaths)
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// ObjectPaths
			if (Filter.SoftObjectPaths.Num() > 0)
			{
				bool bPassesObjectPaths = Filter.SoftObjectPaths.Contains(AssetData.GetSoftObjectPath());

				if (!bPassesObjectPaths)
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// Classes
			if (Filter.ClassPaths.Num() > 0)
			{
				bool bPassesClasses = false;
				if (Filter.bRecursiveClasses)
				{
					// Now check against each discovered class
					for (const FTopLevelAssetPath& ClassName : RequestedClassNames)
					{
						if (ClassName.GetAssetName() == AssetData.AssetClass)
						{
							bPassesClasses = true;
							break;
						}
					}
				}
				else
				{
					// Non-recursive. Just request data for each requested classes.
					for (int32 ClassIdx = 0; ClassIdx < Filter.ClassPaths.Num(); ++ClassIdx)
					{
						if (Filter.ClassPaths[ClassIdx].GetPackageName() == AssetData.AssetClass)
						{
							bPassesClasses = true;
							break;
						}
					}
				}

				if (!bPassesClasses)
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// Tags and values
			if (Filter.TagsAndValues.Num() > 0)
			{
				bool bPassesTags = false;
				for (auto FilterTagIt = Filter.TagsAndValues.CreateConstIterator(); FilterTagIt; ++FilterTagIt)
				{
					bool bAccept;
					if (!FilterTagIt.Value().IsSet())
					{
						// this probably doesn't make sense, but I am preserving the original logic
						bAccept = AssetData.TagsAndValues.ContainsKeyValue(FilterTagIt.Key(), FString());
					}
					else
					{
						bAccept = AssetData.TagsAndValues.ContainsKeyValue(FilterTagIt.Key(), FilterTagIt.Value().GetValue());
					}

					if (bAccept)
					{
						bPassesTags = true;
						break;
					}
				}

				if (!bPassesTags)
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}
		}
#endif
	}
}

bool FExtAssetRegistry::GetDependencies(const FExtAssetIdentifier& AssetIdentifier, TArray<FExtAssetIdentifier>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType /*= EAssetRegistryDependencyType::All*/)
{
	FName PackageName = AssetIdentifier.PackageName;
	TArray<FName> Dependencies;
	if (GetDependencies(PackageName, Dependencies, InDependencyType))
	{
		for (const FName& DependPackage : Dependencies)
		{
			OutDependencies.Add(DependPackage);
		}
		return true;
	}
	return false;
}

bool FExtAssetRegistry::GetDependencies(FName PackageName, TArray<FName>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType /*= EAssetRegistryDependencyType::Packages*/)
{
	FARFilter Filter;
	Filter.PackageNames.Add(PackageName);
	TArray<FExtAssetData> AssetDataList;
	FExtContentBrowserSingleton::GetAssetRegistry().GetAssets(Filter, AssetDataList);

	if (AssetDataList.Num() > 0)
	{
		FExtAssetData& AssetData = AssetDataList[0];
		if (AssetData.IsValid() && AssetData.HasContentRoot())
		{
			TSet<FName> Dependencies;
			if (!!(InDependencyType & EAssetRegistryDependencyType::Hard))
			{
				Dependencies.Append(AssetData.HardDependentPackages);
			}
			if (!!(InDependencyType & EAssetRegistryDependencyType::Soft))
			{
				Dependencies.Append(AssetData.SoftReferencesList);
			}

			TSet<FName> RemappedDependencies;

			FString AssetContentDir = AssetData.AssetContentRoot.ToString();
			auto& AssetContentType = AssetData.AssetContentType;
			FString GameRoot(TEXT("/Game/"));
			if (AssetContentType == FExtAssetData::EContentType::Plugin && !AssetContentDir.IsEmpty())
			{
				GameRoot = FExtAssetDataUtil::GetPluginNameFromAssetContentRoot(AssetContentDir);
			}

			for (const FName Depend : Dependencies)
			{
				FString RemappedFilePath;
				if (FExtAssetDataUtil::RemapGamePackageToFullFilePath(GameRoot, Depend.ToString(), AssetContentDir, RemappedFilePath))
				{
					FString RemappedPackageName = FExtAssetData::ConvertFilePathToTempPackageName(RemappedFilePath);

					// If haven't been cached with temp PackageName, try cache with file path
					if (GetCachedAssetByPackageName(*RemappedPackageName) == nullptr)
					{
						GetOrCacheAssetByFilePath(*RemappedFilePath);
					}

					RemappedDependencies.Add(*RemappedPackageName);
				}
				else
				{
					RemappedDependencies.Add(Depend);
				}
			}

			OutDependencies = RemappedDependencies.Array();

			return true;
		}
	}
	return false;
}

bool FExtAssetRegistry::IsGatheringAssets() const
{
	return bIsGatheringAssets;
}

/////////////////////////////////////////////////////////////
// FExtAssetRegistry Impl
//

FPrimaryAssetId FExtAssetRegistry::ExtractPrimaryAssetIdFromFakeAssetData(const FExtAssetData& InAssetData)
{
	static const FName PrimaryAssetFakeAssetDataPackagePath = FName("/Temp/PrimaryAsset");

	if (InAssetData.PackagePath == PrimaryAssetFakeAssetDataPackagePath)
	{
		return FPrimaryAssetId(InAssetData.AssetClass, InAssetData.AssetName);
	}
	return FPrimaryAssetId();
}

void FExtAssetRegistry::ExtractAssetIdentifiersFromAssetDataList(const TArray<FExtAssetData>& AssetDataList, TArray<FExtAssetIdentifier>& OutAssetIdentifiers)
{
	for (const FExtAssetData& AssetData : AssetDataList)
	{
		FPrimaryAssetId PrimaryAssetId = ExtractPrimaryAssetIdFromFakeAssetData(AssetData);

		if (PrimaryAssetId.IsValid())
		{
			OutAssetIdentifiers.Add(PrimaryAssetId);
		}
		else
		{
			OutAssetIdentifiers.Add(AssetData.PackageName);
		}
	}
}

void FExtAssetRegistry::Shutdown()
{
	StopAllBackgroundGathering();
	
}

namespace  FContentPathUtil
{
	// return: num of paths been merged
	int32 CombineAndSortRootContentPaths(const TArray<FString>& InPathsToCombine, TArray<FString>& OutCombinedPaths)
	{
		return FPathsUtil::SortAndMergeDirs(InPathsToCombine, OutCombinedPaths);
	}

	void GetCleanExistPaths(const TArray<FString>& InPathsToClean, TArray<FString>& OutCleanedPaths)
	{
		OutCleanedPaths.Empty();

		for (const FString& Path : InPathsToClean)
		{
			FString CleanPath = Path.TrimStartAndEnd();

			if (CleanPath.IsEmpty())
			{
				continue;
			}
			CleanPath = FPaths::ConvertRelativePathToFull(Path);
			FPaths::NormalizeDirectoryName(CleanPath);

			if (!CleanPath.IsEmpty() && IFileManager::Get().DirectoryExists(*CleanPath))
			{
				OutCleanedPaths.Add(CleanPath);
			}
		}
	}

	void ExpandCachedSubsRecursively(const TMap<FName, TSet<FName>>& InCachedSubPaths, const FName& InBase, TSet<FName>& AllSubs)
	{
		if (InCachedSubPaths.Contains(InBase))
		{
			const TSet<FName>& Subs = *InCachedSubPaths.Find(InBase);
			AllSubs.Append(Subs);

			for (const FName& Sub : Subs)
			{
				ExpandCachedSubsRecursively(InCachedSubPaths, Sub, AllSubs);
			}
		}
	}
}

void FExtAssetRegistry::LoadRootContentPaths()
{	
	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	if (ExtContentBrowserSetting->bCacheMode)
	{
		RootContentPaths = State.CachedRootContentPaths;
	}
	else
	{
		// Load from config
		RootContentPaths.Empty();
		GConfig->GetArray(*SExtContentBrowser::SettingsIniSection, TEXT("RootContentPaths"), RootContentPaths, GEditorPerProjectIni);
	}
	
	MergeRootContentPathsWith(RootContentPaths, /*bReplaceCurrent*/ true);
}

void FExtAssetRegistry::MergeRootContentPathsWith(const TArray<FString>& InPathsToMerge, bool bReplaceCurrent)
{
	TArray<FString> PathsToMerge(InPathsToMerge);

	if (bReplaceCurrent)
	{
		RootContentPaths.Empty();
	}

	TSet<FString> Before(RootContentPaths);

	TArray<FString> CleanedExistPaths;
	FContentPathUtil::GetCleanExistPaths(PathsToMerge, CleanedExistPaths);
	for (const FString& CleanPath : CleanedExistPaths)
	{
		RootContentPaths.Add(CleanPath);
	}

	CombineRootContentPaths();

	SaveRootContentPaths();

	CacheRootContentPathInfo();

	TSet<FString> After(RootContentPaths);
	TSet<FString> Removed = Before.Difference(After);
	TSet<FString> Added = After.Difference(Before);

	if (Removed.Num() > 0)
	{
		// Stop gathering
		for (const FString RemovedFolder : Removed)
		{
			FName RemovedFolderName(*RemovedFolder);
			BackgroundGatheringFolders.Remove(RemovedFolderName);
			if (BackgroundFolderGatherer.IsValid())
			{
				BackgroundFolderGatherer->StopSearchFolder(RemovedFolder);
			}
		}

		UpdateCacheByRemovedFolders(Removed.Array());
	}

	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	const bool bCacheMode = ExtContentBrowserSetting->bCacheMode;
	if (Added.Num() > 0 && !bCacheMode)
	{
		for (const FString AddedFolder : Added)
		{
			StartFolderGathering(AddedFolder);
		}
	}

	if (Removed.Num() > 0 && Added.Num() == 0)
	{
		RootPathRemovedEvent.Broadcast(Removed.Array()[0]);
	}
	else if (Added.Num() > 0 && Removed.Num() == 0)
	{
		RootPathAddedEvent.Broadcast(Added.Array()[0]);
	}
	else
	{
		RootPathUpdatedEvent.Broadcast();
	}
}

void FExtAssetRegistry::ReplaceRootContentPathsWith(const TArray<FString>& InPaths)
{
	MergeRootContentPathsWith(InPaths, /*bReplaceCurrent*/ true);
}

void FExtAssetRegistry::SaveRootContentPaths()
{
	State.CachedRootContentPaths = RootContentPaths;

	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	GConfig->SetArray(*SExtContentBrowser::SettingsIniSection, TEXT("RootContentPaths"), RootContentPaths, GEditorPerProjectIni);
	GConfig->Flush(/*bRead*/ false, GEditorPerProjectIni);
}

void FExtAssetRegistry::QueryRootContentPaths(TArray<FString>& OutRootContentPaths) const
{
	OutRootContentPaths = RootContentPaths;
}

bool FExtAssetRegistry::QueryRootContentPathFromFilePath(const FString& InFilePath, FString& OutRootContentPath)
{
	for (const auto& Root : RootContentPaths)
	{
		if (FPathsUtil::IsSubOrSamePath(InFilePath, Root))
		{
			OutRootContentPath = Root;
			return true;
		}
	}
	return false;
}

bool FExtAssetRegistry::QueryRootContentPathInfo(const FString& InRootContentPath, FText* OutDisplayName, FExtAssetData::EContentType* OutContentType, FString* OutAssetContentRoot) const
{
	if (const auto& PathInfoPtr = RootContentPathsInfo.Find(InRootContentPath))
	{
		if (OutContentType)
		{
			*OutContentType = PathInfoPtr->ContentType;
		}

		if (OutDisplayName)
		{
			*OutDisplayName = PathInfoPtr->DisplayName;
		}

		if (OutAssetContentRoot)
		{
			*OutAssetContentRoot = PathInfoPtr->AssetContentRoot;
		}
		
		return true;
	}
	return false;
}

bool FExtAssetRegistry::ParseAssetContentRoot(const FString& InFilePath, FString& OutRelativePath, FString& OutAssetContentRoot, FExtAssetData::EContentType& OutAssetContentType, TSet<FName>* InAllDependencies)
{
	bool bFoundRootPath = false;

	FString AssetContentRoot;
	FString RelativePath;

	if (GetAssetContentRoot(InFilePath, AssetContentRoot))
	{
		RelativePath = InFilePath.Mid(FCString::Strlen(*AssetContentRoot));
		FName AssetContentRootName(*AssetContentRoot);
		if (State.CachedAssetContentType.Contains(AssetContentRootName))
		{
			OutAssetContentType = State.CachedAssetContentType[AssetContentRootName];
		}
		bFoundRootPath = true;
	}
	else
	{
		FString AssetContentRootHost;

#if ECB_FEA_IMPORT_PLUGIN
		// Plugin Content
		if (!bFoundRootPath)
		{
			bFoundRootPath = FExtContentDirFinder::FindWithFile(/*AssetFilePath=*/ InFilePath, /*FileToFind=*/ TEXT(".uplugin"), /*bExtension=*/ true, /*FolderPattern=*/ TEXT("/Content"), /*FoundRoot=*/ AssetContentRoot, /*RelativePath=*/ RelativePath);
			if (bFoundRootPath)
			{
				AssetContentRootHost = AssetContentRoot / TEXT("..");
				OutAssetContentType = FExtAssetData::EContentType::Plugin;
			}
		}
#endif

		// Project Content
		if (!bFoundRootPath)
		{
			bFoundRootPath = FExtContentDirFinder::FindWithFile(InFilePath, TEXT(".uproject"), /*bExtension*/ true, TEXT("/Content"), AssetContentRoot, RelativePath);
			if (bFoundRootPath)
			{
				AssetContentRootHost = AssetContentRoot / TEXT("..");
				OutAssetContentType = FExtAssetData::EContentType::Project;
			}
		}

		// Vault Cache Content
		if (!bFoundRootPath)
		{
			bFoundRootPath = FExtContentDirFinder::FindWithFile(InFilePath, TEXT("manifest"), /*bExtension*/ false, TEXT("/data/Content"), AssetContentRoot, RelativePath);
			if (bFoundRootPath)
			{
				AssetContentRootHost = AssetContentRoot / TEXT("../..");
				OutAssetContentType = FExtAssetData::EContentType::VaultCache;
			}
		}

		// Project Content without .uproject file
		if (!bFoundRootPath)
		{
			bFoundRootPath = FExtContentDirFinder::FindFolder(InFilePath, TEXT("Content"), AssetContentRoot, RelativePath);
			if (bFoundRootPath)
			{
				AssetContentRootHost = AssetContentRoot / TEXT("..");
				OutAssetContentType = FExtAssetData::EContentType::PartialProject;
			}
		}

#if ECB_WIP_IMPORT_ORPHAN && 1
		if (!bFoundRootPath)
		{
			// todo: try getting asset content root from dependencies
			FString RootContentPath;
			if (QueryRootContentPathFromFilePath(InFilePath, RootContentPath) && InAllDependencies)
			{

				// Get all paths
				TSet<FName> SubPaths;
				SubPaths.Add(*RootContentPath);
				FExtContentBrowserSingleton::GetAssetRegistry().GetCachedSubPaths(*RootContentPath, SubPaths, /*bRecursively = */ true);

				// also include parent folder in case user select one step deeper
				FString Parent = RootContentPath / TEXT("..");
				FPaths::CollapseRelativeDirectories(Parent);
				SubPaths.Add(*Parent);

				for (const FName& Dir : SubPaths)
				{
					AssetContentRoot = Dir.ToString();
					RelativePath = InFilePath.RightChop(AssetContentRoot.Len());

					if (InAllDependencies->Num() > 0)
					{
						for (const FName& DependPackageName : *InAllDependencies)
						{
							FString DependPackageString = DependPackageName.ToString();
							if (DependPackageString.StartsWith("/Script/"))
							{
								continue;
							}

							FString PackageRoot = FExtAssetDataUtil::GetPackageRootFromFullPackagePath(DependPackageString);
							FString DependentFilePath;
							if (FExtAssetDataUtil::RemapGamePackageToFullFilePath(PackageRoot, DependPackageString, AssetContentRoot, DependentFilePath))
							{
								ECB_LOG(Display, TEXT("[ParseAssetContent] For Orphan Aset: Found dependency: %s In %s"), *DependentFilePath, *AssetContentRoot);
								bFoundRootPath = true;
								break;
							}
						}
					}
					if (bFoundRootPath)
						break;
				}

				if (bFoundRootPath)
				{
					AssetContentRootHost = AssetContentRoot;
					OutAssetContentType = FExtAssetData::EContentType::Orphan;
				}
			}
		}
#endif

		if (bFoundRootPath)
		{
			// Cache Asset Content Root
			FString ContentRootConfigDir = FExtConfigUtil::GetConfigDirByContentRoot(AssetContentRoot);
			FPaths::CollapseRelativeDirectories(AssetContentRootHost);
			AddAssetContentRoot(AssetContentRoot, AssetContentRootHost, ContentRootConfigDir, OutAssetContentType);
		}
		
	}

	if (bFoundRootPath)
	{
		FPaths::NormalizeFilename(RelativePath);
		FPaths::NormalizeDirectoryName(AssetContentRoot);

		if (RelativePath.StartsWith(TEXT("/")))
		{
			RelativePath = RelativePath.Mid(1);
		}

		OutRelativePath = RelativePath;
		OutAssetContentRoot = AssetContentRoot;
		return true;
	}

	return false;
}

void FExtAssetRegistry::AddAssetContentRoot(const FString& InAssetContentRoot, const FString& InAssetContentRootHost, const FString& InConfigDir, FExtAssetData::EContentType InContentType)
{
	int32 NumCached = State.CachedAssetContentRoots.Num();
	bool bAdded = false;

	FName AssetContentRootName(*InAssetContentRoot);

	for (int32 Index = 0; Index < NumCached; ++Index)
	{
		FString RootDir = State.CachedAssetContentRoots[Index].ToString();

		if (!FPaths::IsSamePath(InAssetContentRoot, RootDir) && InAssetContentRoot.Len() >= RootDir.Len())
		{
			State.CachedAssetContentRoots.Insert(AssetContentRootName, Index);
			bAdded = true;
			break;
		}
	}

	if (!bAdded)
	{
		State.CachedAssetContentRoots.Add(AssetContentRootName);
	}

	State.CachedAssetContentType.FindOrAdd(AssetContentRootName) = InContentType;
	State.CachedAssetContentRootConfigDirs.FindOrAdd(AssetContentRootName) = *InConfigDir;
	if (!InAssetContentRootHost.IsEmpty())
	{
		State.CachedAssetContentRootHosts.FindOrAdd(*InAssetContentRootHost) = AssetContentRootName;
		CacheFolderColor(AssetContentRootName);
	}
}

bool FExtAssetRegistry::GetAssetContentRoot(const FString& InFileOrFolderPath, FString& OutFoundAssetContentDir) const
{
	for (int32 Index = 0; Index < State.CachedAssetContentRoots.Num(); ++Index)
	{
		FString RootDir = State.CachedAssetContentRoots[Index].ToString();
		//if (InFilePath.StartsWith(RootDir))
		if (FPathsUtil::IsFileInDir(InFileOrFolderPath, RootDir))
		{
			OutFoundAssetContentDir = RootDir;
			return true;
		}
	}
	return false;
}

FExtAssetData::EContentType FExtAssetRegistry::GetAssetContentRootContentType(const FString& InAssetContentRoot) const
{
	TMap<FName, FExtAssetData::EContentType> CachedAssetContentType;
	FName AssetContentRootName = *InAssetContentRoot;
	if (const FExtAssetData::EContentType* ContentType = State.CachedAssetContentType.Find(AssetContentRootName))
	{
		return *ContentType;
	}
	return FExtAssetData::EContentType::Unknown;
}

bool FExtAssetRegistry::GetFolderColor(const FString& InFolderPath, FLinearColor& OutFolderColor) const
{
	if (const FLinearColor* FoundColor = State.CachedFolderColors.Find(*InFolderPath))
	{
		OutFolderColor = *FoundColor;
		return true;
	}
	return false;
}

bool FExtAssetRegistry::IsAsetContentRootHasFolderColor(const FString& InAssetContentRoot) const
{
	return State.CachedFolderColorIndices.Contains(*InAssetContentRoot);
}

void FExtAssetRegistry::GetAssetContentRootColoredFolders(const FString& InAssetContentRoot, TArray<FName>& OutColoredFolders) const
{
	if (auto* Folders = State.CachedFolderColorIndices.Find(*InAssetContentRoot))
	{
		OutColoredFolders = *Folders;
	}
}

void FExtAssetRegistry::GetAssetContentRootFolderColors(const FString& InAssetContentRoot, TMap<FName, FLinearColor>& OutFoldersColor) const
{
	OutFoldersColor.Empty();

	if (const TArray<FName>* Folders = State.CachedFolderColorIndices.Find(*InAssetContentRoot))
	{
		for (const auto& Folder : *Folders)
		{
			FLinearColor FolderColor;				
			if (GetFolderColor(Folder.ToString(), FolderColor))
			{
				OutFoldersColor.Add(Folder, FolderColor);
			}
		}
	}
}

bool FExtAssetRegistry::IsRootFolder(const FString& InPath) const
{
	for (const FString& RootPath : RootContentPaths)
	{
		//if (InPath.Equals(RootPath, ESearchCase::CaseSensitive))
		if (FPaths::IsSamePath(RootPath, InPath))
		{
			return true;
		}
	}
	return false;
}

bool FExtAssetRegistry::IsRootFolders(const TArray<FString>& InPaths) const
{
	bool bIsRootFolders = false;

	if (InPaths.Num() > 0)
	{
		bIsRootFolders = true;
		for (const FString& Path : InPaths)
		{
			if (!IsRootFolder(Path))
			{
				bIsRootFolders = false;
				break;
			}
		}
	}
	return bIsRootFolders;
}

bool FExtAssetRegistry::AddRootFolder(const FString& InPath, TArray<FString>* OutAdded, TArray<FString>* OutCombined)
{
	if (!InPath.IsEmpty())
	{
		FString FolderPath = FPaths::ConvertRelativePathToFull(InPath);
		FPaths::NormalizeDirectoryName(FolderPath);

		// Validate
		if (FolderPath.IsEmpty())
		{
			return false;
		}

		// Exist or sub dir of existing?
		for (const FString& RootPath : RootContentPaths)
		{
			const bool bExistOrSubPath = FPathsUtil::IsSubOrSamePath(FolderPath, RootPath);
			if (bExistOrSubPath)
			{
				if (OutCombined)
				{
					*OutCombined = {FolderPath};
				}
				return true;
			}
		}

		TSet<FString> Before(RootContentPaths);
		{
			RootContentPaths.Add(FolderPath);
			CombineRootContentPaths();
			SaveRootContentPaths();
		}
		TSet<FString> After(RootContentPaths);

		// Old paths been consolidated, clear cache to trigger re-cache
		TSet<FString> Combined = Before.Difference(After);
		TSet<FString> Added = After.Difference(Before);
		if (Combined.Num() > 0)
		{
			// Only if Removed is not sub path of Added
			TArray<FString> RemovedForUpdateCache;
			for (const FString& RemovedFolder : Combined)
			{
				bool bIsSubPath = false;
				for (const FString& AddedFolder : Added)
				{
					if (FPathsUtil::IsSubOrSamePath(RemovedFolder, AddedFolder))
					{
						bIsSubPath = true;
						break;
					}
				}
				if (!bIsSubPath)
				{
					RemovedForUpdateCache.Add(RemovedFolder);
				}
			}
			UpdateCacheByRemovedFolders(RemovedForUpdateCache);

			if (OutCombined)
			{
				*OutCombined = Combined.Array();
			}
		}

		CacheRootContentPathInfo();

		if (Added.Num() > 0)
		{
#if ECB_FEA_ASYNC_FOLDER_DISCOVERY
			StartFolderGathering(Added.Array()[0]); // todo: gather multiple folders
#endif
			RootPathAddedEvent.Broadcast(Added.Array()[0]);
			if (OutAdded)
			{
				*OutAdded = Added.Array();
			}
		}

		return true;
	}

	return false;
}

bool FExtAssetRegistry::ReloadRootFolders(const TArray<FString>& InPaths, TArray<FString>* OutReloaded /*= nullptr*/)
{
	TArray<FString> Removed;
	if (RemoveRootFolders(InPaths, &Removed))
	{
		for (const FString& ReloadFolder : Removed)
		{
			AddRootFolder(ReloadFolder);
			if (OutReloaded)
			{
				OutReloaded->Add(ReloadFolder);
			}
		}
		return true;
	}

	return false;
}

bool FExtAssetRegistry::RemoveRootFolders(const TArray<FString>& InPaths, TArray<FString>* OutRemovd)
{
	TSet<FString> Before(RootContentPaths);
	{
		for (const FString& InPath : InPaths)
		{
			if (!InPath.IsEmpty())
			{
				FString FolderPath = FPaths::ConvertRelativePathToFull(InPath);
				FPaths::NormalizeDirectoryName(FolderPath);
				RootContentPaths.Remove(FolderPath);
			}
		}
	}
	TSet<FString> After(RootContentPaths);

	TSet<FString> Removed = Before.Difference(After);
	if (Removed.Num() > 0)
	{
		CombineRootContentPaths();

		SaveRootContentPaths();

		// Stop gathering
		for (const FString RemovedFolder : Removed)
		{
			FName RemovedFolderName(*RemovedFolder);
			BackgroundGatheringFolders.Remove(RemovedFolderName);
			if (BackgroundFolderGatherer.IsValid())
			{
				BackgroundFolderGatherer->StopSearchFolder(RemovedFolder);
			}
		}

		UpdateCacheByRemovedFolders(Removed.Array());

		if (OutRemovd)
		{
			*OutRemovd = Removed.Array();
		}

		CacheRootContentPathInfo();

		// todo: should RootPathRemovedEvent take all removed paths?
		RootPathRemovedEvent.Broadcast(InPaths[0]);
		return true;
	}
	
	return false;
}

void FExtAssetRegistry::ReGatheringFolders(const TArray<FString>& InPaths)
{
	UpdateCacheByRemovedFolders(InPaths);

	for (const FString& InPath : InPaths)
	{
		StartFolderGathering(InPath);
	}

	FolderStartGatheringEvent.Broadcast(InPaths);
}

void FExtAssetRegistry::CacheAssets(const struct FARFilter& InFilter)
{
	double GetAssetsStartTime = FPlatformTime::Seconds();

	double ParseAssetsStartTime = FPlatformTime::Seconds();
	double PrepareAssetsTime = 0.;

	if (!IsFilterValid(InFilter, /*bAllowRecursion = */ true) || InFilter.IsEmpty())
	{
		return;
	}

	TSet<FExtAssetData*> FilteredAssets;

	{
		// Expand recursion on filter
		FARFilter Filter;
		ExpandRecursiveFilter(InFilter, Filter);

		// Verify filter input. If all assets are needed, use GetAllAssets() instead.
		if (!IsFilterValid(Filter, /*bAllowRecursion = */ false) || InFilter.IsEmpty())
		{
			return;
		}

		// Prepare a set of each filter component for fast searching
		TSet<FName> FilterPackagePaths(Filter.PackagePaths);
		TSet<FTopLevelAssetPath> FilterClassNames(Filter.ClassPaths);
		TSet<FName> FilterPackageNames(Filter.PackageNames);

		int32 TotalTasks = 0;
		if (FilterPackagePaths.Num() > 0)
		{
			++TotalTasks;
		}
		//if (FilterClassNames.Num()) ++TotalTasks;
		if (FilterPackageNames.Num() > 0)
		{
			++TotalTasks;
		}

		ECB_LOG(Display, TEXT("CacheAssets start...%d PackagePaths; %d ClassNames; %d PackageNames => %d tasks)"), FilterPackagePaths.Num(), FilterClassNames.Num(), FilterPackageNames.Num(), TotalTasks);

		const int32 MinPreparePathsToShowDialog = 50;
		const int32 TotalPathsToPrepare = FilterPackagePaths.Num() + FilterClassNames.Num() + FilterPackageNames.Num();
		const bool bShowPrepareDialog = TotalPathsToPrepare >= MinPreparePathsToShowDialog;

		FScopedSlowTask PrepareTask(TotalTasks, LOCTEXT("ScanningAssets_Prepare", "Building Asset List..."));
		if (bShowPrepareDialog)
		{
			PrepareTask.MakeDialog(/*bShowCancelDialog =*/ true);
		}
		FText ProgressFormat = LOCTEXT("ScanningAssets_Prepare", "Building Asset List...{0}");

		bool bPrepareTaskCanceled = false;

		const bool bDealyParse = true;

		// On disk package paths
		//PrepareTask.EnterProgressFrame(1.f);
		if (!bPrepareTaskCanceled && FilterPackagePaths.Num())
		{
			float Progress = 1.f / FilterPackagePaths.Num();
			for (FName PackagePath : FilterPackagePaths)
			{
				if (bShowPrepareDialog)
				{
					PrepareTask.EnterProgressFrame(Progress, FText::Format(ProgressFormat, FText::FromString(FPaths::GetBaseFilename(PackagePath.ToString()))));
					if (PrepareTask.ShouldCancel())
					{
						bPrepareTaskCanceled = true;
						break;
					}
				}
				
				if (const TArray<FExtAssetData*>* PathAssets = GetOrCacheAssetsByFolder(PackagePath, bDealyParse).Find(PackagePath))
				{
					FilteredAssets.Append(*PathAssets);
				}
			}
		}

		// On disk classes
		//PrepareTask.EnterProgressFrame(1.f);
		if (!bPrepareTaskCanceled && FilterClassNames.Num())
		{
#if ECB_TODO // ALready covered in expanded filters step?
			float Progress = 1.f / FilterPackagePaths.Num();
			for (const FName& PackagePath : FilterPackagePaths)
			{
				PrepareTask.EnterProgressFrame(Progress);
				if (PrepareTask.ShouldCancel())
				{
					bPrepareTaskCanceled = true;
					break;
				}

				TSet<FName> SubPaths;
				GetOrCacheSubPaths(PackagePath, SubPaths, /*bInRecurse =*/ true);

				for (const FName& SubPath : SubPaths)
				{
					if (PrepareTask.ShouldCancel())
					{
						bPrepareTaskCanceled = true;
						break;
					}

					if (PrepareTask.ShouldCancel())
					{
						bPrepareTaskCanceled = true;
						break;
					}

					if (const TArray<FExtAssetData*>* SubPathAssets = GetOrCacheAssetsByFolder(SubPath, bDealyParse).Find(SubPath))
					{
						FilteredAssets.Append(*SubPathAssets);
					}
				}
			}
#endif
		}

		// On disk package names
		//PrepareTask.EnterProgressFrame(1.f);
		if (!bPrepareTaskCanceled && FilterPackageNames.Num())
		{
			float Progress = 1.f / FilterPackageNames.Num();
			for (FName PackageName : FilterPackageNames)
			{
				if (bShowPrepareDialog)
				{
					PrepareTask.EnterProgressFrame(Progress, FText::Format(ProgressFormat, FText::FromString(FPaths::GetBaseFilename(PackageName.ToString()))));
					if (PrepareTask.ShouldCancel())
					{
						bPrepareTaskCanceled = true;
						break;
					}
				}

				const FName* FilePath = State.CachedPackageNameToFilePathMap.Find(PackageName);
				if (FilePath != nullptr)
				{
					if (FExtAssetData* AssetData = GetOrCacheAssetByFilePath(*FilePath, bDealyParse))
					{
						FilteredAssets.Add(AssetData);
					}
				}
			}
		}

		if (bPrepareTaskCanceled)
		{
			return;
		}
	}

	// Parsing
	TSet<FExtAssetData*> ToParseAssets;
	for (FExtAssetData* AssetData : FilteredAssets)
	{
		if (!AssetData->IsParsed())
		{
			ToParseAssets.Add(AssetData);
		}
	}

	PrepareAssetsTime = FPlatformTime::Seconds() - ParseAssetsStartTime;

	const int32 NumToParseAssets = ToParseAssets.Num();
	if (NumToParseAssets > 0)
	{
		const int32 MinAssetToShowDialog = 50;
		const bool bShowProgress = NumToParseAssets >= MinAssetToShowDialog;

		// Restore keyboard focus steal by SlowTask dialog
		TSharedPtr<SWidget> PreviousFocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
		{
			FScopedSlowTask SlowTask(NumToParseAssets, FText::Format(LOCTEXT("ScanningAssets_Scan", "Scanning {0} Assets..."), FText::AsNumber(NumToParseAssets)));
			FText ScanProgressFormat = LOCTEXT("ScanningAssets_ScanFormatText", "Scanning {0} Assets...{1}");

			if (bShowProgress)
			{
				SlowTask.MakeDialog(/*bShowCancelDialog =*/ true);
			}

			for (FExtAssetData* AssetData : ToParseAssets)
			{
				AssetData->Parse();

				// Update cache for class and asset tags
				State.CacheNewAsset(AssetData);

				if (bShowProgress && SlowTask.ShouldCancel())
				{
					break;
				}
				if (bShowProgress)
				{
					SlowTask.EnterProgressFrame(1.f, FText::Format(ScanProgressFormat, FText::AsNumber(NumToParseAssets), FText::FromString(FPaths::GetBaseFilename(AssetData->PackageFilePath.ToString()))));
				}
			}			
		}
		if (bShowProgress && PreviousFocusedWidget.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(PreviousFocusedWidget, EFocusCause::SetDirectly);
		}
	}

	ECB_LOG(Display, TEXT("CacheAssets completed in %0.4f seconds. (Prepare in %0.4f secons)"), FPlatformTime::Seconds() - GetAssetsStartTime, PrepareAssetsTime);
}

void FExtAssetRegistry::CacheAssetsAsync(const struct FARFilter& InFilter)
{
	double GetAssetsStartTime = FPlatformTime::Seconds();

	double ParseAssetsStartTime = FPlatformTime::Seconds();
	double PrepareAssetsTime = 0.;

	if (!IsFilterValid(InFilter, /*bAllowRecursion = */ true) || InFilter.IsEmpty())
	{
		return;
	}

	TSet<FName> FilteredAssetPaths;

	{
		// Expand recursion on filter
		FARFilter Filter;
		ExpandRecursiveFilter(InFilter, Filter);

		// Verify filter input. If all assets are needed, use GetAllAssets() instead.
		if (!IsFilterValid(Filter, /*bAllowRecursion = */ false) || InFilter.IsEmpty())
		{
			return;
		}

		// Prepare a set of each filter component for fast searching
		TSet<FName> FilterPackagePaths(Filter.PackagePaths);
		TSet<FTopLevelAssetPath> FilterClassNames(Filter.ClassPaths);
		TSet<FName> FilterPackageNames(Filter.PackageNames);

		ECB_LOG(Display, TEXT("CacheAssets start...%d PackagePaths; %d ClassNames; %d PackageNames)"), FilterPackagePaths.Num(), FilterClassNames.Num(), FilterPackageNames.Num());

		// On disk package paths
		if (FilterPackagePaths.Num())
		{
			const auto& CachedFilePathsByFolder = State.CachedFilePathsByFolder;
			for (FName PackagePath : FilterPackagePaths)
			{
				if (const TArray<FName>* FoundAssetPaths =  CachedFilePathsByFolder.Find(PackagePath))
				{	
					FilteredAssetPaths.Append(*FoundAssetPaths);
				}
			}
		}

		// On disk classes
		//PrepareTask.EnterProgressFrame(1.f);
		if (FilterClassNames.Num())
		{
#if ECB_TODO // ALready covered in expanded filters step?
			for (const FName& ClassName : FilterClassNames)
			{
			}
#endif
		}

		// On disk package names
		if (FilterPackageNames.Num())
		{
			for (FName PackageName : FilterPackageNames)
			{
				const FName* FilePath = State.CachedPackageNameToFilePathMap.Find(PackageName);
				if (FilePath != nullptr)
				{
					FilteredAssetPaths.Add(*FilePath);
				}
			}
		}
	}

	// To Parsing
	TSet<FName> ToParseAssets;

	if (FilteredAssetPaths.Num() > 0)
	{
		for (const FName& AssetPathName : FilteredAssetPaths)
		{
			if (FExtAssetData* AssetData = GetCachedAssetByFilePath(AssetPathName))
			{
				if (AssetData->IsParsed())
				{
					continue;
				}
			}
			ToParseAssets.Add(AssetPathName);
		}
	}

	StartOrCancelAssetGathering(ToParseAssets.Array(), FilteredAssetPaths.Num());

	ECB_LOG(Display, TEXT("CacheAssetsAsync completed in %0.4f seconds. Kick off %d assets to gathering. (Prepare in %0.4f secons)"), FPlatformTime::Seconds() - GetAssetsStartTime, ToParseAssets.Num());
}

void FExtAssetRegistry::GetOrCacheSubPaths(const FName& InBasePath, TSet<FName>& OutSubPaths, bool bInRecurse)
{
	auto& CachedSubPaths = State.CachedSubPaths;

	if (!bInRecurse && CachedSubPaths.Contains(InBasePath))
	{
		OutSubPaths.Append(*CachedSubPaths.Find(InBasePath));
		return;
	}

	if (bInRecurse)
	{
		TSet<FName> SubPaths;
		FContentPathUtil::ExpandCachedSubsRecursively(CachedSubPaths, InBasePath, SubPaths);
		for (const FName& SubPath : SubPaths)
		{
			OutSubPaths.Add(SubPath);
		}
		//FExtPackageUtils::GetDirectoriesRecursively(InBasePath.ToString(), SubPaths);
	}
	else
	{
		TArray<FString> SubPaths;
		FExtPackageUtils::GetDirectories(InBasePath.ToString(), SubPaths);
		for (const FString& SubPath : SubPaths)
		{
			OutSubPaths.Add(*SubPath);
		}
	}

	// Cancel background folder gathering result?
	if (bInRecurse && BackgroundGatheringFolders.Contains(InBasePath))
	{
		BackgroundGatheringFolders.Remove(InBasePath);
	}
}

void FExtAssetRegistry::GetCachedSubPaths(const FName& InBasePath, TSet<FName>& OutPathList, bool bInRecurse)
{
	auto& CachedSubPaths = State.CachedSubPaths;

	if (bInRecurse)
	{
		if (IsFolderBackgroundGathering(InBasePath.ToString()))
		{
			FString BasePath = InBasePath.ToString();
			for (auto It = CachedSubPaths.CreateIterator(); It; ++It)
			{
				const FName ParentPath = It->Key;
				if (FPathsUtil::IsSubOrSamePath(ParentPath.ToString(), BasePath))
				{
					OutPathList.Append(It->Value);
				}
			}
		}
		else if (CachedSubPaths.Contains(InBasePath))
		{
			FContentPathUtil::ExpandCachedSubsRecursively(CachedSubPaths, InBasePath, OutPathList);
			//OutPathList.Append(*CachedSubPaths.Find(InBasePath));
		}
	}

	if (!bInRecurse && CachedSubPaths.Contains(InBasePath))
	{
		OutPathList.Append(*CachedSubPaths.Find(InBasePath));
	}
}

FExtAssetDependencyInfo FExtAssetRegistry::GetOrCacheAssetDependencyInfo(const FExtAssetData& InAssetData, bool bShowProgess)
{
	const FName PackageFilePath = InAssetData.PackageFilePath;

	auto& CachedDependencyInfoByFilePath = State.CachedDependencyInfoByFilePath;

	if (FExtAssetDependencyInfo* FoundDependencyInfo = CachedDependencyInfoByFilePath.Find(PackageFilePath))
	{
		return *FoundDependencyInfo;
	}

	FExtAssetDependencyInfo DependencyInfo;

	if (!InAssetData.IsUAsset())
	{
		if (InAssetData.PackageName != NAME_None)
		{
			DependencyInfo.AssetStatus = FExtPackageUtils::DoesPackageExist(InAssetData.PackageName.ToString()) ? EDependencyNodeStatus::Valid : EDependencyNodeStatus::Missing;
		}
		else
		{
			DependencyInfo.AssetStatus = EDependencyNodeStatus::Invalid;
		}
	}
	else if (!InAssetData.IsValid())
	{
		DependencyInfo.AssetStatus = EDependencyNodeStatus::Invalid;
	}
	else if (!InAssetData.HasContentRoot())
	{
		DependencyInfo.AssetStatus = EDependencyNodeStatus::Unknown;
	}
	else
	{
		FString AssetContentDir = InAssetData.AssetContentRoot.ToString();
		auto& AssetContentType = InAssetData.AssetContentType;
		DependencyInfo.AssetStatus = FExtAssetDependencyWalker::GatherDependencies(InAssetData, AssetContentDir, AssetContentType, DependencyInfo, bShowProgess);
	}

	if (PackageFilePath != NAME_None && DependencyInfo.AssetStatus != EDependencyNodeStatus::AbortGathering)
	{
		State.CachedDependencyInfoByFilePath.Add(PackageFilePath, DependencyInfo);
	}

	return DependencyInfo;
}

const FExtAssetDependencyInfo* FExtAssetRegistry::GetCachedAssetDependencyInfo(const FExtAssetData& InAssetData) const
{
	const FName PackageFilePath = InAssetData.PackageFilePath;

	auto& CachedDependencyInfoByFilePath = State.CachedDependencyInfoByFilePath;

	if (const FExtAssetDependencyInfo* FoundDependencyInfo = CachedDependencyInfoByFilePath.Find(PackageFilePath))
	{
		return FoundDependencyInfo;
	}

	return nullptr;
}

bool FExtAssetRegistry::RemoveCachedAssetDependencyInfo(const FExtAssetData& InAssetData)
{
	auto& CachedDependencyInfoByFilePath = State.CachedDependencyInfoByFilePath;
	const FName PackageFilePath = InAssetData.PackageFilePath;
	int32 NumRemoved = CachedDependencyInfoByFilePath.Remove(PackageFilePath);
	return NumRemoved > 0;
}

bool FExtAssetRegistry::IsCachedDependencyInfoInValid(const FExtAssetData& InAssetData, bool bTreatSoftErrorAsInvalid /*= false*/) const
{
	if (const FExtAssetDependencyInfo* CachedDependencyInfo = GetCachedAssetDependencyInfo(InAssetData))
	{
		if (bTreatSoftErrorAsInvalid)
		{
			return CachedDependencyInfo->AssetStatus != EDependencyNodeStatus::Valid;
		}
		else
		{
			return CachedDependencyInfo->AssetStatus != EDependencyNodeStatus::Valid && CachedDependencyInfo->AssetStatus != EDependencyNodeStatus::ValidWithSoftReferenceIssue;
		}
	}
	return false;
}

FExtAssetData* FExtAssetRegistry::GetOrCacheAssetByFilePath(const FName& InFilePath, bool bDelayParse/* = false*/)
{
	auto& CachedAssetsByFilePath = State.CachedAssetsByFilePath;
	if (!CachedAssetsByFilePath.Contains(InFilePath))
	{
		if (!IFileManager::Get().FileExists(*InFilePath.ToString()))
		{
			return nullptr;
		}

		// New Asset Data
		FExtAssetData* NewAssetData = new FExtAssetData(InFilePath.ToString(), bDelayParse);
		//if (NewAssetData->IsValid())
		{
			State.CacheNewAsset(NewAssetData);
		}
		return NewAssetData;
	}
	
	FExtAssetData** FoundAssetData = CachedAssetsByFilePath.Find(InFilePath);
	if (FoundAssetData)
	{
		if (!bDelayParse && !((*FoundAssetData)->IsParsed()))
		{
			(*FoundAssetData)->Parse();
		}

		if ((*FoundAssetData)->IsParsed())
		{
			State.ReCacheParsedAsset(*FoundAssetData);
		}

		return *FoundAssetData;
	}
	return NULL;
}

TMap<FName, TArray<FExtAssetData*>>& FExtAssetRegistry::GetOrCacheAssetsByFolder(const FName& InFolder, bool bDelayParse)
{
	auto& CachedAssetsByFolder = State.CachedAssetsByFolder;

	if (!CachedAssetsByFolder.Contains(InFolder))
	{
		TArray<FExtAssetData*> Assets;
		{
			auto& CachedFilePathsByFolder = State.CachedFilePathsByFolder;
			if (!CachedFilePathsByFolder.Contains(InFolder))
			{
				TArray<FName> PathList;
				FExtPackageUtils::GetAllPackages(InFolder, PathList);
				CachedFilePathsByFolder.Add(InFolder, PathList);
			}

			auto& PathList = CachedFilePathsByFolder[InFolder];

			for (const auto& FilePath : PathList)
			{
				FExtAssetData* CachedAsset = GetOrCacheAssetByFilePath(FilePath, bDelayParse);
				if (CachedAsset/* && CachedAsset->IsValid()*/)
				{
					Assets.Emplace(CachedAsset);
				}
			}
		}
		CachedAssetsByFolder.Add(InFolder, Assets);
	}
	return CachedAssetsByFolder;
}

TMap<FName, TArray<FExtAssetData*>>& FExtAssetRegistry::GetOrCacheAssetsByClass(const TSet<FName>& InPaths, bool bRecursively)
{
	for (const FName& InPath : InPaths)
	{
		GetOrCacheAssetsByFolder(InPath);

		if (bRecursively)
		{
			TSet<FName> SubPaths;
			GetOrCacheSubPaths(InPath, SubPaths, /*bInRecurse =*/ true);

			for (const FName& SubPath : SubPaths)
			{
				GetOrCacheAssetsByFolder(SubPath);
			}
		}
	}
	return State.CachedAssetsByClass;
}

FExtAssetData* FExtAssetRegistry::GetCachedAssetByFilePath(const FName& InFilePath)
{
	auto& CachedAssetsByFilePath = State.CachedAssetsByFilePath;
	FExtAssetData** FoundAssetData = CachedAssetsByFilePath.Find(InFilePath);
	if (FoundAssetData)
	{
		return *FoundAssetData;
	}
	return NULL;
}

FExtAssetData* FExtAssetRegistry::GetCachedAssetByPackageName(const FName& InPackageName)
{
	if (const FName* FilePath = State.CachedPackageNameToFilePathMap.Find(InPackageName))
	{
		return GetCachedAssetByFilePath(*FilePath);
	}
	return NULL;
}

TArray<FExtAssetData*> FExtAssetRegistry::GetCachedAssetsByFolder(const FName& InDirectory)
{
	if (State.CachedAssetsByFolder.Find(InDirectory))
	{
		return State.CachedAssetsByFolder[InDirectory];
	}
	return TArray<FExtAssetData*>();
}

TArray<FExtAssetData*> FExtAssetRegistry::GetCachedAssetsByClass(const FName& InClass)
{
	if (State.CachedAssetsByClass.Find(InClass))
	{
		return State.CachedAssetsByClass[InClass];
	}
	return TArray<FExtAssetData*>();
}

const TMap<FName, FName>& FExtAssetRegistry::GetCachedAssetContentRootConfigDirs() const
{
	return State.CachedAssetContentRootConfigDirs;
}

const TMap<FName, FName>& FExtAssetRegistry::GetCachedAssetContentRootHostDirs() const
{
	return State.CachedAssetContentRootHosts;
}

#if ECB_WIP_THUMB_CACHE
const FObjectThumbnail* FExtAssetRegistry::FindCachedThumbnail(const FName& InFilePath)
{
	return State.CachedThumbnails.Find(InFilePath);
}
#endif

bool FExtAssetRegistry::IsEmptyFolder(const FString& FolderPath)
{
	auto& CachedEmptyFolders = State.CachedEmptyFoldersStatus;

	FName FolderName(*FolderPath);
	if (CachedEmptyFolders.Contains(FolderName))
	{
		return CachedEmptyFolders[FolderName];
	}

	bool bEmptyFolder = !FExtPackageUtils::HasPackages(FolderPath, /*bRecursively*/ true);

	// cache it
	CachedEmptyFolders.Add(FolderName, bEmptyFolder);

	return bEmptyFolder;
}

bool FExtAssetRegistry::IsFolderAssetsFullyParsed(const FString& FolderPath) const
{
	const auto& CachedFullyParsedFolders = State.CachedFullyParsedFolders;
	return CachedFullyParsedFolders.Contains(*FolderPath);
}

bool FExtAssetRegistry::CombineRootContentPaths()
{
	return FContentPathUtil::CombineAndSortRootContentPaths(RootContentPaths, RootContentPaths) > 0;
}

void FExtAssetRegistry::CacheRootContentPathInfo()
{
	const bool bHideContentName = true;

	RootContentPathsInfo.Empty();
	for (const FString& RootContentPath : RootContentPaths)
	{
		FExtAssetData::EContentType ContentType = FExtAssetData::EContentType::Orphan;

		// Strip off any trailing forward slashes
		FString DisplayFolderPath = RootContentPath;
		FPaths::NormalizeDirectoryName(DisplayFolderPath);

		FString ContentFolderPath;

		static const FString VaultCacheContentFolderName = TEXT("/data/Content");
		static const FString VaultCacheDataFolderName = TEXT("/data");
		static const FString ProjectOrPluginContentFolderName = TEXT("/Content");

		FString TmpAssetContentRoot;
		FString TmpRelativePath;

		if (DisplayFolderPath.EndsWith(VaultCacheContentFolderName))
		{
			FString VaultCacheFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(DisplayFolderPath, TEXT("../..")));
			if (FExtContentDirFinder::FindWithFolder(VaultCacheFolder, TEXT("manifest"), /*bExtension*/ false))
			{
				ContentFolderPath = DisplayFolderPath;
				DisplayFolderPath = VaultCacheFolder;
				ContentType = FExtAssetData::EContentType::VaultCache;
			}
		}
		else if (DisplayFolderPath.EndsWith(VaultCacheDataFolderName))
		{
			FString VaultCacheFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(DisplayFolderPath, TEXT("..")));
			if (FExtContentDirFinder::FindWithFolder(VaultCacheFolder, TEXT("manifest"), /*bExtension*/ false))
			{
				ContentFolderPath = DisplayFolderPath + TEXT("/Content");
				DisplayFolderPath = VaultCacheFolder;
				ContentType = FExtAssetData::EContentType::VaultCache;
			}
		}
		else if (DisplayFolderPath.EndsWith(ProjectOrPluginContentFolderName))
		{
			ContentFolderPath = DisplayFolderPath;

			FString ProjectOrPluginFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(DisplayFolderPath, TEXT("..")));
			if (FExtContentDirFinder::FindWithFolder(ProjectOrPluginFolder, TEXT(".uplugin"), /*bExtension*/ true))
			{
				DisplayFolderPath = ProjectOrPluginFolder;
				ContentType = FExtAssetData::EContentType::Plugin;
			}
			else if (FExtContentDirFinder::FindWithFolder(ProjectOrPluginFolder, TEXT(".uproject"), /*bExtension*/ true))
			{
				DisplayFolderPath = ProjectOrPluginFolder;
				ContentType = FExtAssetData::EContentType::Project;
			}
		}
		////////////////////////////////////////////
		// 
		else if (FExtContentDirFinder::FindWithFolder(DisplayFolderPath, TEXT("manifest"), /*bExtension*/ false))
		{
			ContentFolderPath = DisplayFolderPath + VaultCacheContentFolderName;
			ContentType = FExtAssetData::EContentType::VaultCache;
		}
		else if (FExtContentDirFinder::FindWithFolder(DisplayFolderPath, TEXT(".uplugin"), /*bExtension*/ true))
		{
			ContentFolderPath = DisplayFolderPath + ProjectOrPluginContentFolderName;
			ContentType = FExtAssetData::EContentType::Plugin;
		}
		else if (FExtContentDirFinder::FindWithFolder(DisplayFolderPath, TEXT(".uproject"), /*bExtension*/ true))
		{
			ContentFolderPath = DisplayFolderPath + ProjectOrPluginContentFolderName;
			ContentType = FExtAssetData::EContentType::Project;
		}
		
		// Partial Project
		if (ContentType == FExtAssetData::EContentType::Orphan && FExtContentDirFinder::FindFolder(DisplayFolderPath, TEXT("Content"), TmpAssetContentRoot, TmpRelativePath))
		{
			ContentFolderPath = TmpAssetContentRoot;
			DisplayFolderPath = bHideContentName ? FPaths::ConvertRelativePathToFull(FPaths::Combine(ContentFolderPath, TEXT(".."))) : ContentFolderPath;
			ContentType = FExtAssetData::EContentType::PartialProject;
		}

		FString DisplayFolderName = FPaths::GetPathLeaf(DisplayFolderPath);
		FText LocalizedFolderName = DisplayFolderName.IsEmpty() ? FText::FromString(DisplayFolderPath) : FText::FromString(DisplayFolderName);

		FPaths::NormalizeDirectoryName(ContentFolderPath);

		FRootContentPathInfo PathInfo;
		PathInfo.ContentType = ContentType;
		PathInfo.AssetContentRoot = ContentFolderPath;
		PathInfo.DisplayName = LocalizedFolderName;

		RootContentPathsInfo.FindOrAdd(RootContentPath) = PathInfo;
	}

	
}

void FExtAssetRegistry::CacheFolderColor(const FName& InAssetContentRoot)
{
	if (const FName* ConfigDirName = State.CachedAssetContentRootConfigDirs.Find(InAssetContentRoot))
	{
		if (!ConfigDirName->IsNone())
		{
			TMap<FName, FLinearColor> FilePathColors;
			if (FExtConfigUtil::LoadPathColors(InAssetContentRoot.ToString(), ConfigDirName->ToString(), FilePathColors))
			{
				TArray<FName> FolderPaths;
				FilePathColors.GenerateKeyArray(FolderPaths);
				State.CachedFolderColorIndices.FindOrAdd(InAssetContentRoot, FolderPaths);
				State.CachedFolderColors.Append(FilePathColors);
			}
		}
	} 
}

void FExtAssetRegistry::GetSubClasses(const TArray<FTopLevelAssetPath>& InClassPaths, const TSet<FTopLevelAssetPath>& ExcludedClassNames, TSet<FTopLevelAssetPath>& SubClassNames) const
{
	struct FClassCacheHelper
	{
		void UpdateTemporaryCaches()
		{
			// And add all in-memory classes at request time
			TSet<FTopLevelAssetPath> InMemoryClassNames;

			for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
			{
				UClass* Class = *ClassIt;

				if (!Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
				{
					FTopLevelAssetPath ClassPathName = Class->GetClassPathName();
					if (Class->GetSuperClass())
					{
						FTopLevelAssetPath SuperClassName = Class->GetSuperClass()->GetClassPathName();
						TSet<FTopLevelAssetPath>& ChildClasses = TempReverseInheritanceMap.FindOrAdd(SuperClassName);
						ChildClasses.Add(ClassPathName);

						TempCachedInheritanceMap.Add(ClassPathName, SuperClassName);
					}
					else
					{
						// This should only be true for a small number of CoreUObject classes
						TempCachedInheritanceMap.Add(ClassPathName, FTopLevelAssetPath());
					}

					// Add any implemented interfaces to the reverse inheritance map, but not to the forward map
					for (int32 i = 0; i < Class->Interfaces.Num(); ++i)
					{
						UClass* InterfaceClass = Class->Interfaces[i].Class;
						if (InterfaceClass) // could be nulled out by ForceDelete of a blueprint interface
						{
							TSet<FTopLevelAssetPath>& ChildClasses = TempReverseInheritanceMap.FindOrAdd(InterfaceClass->GetClassPathName());
							ChildClasses.Add(ClassPathName);
						}
					}

					InMemoryClassNames.Add(ClassPathName);
				}
			}

			// Add non-native classes to reverse map
			for (auto ClassNameIt = TempCachedInheritanceMap.CreateConstIterator(); ClassNameIt; ++ClassNameIt)
			{
				const FTopLevelAssetPath& ClassName = ClassNameIt.Key();
				if (!InMemoryClassNames.Contains(ClassName))
				{
					const FTopLevelAssetPath& ParentClassName = ClassNameIt.Value();
					if (ParentClassName.IsNull())
					{
						TSet<FTopLevelAssetPath>& ChildClasses = TempReverseInheritanceMap.FindOrAdd(ParentClassName);
						ChildClasses.Add(ClassName);
					}
				}
			}
		}

		void ClearTemporaryCaches()
		{
			TempReverseInheritanceMap.Empty();
		}

		/** A temporary fully cached list including native classes */
		TMap<FTopLevelAssetPath, FTopLevelAssetPath> TempCachedInheritanceMap;

		/** A reverse map of TempCachedInheritanceMap, only kept during temp caching */
		TMap<FTopLevelAssetPath, TSet<FTopLevelAssetPath>> TempReverseInheritanceMap;

	} ClassCacheHelper;

	ClassCacheHelper.UpdateTemporaryCaches();

	for (auto& ClassPath : InClassPaths)
	{
		// Now find all subclass names
		TSet<FTopLevelAssetPath> ProcessedClassNames;
		GetSubClasses_Recursive(ClassPath, SubClassNames, ProcessedClassNames, ClassCacheHelper.TempReverseInheritanceMap, ExcludedClassNames);
	}

	ClassCacheHelper.ClearTemporaryCaches();
}

void FExtAssetRegistry::GetSubClasses_Recursive(const FTopLevelAssetPath& InClassName, TSet<FTopLevelAssetPath>& SubClassNames, TSet<FTopLevelAssetPath>& ProcessedClassNames, const TMap<FTopLevelAssetPath, TSet<FTopLevelAssetPath>>& ReverseInheritanceMap, const TSet<FTopLevelAssetPath>& ExcludedClassNames) const
{
	if (ExcludedClassNames.Contains(InClassName))
	{
		// This class is in the exclusion list. Exclude it.
	}
	else if (ProcessedClassNames.Contains(InClassName))
	{
		// This class has already been processed. Ignore it.
	}
	else
	{
		SubClassNames.Add(InClassName);
		ProcessedClassNames.Add(InClassName);

		const TSet<FTopLevelAssetPath>* FoundSubClassNames = ReverseInheritanceMap.Find(InClassName);
		if (FoundSubClassNames)
		{
			for (const FTopLevelAssetPath& ClassName : (*FoundSubClassNames))
			{
				GetSubClasses_Recursive(ClassName, SubClassNames, ProcessedClassNames, ReverseInheritanceMap, ExcludedClassNames);
			}
		}
	}
}

void FExtAssetRegistry::StopAllBackgroundGathering()
{
	if (BackgroundFolderGatherer.IsValid())
	{
		BackgroundFolderGatherer->EnsureCompletion();
		BackgroundFolderGatherer.Reset();
	}
	if (BackgroundAssetGatherer.IsValid())
	{
		BackgroundAssetGatherer->AddBatchToGather(TArray<FName>());
		BackgroundAssetGatherer->EnsureCompletion();
		BackgroundAssetGatherer.Reset();
	}

	BackgroundGatheringSubFolder = NAME_None;
	BackgroundGatheringFolders.Empty();
}

bool FExtAssetRegistry::IsFilterValid(const FARFilter& Filter, bool bAllowRecursion)
{
	for (int32 NameIdx = 0; NameIdx < Filter.PackageNames.Num(); ++NameIdx)
	{
		if (Filter.PackageNames[NameIdx] == NAME_None)
		{
			return false;
		}
	}

	for (int32 PathIdx = 0; PathIdx < Filter.PackagePaths.Num(); ++PathIdx)
	{
		if (Filter.PackagePaths[PathIdx] == NAME_None)
		{
			return false;
		}
	}

	for (int32 ObjectPathIdx = 0; ObjectPathIdx < Filter.SoftObjectPaths.Num(); ++ObjectPathIdx)
	{
		if (Filter.SoftObjectPaths[ObjectPathIdx].IsNull())
		{
			return false;
		}
	}

	for (int32 ClassIdx = 0; ClassIdx < Filter.ClassPaths.Num(); ++ClassIdx)
	{
		if (!Filter.ClassPaths[ClassIdx].IsValid())
		{
			return false;
		}
	}

	for (auto FilterTagIt = Filter.TagsAndValues.CreateConstIterator(); FilterTagIt; ++FilterTagIt)
	{
		if (FilterTagIt.Key() == NAME_None)
		{
			return false;
		}
	}

	if (!bAllowRecursion && (Filter.bRecursiveClasses || Filter.bRecursivePaths))
	{
		return false;
	}

	return true;
}

void FExtAssetRegistry::ExpandRecursiveFilter(const FARFilter& InFilter, FARFilter& ExpandedFilter)
{
	TSet<FName> FilterPackagePaths;
	TSet<FTopLevelAssetPath> FilterClassNames;
	const int32 NumFilterPackagePaths = InFilter.PackagePaths.Num();
	const int32 NumFilterClasses = InFilter.ClassPaths.Num();

	ExpandedFilter = InFilter;

	for (int32 PathIdx = 0; PathIdx < NumFilterPackagePaths; ++PathIdx)
	{
		FilterPackagePaths.Add(InFilter.PackagePaths[PathIdx]);
	}

	if (InFilter.bRecursivePaths)
	{
		// Add subpaths to all the input paths to the list
		for (int32 PathIdx = 0; PathIdx < NumFilterPackagePaths; ++PathIdx)
		{
			GetOrCacheSubPaths(InFilter.PackagePaths[PathIdx], FilterPackagePaths, /*bRecursely = */ true);
		}
	}

	ExpandedFilter.bRecursivePaths = false;
	ExpandedFilter.PackagePaths = FilterPackagePaths.Array();

	if (InFilter.bRecursiveClasses)
	{
		if (InFilter.RecursiveClassPathsExclusionSet.Num() > 0 && InFilter.ClassPaths.Num() == 0)
		{
			// Build list of all classes then remove excluded classes
			TArray<FTopLevelAssetPath> ClassNamesObject;
			ClassNamesObject.Add(FTopLevelAssetPath(TEXT("/Script/CoreUObject"), NAME_Class));

			// GetSubClasses includes the base classes
			GetSubClasses(ClassNamesObject, InFilter.RecursiveClassPathsExclusionSet, FilterClassNames);
		}
		else
		{
			// GetSubClasses includes the base classes
			GetSubClasses(InFilter.ClassPaths, InFilter.RecursiveClassPathsExclusionSet, FilterClassNames);
		}
	}
	else
	{
		for (int32 ClassIdx = 0; ClassIdx < NumFilterClasses; ++ClassIdx)
		{
			FilterClassNames.Add(InFilter.ClassPaths[ClassIdx]);
		}
	}

	ExpandedFilter.ClassPaths = FilterClassNames.Array();
	ExpandedFilter.bRecursiveClasses = false;
	ExpandedFilter.RecursiveClassPathsExclusionSet.Empty();
}

TOptional<FAssetImportInfo> FExtAssetRegistry::ExtractAssetImportInfo(const FExtAssetData& AssetData) const
{
	static const FName LegacySourceFilePathName("SourceFile");

	FAssetDataTagMapSharedView::FFindTagResult Result = AssetData.TagsAndValues.FindTag(UObject::SourceFileTagName());
	if (Result.IsSet())
	{
		return FAssetImportInfo::FromJson(Result.GetValue());
	}
	else
	{
		FAssetDataTagMapSharedView::FFindTagResult ResultLegacy = AssetData.TagsAndValues.FindTag(LegacySourceFilePathName);
		if (ResultLegacy.IsSet())
		{
			FAssetImportInfo Legacy;
			Legacy.Insert(ResultLegacy.GetValue());
			return Legacy;
		}
		else
		{
			return TOptional<FAssetImportInfo>();
		}
	}
}

void FExtAssetRegistry::PrintCacheStatus()
{
	ECB_LOG(Display, TEXT("  FExtObjectThumbnailPool"));
	ECB_LOG(Display, TEXT("  ================================="));
	ECB_LOG(Display, TEXT("  MaxNumThumbnailsInPool:      %d"), FExtContentBrowserSingleton::GetThumbnailPool().NumInPool);
	ECB_LOG(Display, TEXT("  Num Used in Pool:            %d"), FExtContentBrowserSingleton::GetThumbnailPool().Pool.Num());
	ECB_LOG(Display, TEXT("  Pool Memory Ussage:         %dk"), FExtContentBrowserSingleton::GetThumbnailPool().GetAllocatedSize() / 1024);
	ECB_LOG(Display, TEXT("  ---------------------------------"));

	ECB_LOG(Display, TEXT("  FExtAssetRegistry"));
	ECB_LOG(Display, TEXT("  ================================="));
	ECB_LOG(Display, TEXT("  RootContentPaths:           %d"), RootContentPaths.Num());
	ECB_LOG(Display, TEXT("  BackgroundGatheringFolders: %d"), BackgroundGatheringFolders.Num());
	ECB_LOG(Display, TEXT("  BackgroundGatheringSubFolder: %s"), *BackgroundGatheringSubFolder.ToString());
	ECB_LOG(Display, TEXT("  ---------------------------------"));

	State.PrintStatus();
}

void FExtAssetRegistry::ClearCache()
{
	State.Reset();
}

void FExtAssetRegistry::UpdateCacheByRemovedFolders(const TArray<FString>& InRemovedFolders)
{
	State.UpdateCacheByRemovedFolders(InRemovedFolders);
}

void FExtAssetRegistry::StartFolderGathering(const FString& InGatheringFolder)
{
	FName ToGatherFolderName(*InGatheringFolder);
	BackgroundGatheringFolders.Add(ToGatherFolderName);

#if ECB_WIP_IMPORT_ORPHAN

	FString RootContentPath;
	QueryRootContentPathFromFilePath(InGatheringFolder, RootContentPath);
	ECB_LOG(Display, TEXT("[StartFolderGathering] %s, FoundRoot: %s"), *InGatheringFolder, *RootContentPath);

#endif

#if ECB_FEA_IGNORE_FOLDERS
	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	TArray<FString> IgnoreFolders = ExtContentBrowserSetting->IgnoreFolders;
	if (ExtContentBrowserSetting->bIgnoreCommonNonContentFolders)
	{
		IgnoreFolders.Append(ExtContentBrowserSetting->CommonNonContentFolders);
	}
	if (ExtContentBrowserSetting->bIgnoreExternalContentFolders)
	{
		IgnoreFolders.Append(ExtContentBrowserSetting->ExternalContentFolders);
	}
	FExtFolderGatherPolicy GatherPolicy(ExtContentBrowserSetting->bIgnoreFoldersStartWithDot, IgnoreFolders);
#endif

	if (!BackgroundFolderGatherer.IsValid())
	{
#if ECB_FEA_IGNORE_FOLDERS
		BackgroundFolderGatherer = MakeShareable(new FExtFolderGatherer(InGatheringFolder, GatherPolicy));
#else
		BackgroundFolderGatherer = MakeShareable(new FExtFolderGatherer(InGatheringFolder));
#endif
	}
	else
	{
#if ECB_FEA_IGNORE_FOLDERS
		BackgroundFolderGatherer->AddSearchFolder(InGatheringFolder, &GatherPolicy);
#else
		BackgroundFolderGatherer->AddSearchFolder(InGatheringFolder);
#endif
	}
}

void FExtAssetRegistry::StartOrCancelAssetGathering(const TArray<FName>& InAssetPaths, int32 TotoalFiltered)
{
	if (!BackgroundAssetGatherer.IsValid())
	{
		BackgroundAssetGatherer = MakeShareable(new FExtAssetGatherer());
	}
	BackgroundAssetGatherer->AddBatchToGather(InAssetPaths);

	bIsGatheringAssets = InAssetPaths.Num() > 0;

	NumFilteredAssets = TotoalFiltered;
	NumToGatherAssets = InAssetPaths.Num();
}


//////////////////////////////////////////////////////
// FExtAssetDataUtil
//
#if 0
bool FExtAssetDataUtil::RemapPackagesToFullFilePath(const FString& InPackageName, const TArray<FString>& InRootPackageNamesToReplace, const FString& InAssetContentDir, FString& OutFullFilePath)
{
	for (const FString& RootNameToFind/*RootNameToFind*/ : InRootPackageNamesToReplace)
	{
		//todo: make sure RootNameToFind ends with "/", e.g. TEXT("/Game/")
		{
		}
		const int32 SkipLen = RootNameToFind.Len();

		if (InPackageName.StartsWith(RootNameToFind))
		{
			FString RemappedFileBase = FPaths::Combine(InAssetContentDir, *FPaths::GetPath(InPackageName.Mid(SkipLen)), *FPaths::GetBaseFilename(InPackageName));

			OutFullFilePath = RemappedFileBase + FExtAssetSupport::AssetPackageExtension;
			if (IFileManager::Get().FileExists(*OutFullFilePath))
			{
				return true;
			}

			OutFullFilePath = RemappedFileBase + FExtAssetSupport::MapPackageExtension;
			if (IFileManager::Get().FileExists(*OutFullFilePath))
			{
				return true;
			}
		}
	}
	return false;
}
#endif

FString FExtAssetDataUtil::GetPluginNameFromAssetContentRoot(const FString& InAssetContentRoot)
{
	auto IsEndOfSearchChar = [](TCHAR C) { return (C == TEXT('/')) || (C == TEXT('\\')); };
	int32 EndPos = InAssetContentRoot.FindLastCharByPredicate(IsEndOfSearchChar);
	int32 StartPos = InAssetContentRoot.FindLastCharByPredicate(IsEndOfSearchChar, EndPos) + 1;
	FString Result = InAssetContentRoot.Mid(StartPos, EndPos - StartPos);

	return FString::Printf(TEXT("/%s/"), *Result);
}

FString FExtAssetDataUtil::GetPackageRootFromFullPackagePath(const FString& InFullPackagePath)
{
	auto IsEndOfSearchChar = [](TCHAR C) { return (C == TEXT('/')) || (C == TEXT('\\')); };
	int32 StartPos = InFullPackagePath.Find(TEXT("/"), ESearchCase::CaseSensitive);
	if (StartPos == 0)
	{
		int32 EndPos = InFullPackagePath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
		if (EndPos > 2)
		{
			FString Result = InFullPackagePath.Mid(1, EndPos - StartPos - 1);
			return FString::Printf(TEXT("/%s/"), *Result);
		}
	}
	static FString EmptyString(TEXT(""));
	return EmptyString;
}

bool FExtAssetDataUtil::RemapGamePackageToFullFilePath(const FString& GameRoot, const FString& InPackageName, const FString& InAssetContentDir, FString& OutFullFilePath, bool bPackageIsFolder)
{
	//static FString GameRoot(TEXT("/Game/"));
	const int32 SkipLen = GameRoot.Len();

	if (InPackageName.StartsWith(GameRoot))
	{
		FString RemappedFileBase = FPaths::Combine(InAssetContentDir, *FPaths::GetPath(InPackageName.Mid(SkipLen)), *FPaths::GetBaseFilename(InPackageName));

		if (!bPackageIsFolder)
		{
			FString AssetFile = RemappedFileBase + FExtAssetSupport::AssetPackageExtension;;
			OutFullFilePath = RemappedFileBase + FExtAssetSupport::AssetPackageExtension;

			if (IFileManager::Get().FileExists(*AssetFile))
			{
				return true;
			}

			FString MapFile = RemappedFileBase + FExtAssetSupport::MapPackageExtension;;
			if (IFileManager::Get().FileExists(*MapFile))
			{
				OutFullFilePath = MapFile;
				return true;
			}
		}
		else
		{
			OutFullFilePath = RemappedFileBase;

			if (IFileManager::Get().DirectoryExists(*OutFullFilePath))
			{
				return true;
			}
		}
	}

	return false;
}


bool FExtAssetDataUtil::SyncToContentBrowser(const FExtAssetData& InAssetData)
{
	if (InAssetData.HasContentRoot())
	{
		FString AssetFileRelativePath = InAssetData.AssetRelativePath.ToString();
		FString AssetContentDir = InAssetData.AssetContentRoot.ToString();
		FString ProjectContentDir = FPaths::ProjectContentDir();
		FString RemappedFilePath = FPaths::Combine(ProjectContentDir, AssetFileRelativePath);

		ECB_LOG(Display, TEXT("[SyncToContentBrowser] ProjectContentDir: %s   => %s relative to %s => %s")
			, *FPaths::ProjectContentDir(), *AssetFileRelativePath, *AssetContentDir, *RemappedFilePath);

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> AssetDatas;
		AssetRegistry.GetAssetsByPackageName(*FPackageName::FilenameToLongPackageName(RemappedFilePath), AssetDatas, /*bIncludeOnlyOnDiskAssets*/false);
		if (AssetDatas.Num() > 0 && AssetDatas[0].IsValid())
		{
			// Sync Assets in Content Browser
			ECB_LOG(Display, TEXT("Syncing %d assets in content browser."), AssetDatas.Num());
			{
				FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().SyncBrowserToAssets(AssetDatas);
			}
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////
// FExtAssetDependencyWalker and support class
//

bool FExtAssetDependencyInfo::IsValid(bool bIgnoreSoftIssue)
{
// 	return InvalidDependentFiles.Num() == 0
// 		&& MissingDependentFiles.Num() == 0
// 		&& MissingPackageNames.Num() == 0;
	return AssetStatus == EDependencyNodeStatus::Valid || (bIgnoreSoftIssue ? AssetStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue : true);
}


void FExtAssetDependencyInfo::Print()
{
	ECB_LOG(Display, TEXT("--------------- ExtAssetDependencyInfo: totoal %d packages ----------"), AllDepdencyPackages.Num());
	int i = 0;
	for (const auto& Pair : AllDepdencyPackages)
	{
		FString Status = FExtAssetDependencyNode::GetStatusString(Pair.Value);
		ECB_LOG(Display, TEXT(" %d)%s : %s"), i, *Status, *Pair.Key.ToString());
	}

	ECB_LOG(Display, TEXT("--------------- ExtAssetDependencyInfo: totoal %d AssetDepdencies ----------"), AssetDepdencies.Num());
	for (const auto& Pair : AssetDepdencies)
	{
		ECB_LOG(Display, TEXT("- %s"), *Pair.Key.ToString());

		const TArray<FExtAssetDependencyNode>& Nodes = Pair.Value;
		i = 0;
		for (const FExtAssetDependencyNode& Node : Nodes)
		{
			FString NodeString = Node.ToString();
			ECB_LOG(Display, TEXT("     %d) %s"), i, *NodeString);
			++i;
		}
	}
}

FString FExtAssetDependencyNode::GetStatusString(EDependencyNodeStatus InStatus)
{
	FString Status;
	if (InStatus == EDependencyNodeStatus::Valid)
	{
		Status = TEXT("Valid");
	}
	else if (InStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue)
	{
		Status = TEXT("ValidWithSoftReferenceIssue");
	}
	else if (InStatus == EDependencyNodeStatus::Invalid)
	{
		Status = TEXT("Invalid");
	}
	else if (InStatus == EDependencyNodeStatus::Missing)
	{
		Status = TEXT("Missing");
	}
	else if (InStatus == EDependencyNodeStatus::AbortGathering)
	{
		Status = TEXT("Aborted");
	}
	else
	{
		Status = TEXT("Unknown");
	}
	return Status;
}

FString FExtAssetDependencyNode::ToString() const
{
	FString ReferenceTypeString = ReferenceType == EDependencyNodeReferenceType::Soft ? TEXT("Soft") : TEXT("Hard");
	FString StatusString = GetStatusString(NodeStatus);
	return FString::Printf(TEXT("%s-%s: %s"), *ReferenceTypeString, *StatusString, *PackageName.ToString());
}

EDependencyNodeStatus FExtAssetDependencyWalker::GatherDependencies(const FExtAssetData& InAssetData, const FString& InAssetContentDir, const FExtAssetData::EContentType& InAssetContentType, FExtAssetDependencyInfo& InOuDependencyInfo, int32 Level, FScopedSlowTask* SlowTaskPtr)
{
	FString Padding; 
#if ECB_DEBUG
	Padding = Padding.LeftPad((Level + 1) * 6) + TEXT("|");
#endif

	EDependencyNodeStatus CurrentAssetStatus = EDependencyNodeStatus::Valid;

	FString AssetFilePath = InAssetData.PackageFilePath.ToString();
	ECB_LOG(Display, TEXT("%sGather dependencies for %s"), *Padding, *AssetFilePath);

	const bool bFileExist = IFileManager::Get().FileExists(*AssetFilePath);
	if (bFileExist)
	{
		if (InAssetData.IsValid())
		{
			InOuDependencyInfo.ValidDependentFiles.AddUnique(AssetFilePath);
		}
		else
		{
			InOuDependencyInfo.InvalidDependentFiles.AddUnique(AssetFilePath);
			ECB_LOG(Error, TEXT("%s  => %s Found but invalid: %s"), *Padding, *AssetFilePath, *InAssetData.GetInvalidReason());
			return EDependencyNodeStatus::Invalid;
		}
	}
	else
	{
		InOuDependencyInfo.MissingDependentFiles.AddUnique(AssetFilePath);
		ECB_LOG(Display, TEXT("%s  Missing!"), *Padding);
		return EDependencyNodeStatus::Missing;
	}

	const FText ProgressFormat = LOCTEXT("ValidatingPackages", "Validating {0}...{1}");

	ECB_LOG(Display, TEXT("%s    Total Depends: (%d)"), *Padding, InAssetData.HardDependentPackages.Num() + InAssetData.SoftReferencesList.Num());
	ECB_LOG(Display, TEXT("%s    Hard Depends: (%d)"), *Padding, InAssetData.HardDependentPackages.Num());
	for (const FName& DependPackageName : InAssetData.HardDependentPackages)
	{
		EDependencyNodeStatus PackageStatus = GatherPackageDependencies(DependPackageName, InOuDependencyInfo, InAssetContentDir, InAssetContentType, Level, /*bSoftReference*/ false, SlowTaskPtr);
		if (PackageStatus == EDependencyNodeStatus::AbortGathering)
		{
			return EDependencyNodeStatus::AbortGathering;
		}

		TArray<FExtAssetDependencyNode>& DepdencyNodes = InOuDependencyInfo.AssetDepdencies.FindOrAdd(InAssetData.PackageFilePath);
		DepdencyNodes.Emplace(DependPackageName, PackageStatus, /*bSoftReference*/ false);

		if (SlowTaskPtr)
		{
			if (SlowTaskPtr->ShouldCancel())
			{
				return EDependencyNodeStatus::AbortGathering;
			}
			const float ProgessAmount = (Level == 0) ? 1.f : 0.f;
			FName ShortPackageName = FPackageName::GetShortFName(DependPackageName);
			const FText ProgessText = FText::Format(ProgressFormat, FText::FromName(ShortPackageName), FText::FromString(FExtAssetDependencyNode::GetStatusString(PackageStatus)));
			SlowTaskPtr->EnterProgressFrame(ProgessAmount, ProgessText);
		}

		if ((PackageStatus == EDependencyNodeStatus::Invalid || PackageStatus == EDependencyNodeStatus::Missing)
			&& CurrentAssetStatus != EDependencyNodeStatus::Invalid)
		{
			CurrentAssetStatus = EDependencyNodeStatus::Invalid;
		}
		else if (PackageStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue
			&& CurrentAssetStatus == EDependencyNodeStatus::Valid)
		{
			CurrentAssetStatus = EDependencyNodeStatus::ValidWithSoftReferenceIssue;
		}
	}

	ECB_LOG(Display, TEXT("%s    Soft Depends: (%d)"), *Padding, InAssetData.SoftReferencesList.Num());
	for (const FName& DependPackageName : InAssetData.SoftReferencesList)
	{
		EDependencyNodeStatus PackageStatus = GatherPackageDependencies(DependPackageName, InOuDependencyInfo, InAssetContentDir, InAssetContentType, Level, /*bSoftReference*/ true);

		TArray<FExtAssetDependencyNode>& DepdencyNodes = InOuDependencyInfo.AssetDepdencies.FindOrAdd(InAssetData.PackageFilePath);
		if (PackageStatus == EDependencyNodeStatus::AbortGathering)
		{
			return EDependencyNodeStatus::AbortGathering;
		}

		DepdencyNodes.Emplace(DependPackageName, PackageStatus, /*bSoftReference*/ true);

		if (SlowTaskPtr)
		{
			if (SlowTaskPtr->ShouldCancel())
			{
				return EDependencyNodeStatus::AbortGathering;
			}
			const float ProgessAmount = (Level == 0) ? 1.f : 0.f;
			FName ShortPackageName = FPackageName::GetShortFName(DependPackageName);
			const FText ProgessText = FText::Format(ProgressFormat, FText::FromName(ShortPackageName), FText::FromString(FExtAssetDependencyNode::GetStatusString(PackageStatus)));
			SlowTaskPtr->EnterProgressFrame(ProgessAmount, ProgessText);
		}

		if ((PackageStatus == EDependencyNodeStatus::Invalid || PackageStatus == EDependencyNodeStatus::Missing || PackageStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue)
			&& CurrentAssetStatus == EDependencyNodeStatus::Valid)
		{
			CurrentAssetStatus = EDependencyNodeStatus::ValidWithSoftReferenceIssue;
		}
	}

	ECB_LOG(Display, TEXT("%s    => Asset Status: %s"), *Padding, *FExtAssetDependencyNode::GetStatusString(CurrentAssetStatus));

	return CurrentAssetStatus;
}

EDependencyNodeStatus FExtAssetDependencyWalker::GatherDependencies(const FExtAssetData& InAssetData, const FString& InAssetContentDir, const FExtAssetData::EContentType& InAssetContentType, FExtAssetDependencyInfo& InOuDependencyInfo, bool bShowProgess)
{
	if (InAssetData.IsValid())
	{
		int32 NumDirectDependencies = InAssetData.HardDependentPackages.Num() + InAssetData.SoftReferencesList.Num();
		FScopedSlowTask SlowTask(NumDirectDependencies + 1, LOCTEXT("GatheringDependencies", "Gathering dependencies..."));

		int32 Level = 0;
		FScopedSlowTask* SlowTaskPtr = bShowProgess ? &SlowTask : nullptr;
		if (bShowProgess)
		{
			SlowTask.MakeDialog(/*ShowCancel*/true);
		}
		return GatherDependencies(InAssetData, InAssetContentDir, InAssetContentType, InOuDependencyInfo, Level, SlowTaskPtr);
	}
	return EDependencyNodeStatus::Invalid;
}

EDependencyNodeStatus FExtAssetDependencyWalker::GatherPackageDependencies(const FName& DependPackageName, FExtAssetDependencyInfo& InOuDependencyInfo, const FString& InAssetContentDir, const FExtAssetData::EContentType& InAssetContentType, int32& Level, bool bSoftReference, FScopedSlowTask* SlowTaskPtr)
{
	if (SlowTaskPtr && SlowTaskPtr->ShouldCancel())
	{
		return EDependencyNodeStatus::AbortGathering;
	}

	FString Padding;
#if ECB_DEBUG
	Padding = Padding.LeftPad((Level + 1) * 6) + (bSoftReference ? TEXT("|") : TEXT("||"));
#endif
	FString DependPackageString = DependPackageName.ToString();
	if (InOuDependencyInfo.AllDepdencyPackages.Contains(DependPackageName))
	{
		ECB_LOG(Display, TEXT("%s    - %s (processed)"), *Padding, *DependPackageString);
		return InOuDependencyInfo.AllDepdencyPackages[DependPackageName];
	}
	InOuDependencyInfo.AllDepdencyPackages.Add(DependPackageName, EDependencyNodeStatus::Valid);

	EDependencyNodeStatus PackageStatus = EDependencyNodeStatus::Valid;

	const FString UAssetExtension = FExtAssetSupport::AssetPackageExtension; //	const FString UMapExtension = FExtAssetSupport::MapPackageExtension;// (TEXT(".uasset"));

	FString GameRoot(TEXT("/Game/"));
#if ECB_FEA_IMPORT_PLUGIN
	if (InAssetContentType == FExtAssetData::EContentType::Plugin && !InAssetContentDir.IsEmpty())
	{
		GameRoot = FExtAssetDataUtil::GetPluginNameFromAssetContentRoot(InAssetContentDir);
	}
#endif

	if (DependPackageString.StartsWith(GameRoot))
	{
		ECB_LOG(Display, TEXT("%s    - %s (Gathering Dependencies)"), *Padding, *DependPackageString);

		FString DependentFilePath;
		if (!FExtAssetDataUtil::RemapGamePackageToFullFilePath(GameRoot, DependPackageString, InAssetContentDir, DependentFilePath))
		{
			InOuDependencyInfo.MissingDependentFiles.Add(DependentFilePath);
			ECB_LOG(Error, TEXT("  %s          => %s Missing!"), *Padding, *DependPackageString);
			PackageStatus = EDependencyNodeStatus::Missing;
		}
		else
		{
			if (FExtAssetData* DependentAssetPtr = FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheAssetByFilePath(FName(*DependentFilePath)))
			{
				FExtAssetData& DependentAsset = *DependentAssetPtr;
				PackageStatus = GatherDependencies(DependentAsset, InAssetContentDir, InAssetContentType, InOuDependencyInfo, Level + 1, SlowTaskPtr);
			}
			else
			{
				InOuDependencyInfo.InvalidDependentFiles.AddUnique(DependentFilePath);
				ECB_LOG(Error, TEXT("  %s          => %s Found but invalid!"), *Padding, *DependPackageString);
				PackageStatus = EDependencyNodeStatus::Invalid;
			}
		}
	}
	else
	{
		const bool bPackageExist = FExtPackageUtils::DoesPackageExist(DependPackageString);
		if (!bPackageExist)
		{
			InOuDependencyInfo.MissingPackageNames.Add(DependPackageString);
			PackageStatus = EDependencyNodeStatus::Missing;
		}

		ECB_LOG(Display, TEXT("%s    - FindPackage %s : %d"), *Padding, *DependPackageString, bPackageExist);
	}

	InOuDependencyInfo.AllDepdencyPackages[DependPackageName] = PackageStatus;
	return PackageStatus;
}

//--------------------------------------------------
// FExtAssetValidator and support classes Impl
//
bool FExtAssetValidator::ValidateDependency(const TArray<FExtAssetData>& InAssetDatas, FString* OutValidateResult, bool bShowProgess, EDependencyNodeStatus* OutAssetStatus)
{
	int32 NumValid = 0;
	int32 NumInValid = 0;

	EDependencyNodeStatus Result = EDependencyNodeStatus::Unknown;

	for (const FExtAssetData& InAssetData : InAssetDatas)
	{
		bool bValid = false;

		if (!InAssetData.IsValid() || !InAssetData.HasContentRoot())
		{
#if 0
			if (OutValidateResult)
			{
				if (!InAssetData.IsValid())
				{
					*OutValidateResult = FString::Printf(TEXT("Not a valid asset file: %s"), *InAssetData.GetInvalidReason());
				}
				else
				{
					*OutValidateResult = TEXT("The uasset file should be inside project content folder or vault cache data folder to be validate.");
				}
			}
#endif
			if (!InAssetData.IsUAsset())
			{
				const bool bFileExist = IFileManager::Get().FileExists(*InAssetData.PackageFilePath.ToString());
				bValid = bFileExist;
				Result = bValid ? EDependencyNodeStatus::Valid : EDependencyNodeStatus::Missing;
			}
			else
			{
				bValid = InAssetData.IsValid();
				Result = bValid ? EDependencyNodeStatus::Valid : EDependencyNodeStatus::Invalid;
			}
		}
		else
		{
			FExtAssetDependencyInfo DependencyInfo = FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheAssetDependencyInfo(InAssetData, bShowProgess);

			ECB_LOG(Display, TEXT("[ValidateDependency]%s    => Asset Status: %s")
				, *InAssetData.PackageFilePath.ToString()
				, *FExtAssetDependencyNode::GetStatusString(DependencyInfo.AssetStatus));

			Result = DependencyInfo.AssetStatus;

			if (DependencyInfo.AssetStatus == EDependencyNodeStatus::AbortGathering)
			{
				if (OutValidateResult)
				{
					FString ErrorMessage = TEXT("Validation Aborted!");
					*OutValidateResult = ErrorMessage;
				}
				bValid = false;
				break;
			}
			else if (DependencyInfo.IsValid())
			{
				bValid = true;
			}
			else
			{
				bValid = false;
			}

			{
				if (InAssetDatas.Num() == 1)
				{
					FString AssetContentDir = InAssetData.AssetContentRoot.ToString();
					FString ErrorMessage = bValid 
						?  ((DependencyInfo.AssetStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue) 
							? TEXT("Valid with soft refrences issues:\n\n")
							: TEXT("Valid!\n\n"))
						: TEXT("Invalid:\n\n");

					if (DependencyInfo.MissingDependentFiles.Num() > 0)
					{
						FString WordEnd = DependencyInfo.MissingDependentFiles.Num() == 1 ? TEXT("") : TEXT("s");
						FString Msg = FString::Printf(TEXT("Missing dependency file%s:\n"), *WordEnd);
						FString AssetContentDirWithSlash = AssetContentDir + TEXT("/");
						for (const FString& MissingFile : DependencyInfo.MissingDependentFiles)
						{
							FString FriendlyName = MissingFile;
							FriendlyName.RemoveFromStart(AssetContentDirWithSlash);
							Msg += FString::Printf(TEXT("  - %s\n"), *FriendlyName);
						}
						ErrorMessage += Msg;
					}
					if (DependencyInfo.InvalidDependentFiles.Num() > 0)
					{
						FString WordEnd = DependencyInfo.InvalidDependentFiles.Num() == 1 ? TEXT("") : TEXT("s");
						FString Msg = FString::Printf(TEXT("\nInvalid dependency file%s found:\n"), *WordEnd);
						for (const FString& InvalidFile : DependencyInfo.InvalidDependentFiles)
						{
							Msg += FString::Printf(TEXT("  - %s\n"), *InvalidFile);
						}
						ErrorMessage += Msg;
					}
					if (DependencyInfo.MissingPackageNames.Num() > 0)
					{
						FString WordEnd = DependencyInfo.MissingPackageNames.Num() == 1 ? TEXT("") : TEXT("s");
						FString Msg = FString::Printf(TEXT("\nMissing dependency package%s:\n"), *WordEnd);
						for (const FString& MissingPackage : DependencyInfo.MissingPackageNames)
						{
							Msg += FString::Printf(TEXT("  - %s\n"), *MissingPackage);
						}
						ErrorMessage += Msg;
					}

					if (OutValidateResult)
					{
						*OutValidateResult = ErrorMessage;
					}
					return bValid;
				}
			}

#if ECB_DEBUG
				DependencyInfo.Print();
#endif
		}
		
		bValid ? ++NumValid : ++NumInValid;
	}

	const bool bAllValid = (NumValid == InAssetDatas.Num());
	if (OutValidateResult)
	{
		if (bAllValid)
		{
			*OutValidateResult = TEXT("All dependencies are valid.");
		}
		else
		{
			*OutValidateResult = FString::Printf(TEXT("%d valid. %d invalid."), NumValid, NumInValid);
		}
	}

	if (OutAssetStatus)
	{
		*OutAssetStatus = Result;
	}

	return bAllValid;
}


bool FExtAssetValidator::ValidateDependency(const TArray<FExtAssetData*>& InAssetDatas, FString* OutValidateResultPtr /*= nullptr*/, bool bShowProgess /*= false*/, EDependencyNodeStatus* OutAssetStatus /*= nullptr*/)
{
	TArray<FExtAssetData> AssetDatas;
	for (const FExtAssetData* AssetDataPtr : InAssetDatas)
	{
		AssetDatas.Emplace(*AssetDataPtr);
	}
	return ValidateDependency(AssetDatas, OutValidateResultPtr, bShowProgess, OutAssetStatus);
}

void FExtAssetValidator::InValidateDependency(const TArray<FExtAssetData>& InAssetDatas)
{
	for (const FExtAssetData& InAssetData : InAssetDatas)
	{
		FExtContentBrowserSingleton::GetAssetRegistry().RemoveCachedAssetDependencyInfo(InAssetData);
	}
}

//--------------------------------------------------
// FExtCollectionUtil Impl
//
FName FExtCollectionUtil::GetOrCreateCollection(FName InCollectionName, bool bUniqueCollection)
{
	const ECollectionShareType::Type ShareType = ECollectionShareType::CST_Local;

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	ICollectionManager& CollectionManager = CollectionManagerModule.Get();

	FName CollectionName = InCollectionName == NAME_None ? FExtAssetContants::DefaultImportedUAssetCollectionName : InCollectionName;

	ECB_LOG(Display, TEXT("[GetOrCreateCollection] %s %d"), *InCollectionName.ToString(), bUniqueCollection);

	if (bUniqueCollection)
	{
		ECB_LOG(Display, TEXT("[GetOrCreateCollection] %s exist? %d"), *CollectionName.ToString(), CollectionManager.CollectionExists(CollectionName, ShareType));

		if (!CollectionManager.CollectionExists(CollectionName, ShareType))
		{
			if (CollectionManager.CreateCollection(CollectionName, ShareType, ECollectionStorageMode::Static))
			{
				return CollectionName;
			}
		}
		
		FName UniqueCollectionName;
		CollectionManager.CreateUniqueCollectionName(CollectionName, ShareType, UniqueCollectionName);
		if (CollectionManager.CreateCollection(UniqueCollectionName, ShareType, ECollectionStorageMode::Static))
		{
			ECB_LOG(Display, TEXT("[GetOrCreateCollection] %s exist? %d"), *UniqueCollectionName.ToString(), CollectionManager.CollectionExists(UniqueCollectionName, ShareType));
			return UniqueCollectionName;
		}

		return UniqueCollectionName;
	}
	else
	{
		if (CollectionManager.CollectionExists(CollectionName, ShareType))
		{
			return CollectionName;
		}

		ECB_LOG(Display, TEXT("[GetOrCreateCollection] CreateCollection %s exist? %d"), *CollectionName.ToString(), CollectionManager.CollectionExists(CollectionName, ShareType));

		if (CollectionManager.CreateCollection(CollectionName, ShareType, ECollectionStorageMode::Static))
		{
			return CollectionName;
		}
	}

	return NAME_None;
}

bool FExtCollectionUtil::AddAssetsToCollection(const TArray<FAssetData>& InAssets, FName InCollectionName, bool bUniqueCollection)
{
	FName CollectionName = GetOrCreateCollection(InCollectionName, bUniqueCollection);
	if (CollectionName == NAME_None)
	{
		return false;
	}

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	ICollectionManager& CollectionManager = CollectionManagerModule.Get();

	TSet<FSoftObjectPath> CurrentAssetPaths;
	for (const FAssetData& AssetData : InAssets)
	{
		CurrentAssetPaths.Add(AssetData.GetSoftObjectPath());
	}
	const TArray<FSoftObjectPath> ObjectPaths = CurrentAssetPaths.Array();

	int32 NumAdded = 0;
	ECollectionShareType::Type ShareType = ECollectionShareType::CST_Local;
	if (CollectionManager.AddToCollection(CollectionName, ShareType, ObjectPaths, &NumAdded))
	{
		//if (NumAdded > 0)
		{
			return CollectionManager.SaveCollection(CollectionName, ShareType);
		}
	}
	return false;
}

//--------------------------------------------------
// FExtAssetImporter and support classes Impl
//
struct FExtAssetImportTargetFolderInfo
{
	FString TargetFolderPacakgeName;
	FLinearColor TargetFolderColor;

	bool bTargetFolderColorExist = false;
	FLinearColor ExistingTargetFolderColor;

	FExtAssetImportTargetFolderInfo(const FString& InTargetFolderPacakgeName, const FLinearColor& InTargetFolderColor)
		: TargetFolderPacakgeName(InTargetFolderPacakgeName)
		, TargetFolderColor(InTargetFolderColor)
	{
		TSharedPtr<FLinearColor> ExistingColor = ExtContentBrowserUtils::LoadColor(TargetFolderPacakgeName, /*bNoCache*/ true);
		if (ExistingColor.IsValid())
		{
			bTargetFolderColorExist = true;
			ExistingTargetFolderColor = *ExistingColor.Get();
		}
	}
};

struct FExtAssetImportTargetFileInfo
{
	FString SourceFile;
	FString TargetFile;
	bool bTargeFileWasExist = false;
	
	FName AssetContentDir = NAME_None;
	FName SourceAssetName = NAME_None;
	bool bSourceTargetHasSameClass = false;
	bool bMainAsset = false;

	bool bSkipped = false;

	bool bRedirectMe = false; // Request to redirect
	FString RedirectFromPackageName;
	FString RedirectToPackageName;

	// plugin?
	bool bPluginAsset = false;
	FString PluginRootWithSlash = TEXT("");

	FExtAssetImportTargetFileInfo(const FString& InSourceFile, const FString& InTargetFile, bool bInTargetExist)
		: SourceFile(InSourceFile)
		, TargetFile(InTargetFile)
		, bTargeFileWasExist(bInTargetExist)
	{
		if (FExtAssetData* SourceAssetData = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetByFilePath(*InSourceFile))
		{
			AssetContentDir = SourceAssetData->AssetContentRoot;
			SourceAssetName = SourceAssetData->AssetName;

			if (bTargeFileWasExist)
			{
				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
				IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
				TArray<FAssetData> TargetAssetDatas;
				AssetRegistry.GetAssetsByPackageName(*FPackageName::FilenameToLongPackageName(InTargetFile), TargetAssetDatas, /*bIncludeOnlyOnDiskAssets*/false);
				if (TargetAssetDatas.Num() > 0 && TargetAssetDatas[0].IsValid())
				{
					FName& SourceAssetClass = SourceAssetData->AssetClass;
					FName TargetAssetClass = TargetAssetDatas[0].AssetClassPath.GetAssetName();
					bSourceTargetHasSameClass = SourceAssetClass == TargetAssetClass;
					ECB_LOG(Display, TEXT("%s Source Class: %s, Target Class: %s"), *FPaths::GetCleanFilename(InTargetFile), *SourceAssetClass.ToString(), *TargetAssetClass.ToString());
				}
				else
				{
					bSourceTargetHasSameClass = true;
					ECB_LOG(Display, TEXT("TargetAsset %s is empty, set bSourceTargetHasSameClass to true for alwasy overwrite"), *FPaths::GetCleanFilename(InTargetFile));
				}
			}
		}
	}
};

struct FExtAssetImportInfo
{
	// Dependencies
	FExtAssetDependencyInfo DependencyInfo;

	TArray<FExtAssetImportTargetFileInfo> TargetFiles;

	TMap<FString, FExtAssetImportTargetFolderInfo> TargetColoredFolders;

	/**
	 * @bFlatMode : convert all target files path in one folder
	 */
	void GatherTargetFileInfos(const FExtAssetData& AssetData, const FString& AssetContentDir, const FString& TargetContentRoot, const FUAssetImportSetting& InImportSetting)
	{
		ECB_LOG(Display, TEXT("[GatherTargetFileInfos] for %s"), *AssetData.PackageFilePath.ToString());

		const bool bFlatMode = InImportSetting.bFlatMode;
		const bool bSandboxMode = InImportSetting.bSandboxMode;
		const bool bExportMode = InImportSetting.bExportMode;
		const bool bDirectCopyMode = InImportSetting.bDirectCopyMode;

		const bool bImportFolderColor = !bFlatMode && !bSandboxMode && !bExportMode && InImportSetting.bImportFolderColor && FExtContentBrowserSingleton::GetAssetRegistry().IsAsetContentRootHasFolderColor(AssetContentDir);
		const bool bOverrideFolderColor = InImportSetting.bOverrideExistingFolderColor;
		TArray<FName> ColoredFolders;
		if (bImportFolderColor)
		{
			FExtContentBrowserSingleton::GetAssetRegistry().GetAssetContentRootColoredFolders(AssetContentDir, ColoredFolders);
		}

		DependencyInfo = FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheAssetDependencyInfo(AssetData);

		const FString GameRootWithSlash(TEXT("/Game/"));
		const FString SandboxRootWithSlash = FExtAssetData::GetSandboxPackagePathWithSlash();

		FString DefaultProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		FPaths::NormalizeDirectoryName(DefaultProjectContentDir);

		TMap<FString, int32> UniqueFlatFileNames;

		const TArray<FString> SourceFileOnly = { AssetData.PackageFilePath.ToString() };
		const TArray<FString>& AllDependentFiles = bDirectCopyMode ? SourceFileOnly : DependencyInfo.ValidDependentFiles;

		for (const FString& ValidDependentFile : AllDependentFiles)
		{
			if (FExtAssetData* DependentAssetPtr = FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheAssetByFilePath(FName(*ValidDependentFile)))
			{
				FExtAssetData& DependentAsset = *DependentAssetPtr;


				FString OrphanAssetContentRoot = AssetContentDir;
				FString OrphaAssetRelativePath;
				bool bHasContentRootForDirectCopyOrphanAsset = false;
#if ECB_WIP_IMPORT_ORPHAN
				if (!DependentAsset.HasContentRoot() && DependentAsset.AssetContentType == FExtAssetData::EContentType::Orphan)
				{
					const bool bHasCachedCotentRoot = FExtContentBrowserSingleton::GetAssetRegistry().GetAssetContentRoot(DependentAsset.PackageFilePath.ToString(), OrphanAssetContentRoot);
					bHasContentRootForDirectCopyOrphanAsset = InImportSetting.bDirectCopyMode && DependentAsset.AssetContentType == FExtAssetData::EContentType::Orphan;

					if (bHasCachedCotentRoot || bHasContentRootForDirectCopyOrphanAsset)
					{
						OrphaAssetRelativePath = DependentAsset.PackageFilePath.ToString().RightChop(OrphanAssetContentRoot.Len());
						FPaths::NormalizeFilename(OrphaAssetRelativePath);
						if (OrphaAssetRelativePath.StartsWith(TEXT("/")))
						{
							OrphaAssetRelativePath = OrphaAssetRelativePath.Mid(1);
						}

						// Update Asset Content Root for dependency assets use cached info
						if (bHasCachedCotentRoot)
						{
							DependentAsset.AssetContentRoot = *OrphanAssetContentRoot;
							DependentAsset.AssetRelativePath = *OrphaAssetRelativePath;
						}
					}
				}
#endif

#if ECB_WIP_IMPORT_ORPHAN
				bool bHasCotentRoot = DependentAsset.HasContentRoot() || bHasContentRootForDirectCopyOrphanAsset;
#else
				bool bHasCotentRoot = DependentAsset.HasContentRoot();
#endif

				if (bHasCotentRoot)
				{
					FString AssetContentRoot = bHasContentRootForDirectCopyOrphanAsset ? OrphanAssetContentRoot : DependentAsset.AssetContentRoot.ToString();
					FString RelativePath = bHasContentRootForDirectCopyOrphanAsset ? OrphaAssetRelativePath :  DependentAsset.AssetRelativePath.ToString();
					FString FileName = FPaths::GetCleanFilename(RelativePath);
					FString TargetFilePath = FPaths::Combine(TargetContentRoot, RelativePath);
					FPaths::NormalizeFilename(TargetFilePath);

					const bool bTargetFileExist = IFileManager::Get().FileExists(*TargetFilePath);
					ECB_LOG(Display, TEXT("        %s  Found and valid! =>(exist? %d)"), *TargetFilePath, bTargetFileExist);

					const FString& SourceFilePath = ValidDependentFile;
					int32 NewIndex = TargetFiles.Emplace(SourceFilePath, TargetFilePath, bTargetFileExist);

					auto& AssetContentType = DependentAsset.AssetContentType;
					const bool bPluginAsset = AssetContentType == FExtAssetData::EContentType::Plugin && !AssetContentDir.IsEmpty();
					FString PluginRootWithSlash = bPluginAsset 
						? FExtAssetDataUtil::GetPluginNameFromAssetContentRoot(AssetContentRoot) // /PluginName/
						: TEXT("");

					if (bPluginAsset)
					{
						FExtAssetImportTargetFileInfo& TargetFile = TargetFiles[NewIndex];
						TargetFile.bPluginAsset = true;
						TargetFile.PluginRootWithSlash = PluginRootWithSlash;
					}

					if (bExportMode)
					{
						continue; // Skip redirect data
					}

					// Gather redirect info
					// if (bPluginAsset || bFlatMode || bSandboxMode)
					{
						FString SourcePackgeName = FPackageName::FilenameToLongPackageName(FPaths::Combine(DefaultProjectContentDir, RelativePath));
						FString TargetPackageName = FPackageName::FilenameToLongPackageName(TargetFilePath);

						// default redirect
						bool bRedirectMe = false;
						FString RedirectFromPackageName = SourcePackgeName;
						FString RedirectToPackageName = TargetPackageName;

						if (bPluginAsset)
						{
							bRedirectMe = true;
							FString ImportingRoot = bSandboxMode ? SandboxRootWithSlash : GameRootWithSlash;
							RedirectFromPackageName = PluginRootWithSlash + SourcePackgeName.Mid(ImportingRoot.Len());
						}

						if (bFlatMode)
						{
							if (UniqueFlatFileNames.Contains(FileName))
							{
								int32 LastIndex = UniqueFlatFileNames[FileName];
								UniqueFlatFileNames[FileName] = LastIndex + 1; 
								FileName = FPaths::GetBaseFilename(FileName) + FString::FromInt(LastIndex) + FPaths::GetExtension(FileName, /*bIncludeDot*/ true);
							}
							else
							{
								UniqueFlatFileNames.Add(FileName, 1);
							}

							TargetFilePath = FPaths::Combine(TargetContentRoot, FileName);
							FPaths::NormalizeFilename(TargetFilePath);

							bRedirectMe = true;
							RedirectToPackageName = FPackageName::FilenameToLongPackageName(TargetFilePath);
						}

						if (!bPluginAsset && bSandboxMode && TargetPackageName.StartsWith(SandboxRootWithSlash))
						{
							FString OrigPackageName = GameRootWithSlash/*TEXT("/Game/")*/ + TargetPackageName.Mid(SandboxRootWithSlash.Len());

							bRedirectMe = true;
							RedirectFromPackageName = OrigPackageName;
						}

						if (!bRedirectMe)
						{
							FString SourceRootPackage = FExtAssetDataUtil::GetPackageRootFromFullPackagePath(SourcePackgeName);
							FString TargetRootPackage = FExtAssetDataUtil::GetPackageRootFromFullPackagePath(TargetPackageName);
							if (!SourceRootPackage.Equals(TargetRootPackage))
							{
								bRedirectMe = true;
							}
						}

						if (bRedirectMe)
						{
							FExtAssetImportTargetFileInfo& TargetFile = TargetFiles[NewIndex];
							TargetFile.bRedirectMe = true;
							TargetFile.RedirectFromPackageName = RedirectFromPackageName;
							TargetFile.RedirectToPackageName = RedirectToPackageName;

							TargetFile.TargetFile = TargetFilePath;
						}

						// Gather Folder Colors
#if ECB_WIP_IMPORT_FOLDER_COLOR
						if (bImportFolderColor && ColoredFolders.Num() > 0)
						{
							FExtAssetImportTargetFileInfo& TargetFile = TargetFiles[NewIndex];

							for (int32 Index = ColoredFolders.Num() - 1; Index >= 0;  --Index)
							{
								auto& ColoredFolder = ColoredFolders[Index];
								FString ColoredFolderStr = ColoredFolder.ToString();
								if (SourceFilePath.StartsWith(ColoredFolderStr))
								{
									FString FolderRelativePath = ColoredFolderStr.Mid(FCString::Strlen(*AssetContentDir) + 1);

									FString TargetPackageRoot = bRedirectMe
										? FExtAssetDataUtil::GetPackageRootFromFullPackagePath(RedirectToPackageName)
										: FExtAssetDataUtil::GetPackageRootFromFullPackagePath(TargetPackageName);
									FString TargetFolderPackagePath = TargetPackageRoot + FolderRelativePath;
									
									FLinearColor FolderColor;
									if (!FExtContentBrowserSingleton::GetAssetRegistry().GetFolderColor(ColoredFolderStr, FolderColor))
									{
										continue;
									}

									if (!TargetColoredFolders.Contains(TargetFolderPackagePath))
									{
										FExtAssetImportTargetFolderInfo TargetFolderInfo(TargetFolderPackagePath, FolderColor);
										if (!TargetFolderInfo.bTargetFolderColorExist || (TargetFolderInfo.bTargetFolderColorExist && bOverrideFolderColor))
										{
											TargetColoredFolders.Add(TargetFolderPackagePath, TargetFolderInfo);
										}
									}
									
									ECB_LOG(Display, TEXT(" TargetFolderPackagePath: %s, Color: %s"), *TargetFolderPackagePath, *FolderColor.ToString());

									ColoredFolders.RemoveAt(Index);
								}
							}
						}
#endif
					}
				}
			}
		}
	}
};

struct FExtAssetRollbackInfo
{
	void MapSourceToTarget(const FName& InSource, const FName& InTarget)
	{
		SourceToTargetMap.Add(InSource, InTarget);
	}

	void Backup(const FName& InTarget, const FName& InBackup)
	{
		TargetToBackupMap.Add(InTarget, InBackup);
	}

	void MapNewFile(const FName& InSource, const FName& InNewFile, const FName& InNewCreatedDir)
	{
		SourceToNewFileMap.Add(InSource, InNewFile);
		CreatedDirs.Add(InNewCreatedDir);
	}

	void Backup(const FExtAssetImportTargetFolderInfo& InFolderInfoToBackup)
	{
		if (InFolderInfoToBackup.bTargetFolderColorExist)
		{
			OverrideColoredFolders.Add(InFolderInfoToBackup);
		}
		else
		{
			NewColoredFolders.Add(InFolderInfoToBackup);
		}
	}

	void RollbackAll()
	{
		ECB_LOG(Display, TEXT("[Rollback] %d backup, %d new."), TargetToBackupMap.Num(), SourceToNewFileMap.Num());

		// Delete new created files
		for (auto& FilePair : SourceToNewFileMap)
		{
			const FName& NewFile = FilePair.Value;
			if (IFileManager::Get().FileExists(*NewFile.ToString()))
			{
				const bool bRequireExists = false;
				const bool bEvenReadOnly = false;
				const bool bQuiet = false;
				bool bReuslt = IFileManager::Get().Delete(*NewFile.ToString(), bRequireExists, bEvenReadOnly, bQuiet);
				if (bReuslt)
				{
					ECB_LOG(Display, TEXT("     X Success to delete new file:\n\t\t%s."), *NewFile.ToString());
				}
				else
				{
					ECB_LOG(Error, TEXT("     X Failed to delete new file:\n\t\t%s."), *NewFile.ToString());
				}
			}
			else
			{
				ECB_LOG(Warning, TEXT("     X Can't find new file:\n\t\t%s."), *NewFile.ToString());
			}
		}

		// Move back backup files
		TArray<UPackage*> ReplacedPackages;
		for (auto It = TargetToBackupMap.CreateIterator(); It; ++It)
		{
			const FName& Target = It->Key;
			const FName& Backup = It->Value;
			if (IFileManager::Get().FileExists(*Backup.ToString()))
			{	
				FExtPackageUtils::UnloadPackage({ Target.ToString() }, &ReplacedPackages);

				const bool bReplace = true;
				const bool bEvenIfReadOnly = true;
				const bool bAttributes = true;
				const bool bDoNotRetryOrError = false;
				bool bResult = IFileManager::Get().Move(/*Dest=*/*Target.ToString(), /*Src=*/ *Backup.ToString(), bReplace, bEvenIfReadOnly, bAttributes, bDoNotRetryOrError);
				if (bResult)
				{
					ECB_LOG(Display, TEXT("     <- Success Move backuped file from \n\t\t%s \n\t\tto %s."), *Backup.ToString(), *Target.ToString());
				}
				else
				{
					ECB_LOG(Error, TEXT("     <- Failed to move backuped file from \n\t\t%s \n\t\tto %s."), *Backup.ToString(), *Target.ToString());
				}
			}
			else
			{
				ECB_LOG(Warning, TEXT("     X Can't find backup file:\n\t\t%s."), *Backup.ToString());
			}
		}

		// Delete new created dirs
		TArray<FString> NewCreateDirs;
		for (auto& Dir : CreatedDirs)
		{
			NewCreateDirs.Emplace(Dir.ToString());
		}
		TArray<FString> MergedCreateDirs;
		FPathsUtil::SortAndMergeDirs(NewCreateDirs, MergedCreateDirs);
		for (auto& CreatedDir : MergedCreateDirs)
		{
			if (IFileManager::Get().DirectoryExists(*CreatedDir))
			{
				bool bReuslt = IFileManager::Get().DeleteDirectory(*CreatedDir, /*RequireExists */ false, /*Tree */ true);
				if (bReuslt)
				{
					ECB_LOG(Display, TEXT("     X Success delete new dir:\n\t\t%s."), *CreatedDir);
				}
				else
				{
					ECB_LOG(Error, TEXT("     X Failed to delete new dir:\n\t\t%s."), *CreatedDir);
				}
			}
		}

		// Rollback folder colors
		{
			for (auto& FolderInfo : OverrideColoredFolders)
			{
				AssetViewUtils::SetPathColor(FolderInfo.TargetFolderPacakgeName, TOptional<FLinearColor>()); // clear PatchColors cache
				AssetViewUtils::SetPathColor(FolderInfo.TargetFolderPacakgeName, TOptional<FLinearColor>(FLinearColor(FolderInfo.ExistingTargetFolderColor)));
			}
			for (auto& FolderInfo : NewColoredFolders)
			{
				AssetViewUtils::SetPathColor(FolderInfo.TargetFolderPacakgeName, TOptional<FLinearColor>());
			}
		}

		UPackageTools::ReloadPackages(ReplacedPackages);
	}

#if ECB_TODO // todo: should support rollback individual file?
	void RollbackImportedFile(const FName& InTargetFile)
	{
		// Delete new file if found
		if (FName* NewFile = SourceToNewFileMap.Find(InTargetFile))
		{

		}
		// Or restore backup
		else
		{
			if (FName* Target = SourceToTargetMap.Find(InTargetFile))
			{
				if (FName* Backup = TargetToBackupMap.Find(*Target))
				{

				}
			}
		}
	}
#endif

	void DeleteBackupFiles()
	{
		// Delete all backup files
		for (auto& FilePair : TargetToBackupMap)
		{
			const FName& Backup = FilePair.Value;
			if (IFileManager::Get().FileExists(*Backup.ToString()))
			{
				const bool bRequireExists = false;
				const bool bEvenReadOnly = false;
				const bool bQuiet = false;
				bool bReuslt = IFileManager::Get().Delete(*Backup.ToString(), bRequireExists, bEvenReadOnly, bQuiet);
				ECB_LOG(Display, TEXT("     X Delete backup:\n\t\t%s.\n\t\t success? %d"), *Backup.ToString(), bReuslt);
			}
			else
			{
				ECB_LOG(Display, TEXT("     * Backup file already deleted:\n\t\t%s."), *Backup.ToString());
			}
		}

	}

	TMap<FName, FName> SourceToTargetMap;
	TMap<FName, FName> TargetToBackupMap;
	TMap<FName, FName> SourceToNewFileMap;
	TSet<FName> CreatedDirs;

	TArray<FExtAssetImportTargetFolderInfo> OverrideColoredFolders;
	TArray<FExtAssetImportTargetFolderInfo> NewColoredFolders;
};

struct FExtAssetImportTask
{
	bool IfFoundInvalidOrMissingDependency() const
	{
		return ImportInfo.DependencyInfo.InvalidDependentFiles.Num() > 0
			|| ImportInfo.DependencyInfo.MissingDependentFiles.Num() > 0
			|| ImportInfo.DependencyInfo.MissingPackageNames.Num() > 0;
	}

	bool IfAllTargetFilesWereExist() const
	{
		bool bAllExist = true;
		for (const FExtAssetImportTargetFileInfo& Info : ImportInfo.TargetFiles)
		{
			if (!Info.bTargeFileWasExist)
			{
				bAllExist = false;
				break;
			}
		}
		return bAllExist;
	}

	bool IsSuccessImported() const
	{
		return Status == EImportTaskStatus::Imported;
	}

	bool IsSkipped() const
	{
		return Status == EImportTaskStatus::Skipped;
	}

	bool IsNotStarted() const
	{
		return Status == EImportTaskStatus::NotStarted;
	}

	bool IsRollbacked() const
	{
		return Status == EImportTaskStatus::Rollbacked;
	}

	void Rollback()
	{
		RollbackInfo.RollbackAll();

		Status = EImportTaskStatus::Rollbacked;
	}

	bool IsDependsOn(const FString& InDependencyFilePath)
	{
		return ImportInfo.DependencyInfo.ValidDependentFiles.Find(InDependencyFilePath) != INDEX_NONE;
	}

public:

	// Source
	FExtAssetData SourceAssetData;
	FString GetMainSourceFilePath() { return SourceAssetData.PackageFilePath.ToString(); }

	// ImportInfo
	FExtAssetImportInfo ImportInfo;

	// Rollback
	FExtAssetRollbackInfo RollbackInfo;

	// Result
	FAssetData ImportedMainAssetData;

	enum class EImportTaskStatus : uint8
	{
		NotStarted,
		Skipped,
		Imported,
		Failed,
		Rollbacked,

	};
	EImportTaskStatus Status = EImportTaskStatus::NotStarted;
};

struct FExtAssetImportSession
{
	FExtAssetImportSession(const FUAssetImportSetting& InImportSetting)
	{
		ImportSetting = InImportSetting;
		FPaths::NormalizeDirectoryName(ImportSetting.TargetContentDir); // double make sure no ending slash
	
		const FString SessionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
		SesstionTempDir = FPaths::Combine(FExtAssetData::GetImportSessionTempDir(), SessionId);

		if (!IFileManager::Get().DirectoryExists(*SesstionTempDir))
		{
			if (!IFileManager::Get().MakeDirectory(*SesstionTempDir, /*tree*/ true))
			{
				ECB_LOG(Display, TEXT("[FExtAssetImportSession] Failed to create backup dir: %s"), *SesstionTempDir);
			}
		}
	}

	~FExtAssetImportSession()
	{
		if (IFileManager::Get().DirectoryExists(*SesstionTempDir))
		{
			const bool bRequireExists = false;
			const bool bTree = true;
			if (!IFileManager::Get().DeleteDirectory(*SesstionTempDir, bRequireExists, bTree))
			{
				ECB_LOG(Display, TEXT("[FExtAssetImportSession] Failed to delete backup dir: %s"), *SesstionTempDir);
			}
		}
	}

	void ImportAssets(const TArray<FExtAssetData>& InAssetDatas)
	{
		int32 NumMainAssetsToImport = InAssetDatas.Num();
		ECB_LOG(Display, TEXT("========== Importing %d Assets ============"), NumMainAssetsToImport);
		if (NumMainAssetsToImport <= 0)
		{
			return;
		}

		ECB_LOG(Display, TEXT("------------ Phase 1: Gathering dependencies -----------------"));
		{
			PrepareImportTasksWithDependencies(InAssetDatas);

			if (IsSessionRequestToCancel())
			{
				const FString FirstAssetFilePath = InAssetDatas[0].PackageFilePath.ToString();
				FString AbortMessage = FString::Printf(TEXT("Import %s%s aborted!")
					, *FPaths::GetBaseFilename(FirstAssetFilePath)
					, (NumMainAssetsToImport ? TEXT("") : *FString::Printf(TEXT(" and other %d assets"), NumMainAssetsToImport - 1)));

				ExtContentBrowserUtils::NotifyMessage(FText::FromString(AbortMessage));
				return;
			}

			const int32 NumAllAsssetsToImport = GetNumAllAssetsToImport();
			if (NumAllAsssetsToImport < 1)
			{
				FString AbortMessage = TEXT("No asset files been gathered to import, aborting!");
				ExtContentBrowserUtils::NotifyMessage(FText::FromString(AbortMessage));
				return;
			}
		}

		ECB_LOG(Display, TEXT("------------ Phase 2: Importing asset files -----------------"));
		{
			const int32 NumAllAsssetsToImport = GetNumAllAssetsToImport();
			const float TotalSteps = NumAllAsssetsToImport + 1;

			TSharedPtr<FExtAssetCoreUtil::FExtAssetFeedbackContext> FeedbackContext(new FExtAssetCoreUtil::FExtAssetFeedbackContext);
			FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("ImportingNAssets", "Importing {0} Assets..."), FText::AsNumber(NumAllAsssetsToImport))
				, /*bEnabled*/ true,
				*FeedbackContext.Get()
			);

			SlowTask.MakeDialog(/*bShowCancelDialog =*/ true);

			RunImportTasks(SlowTask);
		}

		TArray<FAssetData> SuccessImportedAssets;
		TArray<FAssetData> SuccessImportedMainAssets;

		ECB_LOG(Display, TEXT("------------ Phase 3: Validate and Sync/Redirect -----------------"));
		if (!IsSessionRequestToCancel())
		{
			int32 NumFilesToValidateOrSync = GetImportedFilePaths(/*bIncludeSkipped*/ImportSetting.bSyncExistingAssets).Num();

			TSharedPtr<FExtAssetCoreUtil::FExtAssetFeedbackContext> FeedbackContext(new FExtAssetCoreUtil::FExtAssetFeedbackContext);
			const float TotalSteps = RedirectInfo.NoRedirectImportedPackageNames.Num() + 1;
			FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("ValidatingNAssets", "Validating {0} Assets..."), FText::AsNumber(NumFilesToValidateOrSync))
				, /*bEnabled*/ true,
				*FeedbackContext.Get()
			);
			SlowTask.MakeDialog(/*bShowCancelDialog =*/ true);
			//SlowTask.EnterProgressFrame(1, LOCTEXT("ImportingAssets_Validate", "Validating imported assets..."));

			ValidateSyncRedirectImportResult(SlowTask, SuccessImportedAssets, SuccessImportedMainAssets);
		}

		ECB_LOG(Display, TEXT("------------ Phase 4: Rollback and cleanup -----------------"));
		{
			FScopedSlowTask SlowTask(1, LOCTEXT("ImportingAssets_PostImport", "Finish Importing..."));
			SlowTask.MakeDialog(/*bShowCancelDialog =*/ false);

			if (!IsSessionRequestToCancel())
			{
				SlowTask.EnterProgressFrame(.5f, LOCTEXT("ImportingAssets_Finishing", "Finishing import..."));
			}
			else
			{
				SlowTask.EnterProgressFrame(.5f, LOCTEXT("ImportingngAssets_Canceling", "Canceling import..."));
			}
			PostImport(SlowTask);
		}

		ECB_LOG(Display, TEXT("------------ Phase 5: Load assets -----------------"));
		if (const bool bShouldLoadAssets = ImportSetting.bLoadAssetAfterImport && !IsSessionRequestToCancel() && !IsSessionRollbacked())
		{
			const float TotalSteps = NumMainAssetsToImport + 1;
			FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("LoadingNAssets", "Loading {0} Assets..."), FText::AsNumber(NumMainAssetsToImport)));
			SlowTask.MakeDialog(/*bShowCancelDialog =*/ false);
			SlowTask.EnterProgressFrame(0, LOCTEXT("ImportingAssets_LoadAssets", "Loading assets..."));

			LoadAssets(SlowTask, SuccessImportedAssets);

			// Reload redirected packages ?
			const bool bReloadRedirectedPackages = false;
			if (bReloadRedirectedPackages)
			{
				// Don't allow asset reload during PIE
				if (GIsEditor)
				{
					if (SuccessImportedAssets.Num() > 0)
					{
						TArray<UPackage*> PackagesToReload;

						for (const FAssetData& AssetData : SuccessImportedAssets)
						{
							if (AssetData.AssetClassPath.GetPackageName() == UObjectRedirector::StaticClass()->GetFName())
							{
								// Don't operate on Redirectors
								continue;
							}

							if (AssetData.AssetClassPath.GetPackageName() == UUserDefinedStruct::StaticClass()->GetFName())
							{
								// User created structures cannot be safely reloaded.
								continue;
							}

							if (AssetData.AssetClassPath.GetPackageName() == UUserDefinedEnum::StaticClass()->GetFName())
							{
								// User created enumerations cannot be safely reloaded.
								continue;
							}

							UPackage* Package = AssetData.GetPackage();
							if (Package->IsFullyLoaded() && IsPackageRedirected(AssetData.PackageName.ToString()))
							{
								PackagesToReload.AddUnique(Package);
							}
						}

						if (PackagesToReload.Num() > 0)
						{
							UPackageTools::ReloadPackages(PackagesToReload);
						}
					}
				}
			}
		}

#if ECB_WIP_IMPORT_ADD_TO_COLLECTION

		ECB_LOG(Display, TEXT("------------ Phase 6: Add to Collection -----------------"));
		if (!IsSessionRequestToCancel() && !IsSessionRollbacked())
		{
			AddImportedAssetsToCollection(SuccessImportedAssets);
		}

#endif

#if ECB_FEA_IMPORT_ADD_PLACE
		ECB_LOG(Display, TEXT("------------ Phase 7: Place imported assets -----------------"));
		if (ImportSetting.bPlaceImportedAssets)
		{
			if (!IsSessionRequestToCancel() && !IsSessionRollbacked()
				&& SuccessImportedMainAssets.Num() > 0 && GCurrentLevelEditingViewportClient)
			{
				PlaceImportedMainAssetsToLevelEditor(SuccessImportedMainAssets);
			}
		}
#endif

#if ECB_WIP_IMPORT_FOR_DUMP
		ECB_LOG(Display, TEXT("------------ Phase 8: Dump imported assets -----------------"));
		if (ImportSetting.bDumpAsset)
		{
			if (!IsSessionRequestToCancel() && !IsSessionRollbacked()
				&& SuccessImportedMainAssets.Num() > 0)
			{
				DumpImportedMainAssetsViaExportDialog(SuccessImportedMainAssets);
			}

			ECB_LOG(Display, TEXT("------------ Phase 8.1: Cleanup -----------------"));
			if (!IsSessionRollbacked())
			{
				FScopedSlowTask SlowTask(1, LOCTEXT("ImportingAssets_Cleanup", "Cleanup..."));
				SlowTask.MakeDialog(/*bShowCancelDialog =*/ false);

				RollbackSession(SlowTask);
				Cleanup();
			}
		}
#endif
	}

	void PrepareImportTasksWithDependencies(const TArray<FExtAssetData>& InAssetDatas)
	{
		const int32 NumMainAssetsToImport = InAssetDatas.Num();
		ECB_LOG(Display, TEXT("[PrepareImportTasksWithDependencies] for %d assets."), NumMainAssetsToImport);
		if (NumMainAssetsToImport <= 0)
		{
			return;
		}
		const bool bNoDepeendencyGathering = ImportSetting.bDirectCopyMode;

		TSet<FName> DependencyGatheredFiles; // All files already analyzed to gather dependencies

		for (int32 Index = 0; Index < NumMainAssetsToImport; ++Index)
		{
			const FExtAssetData& AssetData = InAssetDatas[Index];
			FName PackageFilePath = AssetData.PackageFilePath;
			ECB_LOG(Display, TEXT("  %d) %s"), Index, *PackageFilePath.ToString());

			if (DependencyGatheredFiles.Contains(PackageFilePath))
			{
				ECB_LOG(Display, TEXT("    => Already analyzed. Skip analyzing %s."), *PackageFilePath.ToString());
				break;
			}
			DependencyGatheredFiles.Add(PackageFilePath);

			const bool bAssetFileExist = IFileManager::Get().FileExists(*PackageFilePath.ToString());
			if (!bAssetFileExist)
			{
				ECB_LOG(Display, TEXT("    => Asset file: %s not exist. Skip."), *PackageFilePath.ToString());
				break;
			}

			if (!bNoDepeendencyGathering)
			{
				FExtAssetDependencyInfo DependencyInfo = FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheAssetDependencyInfo(AssetData, /*bShowProgess*/ true);
				if (DependencyInfo.AssetStatus == EDependencyNodeStatus::AbortGathering)
				{
					bSessionRequestToCancel = true;
					ECB_LOG(Error, TEXT("[PrepareImportTasksWithDependencies] GetOrCacheAssetDependencyInfo aborted: "), *FPaths::GetBaseFilename(PackageFilePath.ToString()));
					break;
				}

				if (DependencyInfo.AssetStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue && ImportSetting.bIgnoreSoftReferencesError)
				{
					ImportSetting.bSkipImportIfAnyDependencyMissing = false;
				}
				else if (DependencyInfo.AssetStatus != EDependencyNodeStatus::Valid && ImportSetting.bSkipImportIfAnyDependencyMissing)
				{
					ECB_LOG(Display, TEXT("    => Asset file: %s not valid. Skip."), *PackageFilePath.ToString());
					break;
				}
			}

			FExtAssetImportTask NewTask;
			NewTask.SourceAssetData = AssetData;

			if (AssetData.HasContentRoot())
			{
				FString AssetFileRelativePath = AssetData.AssetRelativePath.ToString();
				FString AssetContentDir = AssetData.AssetContentRoot.ToString();
				ECB_LOG(Display, TEXT("    => %s relative to %s."), *AssetFileRelativePath, *AssetContentDir);

				if (FPaths::IsSamePath(AssetContentDir, ImportSetting.TargetContentDir))
				{
					ECB_LOG(Display, TEXT(" Asset Cotent Dir (%s) same as project content dir (%s), skip importing %s."), *AssetContentDir, *ImportSetting.TargetContentDir, *AssetFileRelativePath);
					FString SkipMessage = FString::Printf(TEXT("Can't import assets into same project: %s."), *AssetContentDir);
					SetTaskSkipped(NewTask, SkipMessage);
					continue;
				}

				FExtAssetImportInfo& ImportInfo = NewTask.ImportInfo;
				ImportInfo.GatherTargetFileInfos(AssetData, AssetContentDir, ImportSetting.TargetContentDir, ImportSetting);
			}
			else
			{
#if ECB_WIP_IMPORT_ORPHAN
				
				FString RootContentPath;
				if (/*ImportSetting.bDirectCopyMode && */AssetData.AssetContentType == FExtAssetData::EContentType::Orphan
					&& FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPathFromFilePath(AssetData.PackageFilePath.ToString(), RootContentPath)
					)
				{
					FExtAssetImportInfo& ImportInfo = NewTask.ImportInfo;
					ImportInfo.GatherTargetFileInfos(AssetData, RootContentPath, ImportSetting.TargetContentDir, ImportSetting);
				}
				else
#endif
				{
					ECB_LOG(Display, TEXT("    => Can't find relative path to asset content root, skip importing %s."), *FPaths::GetCleanFilename(PackageFilePath.ToString()));
					FString SkipMessage = FString::Printf(TEXT("Can't find relative path to asset content root for %s."), *FPaths::GetCleanFilename(PackageFilePath.ToString()));
					SetTaskSkipped(NewTask, SkipMessage);
				}
			}

			ImportTasks.Add(NewTask);
		}
	}

	void RunImportTasks(FScopedSlowTask& SlowTask)
	{
		if (SlowTask.ShouldCancel())
		{
			ECB_LOG(Warning, TEXT("[RunImportTasks] Canceled"));
			return;
		}

		int32 NumImportTasks = ImportTasks.Num();
		ECB_LOG(Display, TEXT("[RunImportTasks] %d tasks."), NumImportTasks);
		if (NumImportTasks <= 0)
		{
			return;
		}

		// All imported files including dependency file path to result map
		TMap<FName, bool> ImportedFilesResult;

#if ECB_WIP_IMPORT_FOLDER_COLOR
		// All imported colored folders
		TSet<FString> ImportedColoredFolders;
#endif

		bool bCancelRequestedByUser = false;

		for (auto& Task : ImportTasks)
		{
			if (!Task.IsNotStarted())
			{
				continue;
			}

			if (Task.IfFoundInvalidOrMissingDependency())
			{
				ECB_INFO(Warning, TEXT("Dependency missing or invaid when importing %s. (Invalid files: %d, Missing files: %d, Missing Packages: %d"), *Task.GetMainSourceFilePath()
					, Task.ImportInfo.DependencyInfo.InvalidDependentFiles.Num()
					, Task.ImportInfo.DependencyInfo.MissingDependentFiles.Num()
					, Task.ImportInfo.DependencyInfo.MissingPackageNames.Num());
				if (ImportSetting.bSkipImportIfAnyDependencyMissing)
				{
					ECB_LOG(Display, TEXT("   => Skip according to import setting."));

					const int32 NumInvalid = Task.ImportInfo.DependencyInfo.InvalidDependentFiles.Num();
					const int32 NumMissing = Task.ImportInfo.DependencyInfo.MissingDependentFiles.Num() + Task.ImportInfo.DependencyInfo.MissingPackageNames.Num();

					FString InvalidMessage = NumInvalid > 0 ? FString::Printf(TEXT("%d %s invalid. "), NumInvalid, (NumInvalid == 1 ? TEXT("dependency") : TEXT("dependencies"))) : TEXT("");
					FString MissingMessage = NumMissing > 0 ? FString::Printf(TEXT("%d %s missing."), NumMissing, (NumMissing == 1 ? TEXT("dependency") : TEXT("dependencies"))) : TEXT("");

					FString SkipMessage = FString::Printf(TEXT("%s%s"), *InvalidMessage, *MissingMessage);
					SetTaskSkipped(Task, SkipMessage);
					continue;
				}
			}

			if (Task.IfAllTargetFilesWereExist())
			{
				ECB_INFO(Warning, TEXT("All files including %d dependencies were exist when importing %s."), Task.ImportInfo.DependencyInfo.ValidDependentFiles.Num(), *Task.GetMainSourceFilePath());
				if (!ImportSetting.bOverwriteExistingFiles)
				{
					ECB_LOG(Display, TEXT("   => Skip according to import setting."));
					FString SkipMessage = FString::Printf(TEXT("All files including %d dependencies were exist."), Task.ImportInfo.DependencyInfo.ValidDependentFiles.Num());
					SetTaskSkipped(Task, SkipMessage);

					FString MainSourceFile = Task.GetMainSourceFilePath();
					for (FExtAssetImportTargetFileInfo TargetFileInfo : Task.ImportInfo.TargetFiles)
					{
						if (MainSourceFile.Equals(TargetFileInfo.SourceFile))
						{
							TargetFileInfo.bMainAsset = true;
						}
						TargetFileInfo.bSkipped = true;
						ImportedFilesInSession.Add(TargetFileInfo);
					}

					//continue;
				}
			}

			if (!Task.IsSkipped())
			{
				ECB_LOG(Display, TEXT("Importing %s with %d dependencies will be imported)"), *Task.GetMainSourceFilePath(), Task.ImportInfo.DependencyInfo.ValidDependentFiles.Num());

				bool bFailed = false;
				FString FailMessage;

				for (FExtAssetImportTargetFileInfo TargetFileInfo : Task.ImportInfo.TargetFiles)
				{
					const FString& Src = TargetFileInfo.SourceFile;
					const FString& Dest = TargetFileInfo.TargetFile;

					const FString SrcBaseFileName = FPaths::GetBaseFilename(Src);

					// SlowTask
					{
						SlowTask.EnterProgressFrame(0.f, FText::FromString(FString::Printf(TEXT("%s %s")
							, (ImportSetting.bExportMode ? TEXT("Exporting") : TEXT("Importing"))
							, *SrcBaseFileName)));
						if (SlowTask.ShouldCancel())
						{
							bSessionRequestToCancel = true;
							bFailed = true;
							ECB_LOG(Error, TEXT("[RunImportTasks] Canceling %s"), *SrcBaseFileName);
							break;
						}
					}

					MapRollbackSourceAndTarget(Task, *Src, *Dest);

					if (bool* ImportReuslt = ImportedFilesResult.Find(FName(*Src)))
					{
						bool bSuccessImported = *ImportReuslt;
						if (bSuccessImported)
						{
							ECB_LOG(Warning, TEXT("   => %s was already imported to %s, skip."), *Src, *Dest);
							continue;
						}
						else
						{
							ECB_LOG(Error, TEXT("   => %s already failed to import"), *Src);
							bFailed = true;
							break;
						}
					}

					ImportedFilesResult.FindOrAdd(FName(*Src)) = true;

#if ECB_FEA_IMPORT_ROLLBACK
					const bool bNeedBackup = ImportSetting.bRollbackIfFailed && ImportSetting.bOverwriteExistingFiles && TargetFileInfo.bTargeFileWasExist && TargetFileInfo.bSourceTargetHasSameClass;
					if (bNeedBackup)
					{
						const FString Backup = GenerateBackupFilePath(TargetFileInfo.TargetFile);
						bool bBackuped = AddRollbackBackup(Task, *Dest, *Backup);

						// SlowTask
						{
							SlowTask.EnterProgressFrame(1.f / 2, FText::FromString(FString::Printf(TEXT("Backing Up %s"), *SrcBaseFileName)));
							if (SlowTask.ShouldCancel())
							{
								bSessionRequestToCancel = true;
								bFailed = true;
								ECB_LOG(Warning, TEXT("[RunImportTasks] Cancel backup %s"), *SrcBaseFileName);
								break;
							}
						}

						if (!bBackuped)
						{
							ECB_LOG(Error, TEXT("   => Failed to backup %s to %s"), *Dest, *Backup);
							ImportedFilesResult.FindOrAdd(FName(*Src)) = false;

							FailMessage = FString::Printf(TEXT("Failed to backup file: %s."), *FPaths::GetCleanFilename(TargetFileInfo.TargetFile));
							bFailed = true;
							break;
						}
						else
						{
							ECB_LOG(Display, TEXT("   => Success to backup %s to %s for rollback"), *Dest, *Backup);
						}
					}
#endif
					const bool bCanSkip = !ImportSetting.bOverwriteExistingFiles && TargetFileInfo.bTargeFileWasExist;

					if (bCanSkip)
					{
						ECB_LOG(Warning, TEXT("   => Skip to existing file: %s according to import setting"), *TargetFileInfo.TargetFile);

						FString MainSourceFile = Task.GetMainSourceFilePath();
						if (MainSourceFile.Equals(TargetFileInfo.SourceFile))
						{
							TargetFileInfo.bMainAsset = true;
						}
						TargetFileInfo.bSkipped = true;
						ImportedFilesInSession.Add(TargetFileInfo);
						continue;
					}

					if (ImportSetting.bOverwriteExistingFiles && TargetFileInfo.bTargeFileWasExist && !TargetFileInfo.bSourceTargetHasSameClass)
					{
						ECB_LOG(Error, TEXT("   => Not allow to overwrite %s with %s"), *Dest, *Src);
						ImportedFilesResult.FindOrAdd(FName(*Src)) = false;

						FailMessage = FString::Printf(TEXT("Not allow to overwrite %s."), *FPaths::GetCleanFilename(TargetFileInfo.TargetFile));

						bFailed = true;
						break;
					}

					const bool bOverwriteExisting = TargetFileInfo.bTargeFileWasExist;

					FString* PluginRootForReloadPackage = nullptr;
					{
						if (TargetFileInfo.bPluginAsset)
						{
							PluginRootForReloadPackage = &TargetFileInfo.PluginRootWithSlash;
						}
					}

					const bool bCopied = AddRollbackNewFile(Task, *Src, *Dest, bOverwriteExisting, PluginRootForReloadPackage);

					// SlowTask
					{
						SlowTask.EnterProgressFrame(1.f / 2, FText::FromString(FString::Printf(TEXT("Adding %s"), *SrcBaseFileName)));
						if (SlowTask.ShouldCancel())
						{
							bSessionRequestToCancel = true;
							bFailed = true;
							ECB_LOG(Warning, TEXT("[RunImportTasks] Cancel adding %s"), *SrcBaseFileName);
							break;
						}
					}

					if (!bCopied)
					{
						ECB_LOG(Error, TEXT("   => Failed to copy %s to %s"), *Src, *Dest);
						ImportedFilesResult.FindOrAdd(FName(*Src)) = false;

						FailMessage = FString::Printf(TEXT("Failed to copy file: %s."), *FPaths::GetCleanFilename(TargetFileInfo.SourceFile));

						bFailed = true;
						break;
					}
					else
					{
						ECB_LOG(Display, TEXT("   => Success to copy %s to %s"), *Src, *Dest);

						// Is Main Asset
						if (TargetFileInfo.SourceFile.Equals(Task.GetMainSourceFilePath()))
						{
							TargetFileInfo.bMainAsset = true;
						}
						// Or dependency
						else
						{
							++NumSuccessImportedDependencyAssets;
						}

						ImportedFilesInSession.Add(TargetFileInfo);
					}
				}

				if (bFailed)
				{
					ECB_LOG(Error, TEXT("Failed to imported asset: %s"), *Task.GetMainSourceFilePath());
#if ECB_FEA_IMPORT_ROLLBACK
					// Failed the task
					MarkTaskFailed(Task, FailMessage);
#endif
				}
				else
				{
					SetTaskSuccessImported(Task);
					ECB_LOG(Display, TEXT("Sucess to imported asset: %s"), *Task.GetMainSourceFilePath());
				}
			}

#if ECB_WIP_IMPORT_FOLDER_COLOR
			// Import Folder Color
			if (Task.IsSuccessImported() || Task.IsSkipped())
			{
				TMap<FString, FExtAssetImportTargetFolderInfo>& TargetFoldersMap = Task.ImportInfo.TargetColoredFolders;
				ECB_LOG(Display, TEXT("Importing %d colored folders"), TargetFoldersMap.Num());
				for (auto& Pair : TargetFoldersMap)
				{
					if (!ImportedColoredFolders.Contains(Pair.Key))
					{
						ECB_LOG(Display, TEXT("  Importing %s"), *Pair.Key);

						auto& TargetFolderInfo = Pair.Value;

						ECB_LOG(Display, TEXT(" %s, %s, %d, %s"), *TargetFolderInfo.TargetFolderPacakgeName, *TargetFolderInfo.TargetFolderColor.ToString(), TargetFolderInfo.bTargetFolderColorExist, *TargetFolderInfo.ExistingTargetFolderColor.ToString());

#if ECB_WIP_IMPORT_FOLDER_COLOR_OVERRIDE
						AssetViewUtils::SetPathColor(TargetFolderInfo.TargetFolderPacakgeName, TOptional<FLinearColor>()); // clear PatchColors cache
						AssetViewUtils::SetPathColor(TargetFolderInfo.TargetFolderPacakgeName, TOptional<FLinearColor>(FLinearColor(TargetFolderInfo.TargetFolderColor)));
#else
						ExtContentBrowserUtils::SaveColor(TargetFolderInfo.TargetFolderPacakgeName, nullptr); // clear PatchColors cache
						ExtContentBrowserUtils::SaveColor(TargetFolderInfo.TargetFolderPacakgeName, MakeShareable(new FLinearColor(TargetFolderInfo.TargetFolderColor)), true);
#endif
						Task.RollbackInfo.Backup(TargetFolderInfo);
						SessionRollbackInfo.Backup(TargetFolderInfo);
						
						ImportedColoredFolders.Add(Pair.Key);
					}
					else
					{
						ECB_LOG(Display, TEXT("  Skipping %s"), *Pair.Key);
					}
				}
			}
#endif
			
		} // end of ImportTasks Loop


	}

	void ValidateSyncRedirectImportResult(FScopedSlowTask& SlowTask, TArray<FAssetData>& OutSuccessImportedAssets, TArray<FAssetData>& OutSuccessImportedMainAssets)
	{	
		if (SlowTask.ShouldCancel())
		{
			ECB_LOG(Warning, TEXT("[ValidateAndSyncImportResult] Canceled"));
			return;
		}

		// Check num of files to validate or sync
		{
			const bool bIncludeSkipeed = ImportSetting.bSyncExistingAssets; 
			int32 NumFilesToValidateOrSync = NumImportedFiles(bIncludeSkipeed);

			ECB_LOG(Display, TEXT("[PostImportTasks] validate and sync %d imported files."), NumFilesToValidateOrSync);

			if (NumFilesToValidateOrSync <= 0)
			{
				return;
			}
		}
		
		// Register to asset registry, and redirect
		{
			SyncWithAssetRegistryAndRedirect(SlowTask);
		}

		// Validate by search with asset registry
		TArray<FAssetData> SuccessImportedAssets;
		TArray<FAssetData> SuccessImportedMainAssets;
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			const bool bIncludeSkipped = ImportSetting.bSyncExistingAssets;
			TSet<FString> AllImportedFilePaths = GetImportedFilePaths(bIncludeSkipped);
			TSet<FString> AllImportedMainAssetFilePaths = GetImportedMainAssetFilePaths(bIncludeSkipped);

			for (const auto& ImportedFilePath : AllImportedFilePaths)
			{
				const bool bMainAsset = AllImportedMainAssetFilePaths.Contains(ImportedFilePath);

				TArray<FAssetData> AssetDatas;
				AssetRegistry.GetAssetsByPackageName(*FPackageName::FilenameToLongPackageName(ImportedFilePath), AssetDatas, /*bIncludeOnlyOnDiskAssets*/false);
				if (AssetDatas.Num() > 0 && AssetDatas[0].IsValid())
				{
					SuccessImportedAssets.Emplace(AssetDatas[0]);

					// Is Main Asset?
					if (bMainAsset)
					{
						SuccessImportedMainAssets.Emplace(AssetDatas[0]);

						// update task
						SetTaskAssetDataByImportFilePath(ImportedFilePath, AssetDatas[0]);
					}
				}
				else
				{
#if ECB_FEA_IMPORT_ROLLBACK
					// Make all tasks failed if has this file as dependency
					ECB_LOG(Display, TEXT("Can't find asset data for imported:%s, mark task as failed."), *FPaths::GetCleanFilename(ImportedFilePath));
					FString FailMessage = FString::Printf(TEXT("%s is not a valid asset."), *FPaths::GetCleanFilename(ImportedFilePath));
					MarkTasksFailedByImportFilePath(ImportedFilePath, FailMessage);
#endif
				}
			}
		}

		// Sync Assets in Content Browser
		if (ImportSetting.bSyncAssetsInContentBrowser)
		{
			ECB_LOG(Display, TEXT("Syncing %d assets in content browser."), SuccessImportedAssets.Num());
			{
				FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				const bool bSyncMainAssetsOnly = true; // todo: sync all imported assets (with dependencies) or main asset only?
				if (bSyncMainAssetsOnly)
				{
					ContentBrowserModule.Get().SyncBrowserToAssets(SuccessImportedMainAssets);
				}
				else
				{
					ContentBrowserModule.Get().SyncBrowserToAssets(SuccessImportedAssets);
				}
			}
		}

		OutSuccessImportedAssets = SuccessImportedAssets;
		OutSuccessImportedMainAssets = SuccessImportedMainAssets;
	}

	void PostImport(FScopedSlowTask& SlowTask)
	{
		// Rollback
		if (bSessionRequestToCancel)
		{
			ECB_LOG(Warning, TEXT("[PostImport] SlowTask Cancelled, Rollback all taks!"));
			MarkAllTaskFailed();
		}

		if (bSessionRequestToCancel || ImportSetting.bRollbackIfFailed)
		{
			bool bRollbackOnlyFailedTasks = false;/*true*/; // todo: make it an import option?

			if (bRollbackOnlyFailedTasks)
			{
				RollbackFailedTasks(SlowTask);
			}
			else if (HasAnyTaskFailed())// rollback whole session if any task failed
			{
				RollbackSession(SlowTask);
			}
		}

		// Cleanup: delete all backups files generated in this session
		if (!ImportSetting.bDumpAsset)
		{
			Cleanup();
		}
		
		// Notify user
		if (!ImportSetting.bSilentMode)
		{
			NotifyResult();
		}
	}

	void LoadAssets(FScopedSlowTask& SlowTask, TArray<FAssetData>& InImportedAssets)
	{
		if (ImportSetting.bLoadAssetAfterImport && !bSessionRollbacked)
		{
			int32 NumAssets = InImportedAssets.Num();
			if (NumAssets > 0)
			{
				float TaskStep = 1.0f / NumAssets;
				for (auto& AssetData : InImportedAssets)
				{
					bool bSkipLoad = false;
					if (AssetData.AssetClassPath.GetPackageName() == UWorld::StaticClass()->GetFName())
					{
						bSkipLoad = true;
						continue;
					}
					FText TaskMessage = FText::FromString(FString::Printf(TEXT("%s %s..."), (bSkipLoad ? TEXT("Skip loading") : TEXT("Loading")), *AssetData.AssetName.ToString()));
					SlowTask.EnterProgressFrame(TaskStep, TaskMessage);

					if (!bSkipLoad)
					{
						AssetData.GetAsset();
					}
				}
			}
			
#if 0
			for (auto& Task : ImportTasks)
			{
				const FName AssetFilePath = *Task.GetMainSourceFilePath();

				const bool bShouldLoadAsset = Task.IsSuccessImported() || (ImportSetting.bSyncExistingAssets && Task.IsSkipped());

				if (bShouldLoadAsset && Task.ImportedMainAssetData.IsValid())
				{
					// don't load a level
					if (FPaths::GetExtension(Task.GetMainSourceFilePath(), /*IncludeDot*/ true).Equals(FExtAssetSupport::MapPackageExtension, ESearchCase::IgnoreCase))
					{
						continue;
					}
					if (!Task.ImportedMainAssetData.IsAssetLoaded())
					{
						SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Loading %s..."), *FPaths::GetBaseFilename(AssetFilePath.ToString()))));
						Task.ImportedMainAssetData.GetAsset();
					}
				}
			}
#endif
		}
	}

	void AddImportedAssetsToCollection(const TArray<FAssetData>& InAssets)
	{
		if (ImportSetting.bAddImportedAssetsToCollection)
		{
			FExtCollectionUtil::AddAssetsToCollection(InAssets
				, ImportSetting.ImportedUAssetCollectionName
				, ImportSetting.bUniqueCollectionNameForEachImportSession
			);
		}
	}

	void PlaceImportedMainAssetsToLevelEditor(const TArray<FAssetData>& InAssets)
	{
		if (ImportSetting.bPlaceImportedAssets)
		{
			FAssetDataUtil::PlaceToCurrentLevelViewport(InAssets);
		}
	}

	void DumpImportedMainAssetsViaExportDialog(const TArray<FAssetData>& InAssets)
	{
		if (ImportSetting.bDumpAsset)
		{
			FAssetDataUtil::ExportAssetsWithDialog(InAssets);
		}
	}

	void RunZipImportTasks()
	{
		int32 NumImportTasks = ImportTasks.Num();
		ECB_LOG(Display, TEXT("[RunImportTasks] %d tasks."), NumImportTasks);
		if (NumImportTasks <= 0)
		{
			return;
		}

		// All imported files including dependency file path to result map
		TMap<FName, bool> ImportedFilesResult;

		for (auto& Task : ImportTasks)
		{
			const FName AssetFilePath = *Task.GetMainSourceFilePath();

			if (!Task.IsNotStarted())
			{
				continue;
			}

			ECB_LOG(Display, TEXT("Importing %s with %d dependencies will be imported)"), *AssetFilePath.ToString(), Task.ImportInfo.DependencyInfo.ValidDependentFiles.Num());

			bool bFailed = false;
			FString FailMessage;

			for (FExtAssetImportTargetFileInfo TargetFileInfo : Task.ImportInfo.TargetFiles)
			{
				const FString& Src = TargetFileInfo.SourceFile;
				const FString& Dest = TargetFileInfo.TargetFile;

				if (bool* ImportReuslt = ImportedFilesResult.Find(FName(*Src)))
				{
					bool bSuccessImported = *ImportReuslt;
					if (bSuccessImported)
					{
						ECB_LOG(Warning, TEXT("   => %s was already imported to %s, skip."), *Src, *Dest);
						continue;
					}
					else
					{
						ECB_LOG(Error, TEXT("   => %s already failed to import"), *Src);
						bFailed = true;
						break;
					}
				}

				ImportedFilesResult.FindOrAdd(FName(*Src)) = true;
				{
					ECB_LOG(Display, TEXT("   => Success to dry copy %s to %s"), *Src, *Dest);

					// Is Main Asset
					if (TargetFileInfo.SourceFile.Equals(Task.GetMainSourceFilePath()))
					{
						TargetFileInfo.bMainAsset = true;
					}
					// Or dependency
					else
					{
						++NumSuccessImportedDependencyAssets;
					}

					ImportedFilesInSession.Add(TargetFileInfo);
				}
			}

			if (bFailed)
			{
				ECB_LOG(Error, TEXT("Failed to imported asset: %s"), *Task.GetMainSourceFilePath());
#if ECB_FEA_IMPORT_ROLLBACK
				// Failed the task
				MarkTaskFailed(Task, FailMessage);
#endif
			}
			else
			{
				SetTaskSuccessImported(Task);
				ECB_LOG(Display, TEXT("Sucess to imported asset: %s"), *Task.GetMainSourceFilePath());
			}
		}
	}

	void ZipImportedFiles(const FString& SaveFileName)
	{
		TArray<FString> ImportedFilePaths = GetImportedFilePaths().Array();

		if (ImportedFilePaths.Num() > 0)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			IFileHandle* ZipFile = PlatformFile.OpenWrite(*SaveFileName);
			if (ZipFile)
			{
				FZipArchiveWriter* ZipWriter = new FZipArchiveWriter(ZipFile);
				
				TArray<FString>& FilesToArchive = ImportedFilePaths;

				const FString RootDir = ImportSetting.TargetContentDir;
				const int32 SkipLen = RootDir.Len() + 1;

				for (FString& FileName : FilesToArchive)
				{
					TArray<uint8> FileData;
					FFileHelper::LoadFileToArray(FileData, *FileName);
					FString RelativeFileName = FileName.Mid(SkipLen);

					ZipWriter->AddFile(RelativeFileName, FileData, FDateTime::Now());
				}

				delete ZipWriter;
				ZipWriter = nullptr;
			}
		}
	}

	void ZipSourceFiles(const FString& SaveFileName)
	{
		TArray<FExtAssetImportTargetFileInfo>& ImportedFileInfos = ImportedFilesInSession;

		if (ImportedFileInfos.Num() > 0)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			IFileHandle* ZipFile = PlatformFile.OpenWrite(*SaveFileName);
			if (ZipFile)
			{
				FZipArchiveWriter* ZipWriter = new FZipArchiveWriter(ZipFile);

				for (const FExtAssetImportTargetFileInfo& FileInfo : ImportedFileInfos)
				{
					const FString& RootDir = FileInfo.AssetContentDir.ToString();
					const int32 SkipLen = RootDir.Len() + 1;

					FString FileName = FileInfo.SourceFile;
					TArray<uint8> FileData;
					FFileHelper::LoadFileToArray(FileData, *FileName);
					FString RelativeFileName = FileName.Mid(SkipLen);

					ZipWriter->AddFile(RelativeFileName, FileData, FDateTime::Now());
				}

				delete ZipWriter;
				ZipWriter = nullptr;
			}
		}
	}

	bool HasAnyTaskFailed() const
	{
		for (const auto& Task : ImportTasks)
		{
			if (!Task.IsSuccessImported() && !Task.IsSkipped())
			{
				return true;
			}
		}
		return false;
	}

	void RollbackFailedTasks(FScopedSlowTask& SlowTask)
	{
		bool bHasAnySuccessdTask = false;
		for (auto& Task : ImportTasks)
		{
			if (!Task.IsSuccessImported() && !Task.IsSkipped())
			{
				SlowTask.EnterProgressFrame(0.0f, FText::FromString(FString::Printf(TEXT("Rollback %s"), *FPaths::GetBaseFilename(Task.GetMainSourceFilePath()))));
				Task.Rollback();
			}
			else
			{
				bHasAnySuccessdTask = true;
			}
		}

		// mark session rollbacked if all tasks failed or skipped
		if (!bHasAnySuccessdTask)
		{
			bSessionRollbacked = true;
		}
	}

	void RollbackSession(FScopedSlowTask& SlowTask)
	{
		SlowTask.EnterProgressFrame(0.0f, LOCTEXT("RollbackImportSession", "Rollback import session..."));

		SessionRollbackInfo.RollbackAll();

		bSessionRollbacked = true;

		MarkAllTaskRollbacked();
	}

	void Cleanup()
	{
		SessionRollbackInfo.DeleteBackupFiles();
	}

	bool IsSessionRequestToCancel() const
	{
		return bSessionRequestToCancel;
	}

	bool IsSessionRollbacked() const
	{
		return bSessionRollbacked;
	}

	bool IsPackageRedirected(const FString& InAssetPackageName) const
	{
		return RedirectInfo.IsRedirected(InAssetPackageName);
	}

	int32 GetNumAllAssetsToImport() const
	{
		TSet<FString> AllSourceFiles;
		for (const auto& Task : ImportTasks)
		{
			for (const FExtAssetImportTargetFileInfo& TargetFileInfo : Task.ImportInfo.TargetFiles)
			{
				AllSourceFiles.Emplace(TargetFileInfo.SourceFile);
			}
		}
		return AllSourceFiles.Num();
	}

	int32 NumImportedFiles(bool bIncludeSkipped = false) const
	{
		int32 Num = 0;
		for (const FExtAssetImportTargetFileInfo& TargetFileInfo : ImportedFilesInSession)
		{
			if (!TargetFileInfo.bSkipped || bIncludeSkipped)
			{
				++Num;
			}
		}
		return Num;
	}

	TSet<FString> GetImportedFilePaths(bool bIncludeSkipped = false) const
	{
		TSet<FString> ImportedFilePaths;
		for (const FExtAssetImportTargetFileInfo& TargetFileInfo : ImportedFilesInSession)
		{
			if (!TargetFileInfo.bSkipped || bIncludeSkipped)
			{
				ImportedFilePaths.Add(TargetFileInfo.TargetFile);
			}
		}
		return ImportedFilePaths;
	}

	TSet<FString> GetImportedMainAssetFilePaths(bool bIncludeSkipped = false) const
	{
		TSet<FString> ImportedFilePaths;
		for (const FExtAssetImportTargetFileInfo& TargetFileInfo : ImportedFilesInSession)
		{
			if (TargetFileInfo.bMainAsset && (!TargetFileInfo.bSkipped || bIncludeSkipped))
			{
				ImportedFilePaths.Add(TargetFileInfo.TargetFile);
			}
		}
		return ImportedFilePaths;
	}

	void NotifyResult()
	{
		struct Local
		{
			static FString GetMainAssetNameStrByTaskStatus(const TArray<FExtAssetImportTask>& InTasks, FExtAssetImportTask::EImportTaskStatus TaskStatus)
			{
				FString MainAssetNameStr;
				int32 NumFound = 0;
				for (auto& Task : InTasks)
				{
					if (Task.Status == TaskStatus)
					{
						if (NumFound == 0)
						{
							MainAssetNameStr = Task.SourceAssetData.AssetName.ToString();
						}
						++NumFound;
					}
				}

				if (NumFound > 0)
				{
					NumFound > 1 ? MainAssetNameStr += TEXT(", ...") : MainAssetNameStr += TEXT(",");
				}
				return MainAssetNameStr;
			}
		};

		const int32 NumImportTaskSuccess = NumTasksByStatus(FExtAssetImportTask::EImportTaskStatus::Imported);
		const int32 NumMainAssetsImportSuccess = NumImportTaskSuccess;
		const int32 NumImportTaskSkipped = NumTasksByStatus(FExtAssetImportTask::EImportTaskStatus::Skipped);
		const int32 NumImportTaskFailed = NumTasksByStatus(FExtAssetImportTask::EImportTaskStatus::Failed);
		const int32 NumImportTaskRollbacked = NumTasksByStatus(FExtAssetImportTask::EImportTaskStatus::Rollbacked);


		FString MainSuccessdAssetName = Local::GetMainAssetNameStrByTaskStatus(ImportTasks, FExtAssetImportTask::EImportTaskStatus::Imported);
		FString MainSkippedAssetName = Local::GetMainAssetNameStrByTaskStatus(ImportTasks, FExtAssetImportTask::EImportTaskStatus::Skipped);
		FString MainFailedAssetName = Local::GetMainAssetNameStrByTaskStatus(ImportTasks, FExtAssetImportTask::EImportTaskStatus::Failed);
		FString MainRollabckedAssetName = Local::GetMainAssetNameStrByTaskStatus(ImportTasks, FExtAssetImportTask::EImportTaskStatus::Rollbacked);
		

		FString MainAssetSuccessMessage = NumMainAssetsImportSuccess > 0 ? FString::Printf(TEXT("%s %d main asset(s) imported. "), *MainSuccessdAssetName, NumMainAssetsImportSuccess) : TEXT("");
		FString DependencySuccessMessage = (!ImportSetting.bDirectCopyMode && NumSuccessImportedDependencyAssets > 0) ? FString::Printf(TEXT("%d dependencies imported. "), NumSuccessImportedDependencyAssets) : TEXT("");
		FString FailedMessage = NumImportTaskFailed > 0 ? FString::Printf(TEXT("%s %d failed to import (%s). "), *MainFailedAssetName, NumImportTaskFailed, *LastFailedMessage) : TEXT("");
		FString SkippedMessage = NumImportTaskSkipped > 0 ? FString::Printf(TEXT("%s %d skipped to import (%s). "), *MainSkippedAssetName, NumImportTaskSkipped, *LastSkipMessage) : TEXT("");
		FString RollbackMessage = NumImportTaskRollbacked > 0 ? FString::Printf(TEXT("%s %d import task(s) rollbacked. "), *MainRollabckedAssetName, NumImportTaskRollbacked) : TEXT("");

		FString ResultMessage = FString::Printf(TEXT("%s%s%s%s%s"), *MainAssetSuccessMessage, *DependencySuccessMessage, *FailedMessage, *SkippedMessage, *RollbackMessage);

		ExtContentBrowserUtils::NotifyMessage(FText::FromString(ResultMessage), /*bPrintToConsole*/true);
	}

public:

	// Tasks
	TArray<FExtAssetImportTask> ImportTasks;

	// Rollback Info
	FExtAssetRollbackInfo SessionRollbackInfo;

	// Import Result
	TArray<FExtAssetImportTargetFileInfo> ImportedFilesInSession; // All imported/skipped dependency files include main asset

private:

	const FString GenerateBackupFilePath(const FString& InFilePath) const
	{
		FString SessionBackupFile = InFilePath + TEXT(".impbak"); // Fallback

		//if (FPaths::MakePathRelativeTo(RelativePath, *ProjectContentDir)) // FPaths::MakePathRelativeTo assume InRelativeTo has end slash which not apply here
		//if (InFilePath.StartsWith(ProjectContentDir))
		if (FPathsUtil::IsFileInDir(InFilePath, ImportSetting.TargetContentDir))
		{
			FString RelativePath = InFilePath.Mid(ImportSetting.TargetContentDir.Len() + 1);
			SessionBackupFile = FPaths::Combine(SesstionTempDir, RelativePath);
		}
		else // Cant make relative path, backup with unique file extension
		{
			FString UniqueId = FString::FromInt(FGuid::NewGuid().A);
			FString UniqueFileExt = TEXT(".") + UniqueId;
			SessionBackupFile = FPaths::Combine(SesstionTempDir, FPaths::GetBaseFilename(InFilePath), UniqueFileExt);
		}
		
		return SessionBackupFile;
	}

	void MapRollbackSourceAndTarget(FExtAssetImportTask& InTask, const FName& InSource, const FName& InTarget)
	{
		InTask.RollbackInfo.MapSourceToTarget(InSource, InTarget);
		SessionRollbackInfo.MapSourceToTarget(InSource, InTarget);
	}

	bool AddRollbackBackup(FExtAssetImportTask& InTask, const FName& InTarget, const FName& InBackup)
	{
// 		const bool bReplace = true;
// 		const bool bEvenIfReadOnly = false;
// 		const bool bAttributes = false;
// 		const bool bDoNotRetryOrError = false;
// 		bool bResult = IFileManager::Get().Move(/*Dest=*/*InBackup.ToString(), /*Src=*/ *InTarget.ToString(), bReplace, bEvenIfReadOnly, bAttributes, bDoNotRetryOrError);

		const bool bReplace = true;
		const bool bEvenIfReadOnly = true;
		const bool bAttributes = true;
		FCopyProgress* const CopyProgress = nullptr;
		uint32 CopyResult = IFileManager::Get().Copy(/*Dest=*/ *InBackup.ToString(), /*Src=*/ *InTarget.ToString(), bReplace, bEvenIfReadOnly, bAttributes, CopyProgress, FILEREAD_AllowWrite, FILEWRITE_AllowRead);
		bool bResult = CopyResult == COPY_OK;

		if (bResult)
		{
			InTask.RollbackInfo.Backup(InTarget, InBackup);
			SessionRollbackInfo.Backup(InTarget, InBackup);
		}
		return bResult;
	}

	bool AddRollbackNewFile(FExtAssetImportTask& InTask, const FName& InSource, const FName& InTarget, bool bReplaceExisting, FString* InPluginRootForReloadPackage /*todo: include all invalid roots? */)
	{
		TArray<UPackage*> ReplacedPackages;
		if (bReplaceExisting)
		{
			FExtPackageUtils::UnloadPackage({ InTarget.ToString() }, &ReplacedPackages);
		}

		struct Local
		{
			static bool GetCreatedDir(const FName& InNewFile, FName& OutNewCreatedDir)
			{
				FString NewFile = InNewFile.ToString();
				FString Dir = FPaths::GetPath(NewFile);

				ECB_LOG(Display, TEXT("NewFile: %s, Dir: %s, Exist? %d"), *NewFile, *Dir, FPaths::DirectoryExists(Dir));

				if (!FPaths::DirectoryExists(Dir))
				{
					FString CreatedDir = Dir;
					int32 FindIndex = Dir.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, Dir.Len());
					for (; FindIndex != INDEX_NONE; FindIndex = Dir.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, FindIndex))
					{
						FString SubDir = Dir.Mid(0, FindIndex);
						const bool bSubDirExist = FPaths::DirectoryExists(SubDir);
						if (bSubDirExist)
						{
							break;
						}
						CreatedDir = SubDir;
						ECB_LOG(Display, TEXT("    => SubDir: %s, Exist? %d"), *SubDir, FPaths::DirectoryExists(SubDir));
					}
					OutNewCreatedDir = *CreatedDir;
					return true;
				}
				return false;
			}
		};
		FName NewCreatedDir;
		const bool HasNewCreatedDir = Local::GetCreatedDir(InTarget, NewCreatedDir);

		const bool bReplace = bReplaceExisting;
		const bool bEvenIfReadOnly = false;
		const bool bAttributes = false;
		FCopyProgress* const CopyProgress = nullptr;
		uint32 CopyResult = IFileManager::Get().Copy(/*Dest=*/ *InTarget.ToString(), /*Src=*/ *InSource.ToString(), bReplace, bEvenIfReadOnly, bAttributes, CopyProgress, FILEREAD_AllowWrite, FILEWRITE_AllowRead);
		//uint32 CopyResult = IFileManager::Get().Copy(/*Dest=*/ *InTarget.ToString(), /*Src=*/ *InSource.ToString(), true, true);
		bool bResult = CopyResult == COPY_OK;
		if (bResult && !bReplaceExisting)
		{
			InTask.RollbackInfo.MapNewFile(InSource, InTarget, NewCreatedDir);
			SessionRollbackInfo.MapNewFile(InSource, InTarget, NewCreatedDir);
		}

		if (bReplaceExisting && !bResult)
		{
			ECB_LOG(Error, TEXT("Failed to overwrite: %s"), *InTarget.ToString());
		}

		if (ImportSetting.bLoadAssetAfterImport && !ImportSetting.bSandboxMode)
		{
			struct Local
			{
				static void GetInvalidRoots(const TArray<UPackage*>& InReplacedPackages, TArray<FString>& OutInValidRootsWithSlash)
				{
					for (const UPackage* Package : InReplacedPackages)
					{
						if (!Package)
						{
							continue;
						}
						
						FString PackagePath = Package->GetPathName();
						if (FPackageName::IsValidLongPackageName(PackagePath))
						{
							continue;
						}
						
						FString InvalidRoot = FExtAssetDataUtil::GetPackageRootFromFullPackagePath(PackagePath);
						if (!InvalidRoot.IsEmpty())
						{
							OutInValidRootsWithSlash.AddUnique(InvalidRoot);
							ECB_LOG(Display, TEXT("[GetInvalidRoots]: %s -> %s"), *PackagePath, *InvalidRoot);
						}
					}
				}

				static void MountInvalidRoots(const TArray<FString>& InValidRootsWithSlash)
				{
					for (const FString& MountPoint : InValidRootsWithSlash)
					{
						FPackageName::RegisterMountPoint(MountPoint /*TEXT("/SomeRoot/")*/, FExtAssetData::GetUAssetBrowserTempDir());
					}
				}

				static void UnMountInvalidRoots(const TArray<FString>& InValidRootsWithSlash)
				{
					for (const FString& MountPoint : InValidRootsWithSlash)
					{
						FPackageName::UnRegisterMountPoint(MountPoint /*TEXT("/SomeRoot/")*/, FExtAssetData::GetUAssetBrowserTempDir());
					}
				}

				static bool IsValidRoot(const FString& InRoot)
				{
					if (FPackageName::IsValidLongPackageName(InRoot))
					{
						return true;
					}
					return false;
				}
			};

			TArray<FString> InValidRootsWithSlash;
			//Local::GetInvalidRoots(ReplacedPackages, InValidRootsWithSlash);
			if (InPluginRootForReloadPackage && !Local::IsValidRoot(*InPluginRootForReloadPackage))
			{
				InValidRootsWithSlash.AddUnique(*InPluginRootForReloadPackage);
			}
			Local::MountInvalidRoots(InValidRootsWithSlash);
			{
				// Make sure invalid root (e.g. not import from not installed plugin) is mount first before reload, get rid of warning messages
				UPackageTools::ReloadPackages(ReplacedPackages);
			}
			Local::UnMountInvalidRoots(InValidRootsWithSlash);
		}

		return bResult;
	}

	void RollbackImportTask(FExtAssetImportTask& InTask)
	{
		ECB_LOG(Error, TEXT("[RollbackImportTask] %s, bAlreaydRoolbacked? %d"), *InTask.GetMainSourceFilePath(), InTask.IsRollbacked());

		if (!InTask.IsRollbacked())
		{
			InTask.Rollback();
		}
	}

	void MarkAllTaskFailed()
	{
		for (auto& Task : ImportTasks)
		{
			Task.Status = FExtAssetImportTask::EImportTaskStatus::Failed;
		}
	}

	void MarkAllTaskRollbacked()
	{
		for (auto& Task : ImportTasks)
		{
			Task.Status = FExtAssetImportTask::EImportTaskStatus::Rollbacked;
		}
	}

	void MarkTaskFailed(FExtAssetImportTask& InTask, const FString& InFailedMessage)
	{
		ECB_LOG(Error, TEXT("[MarkTaskFailed] %s"), *InTask.GetMainSourceFilePath());

		LastFailedMessage = InFailedMessage;

		InTask.Status = FExtAssetImportTask::EImportTaskStatus::Failed;
	}

	void MarkTasksFailedByImportFilePath(const FString& ImportedFilePath, const FString& InFailedMessage)
	{
		for (auto& Task : ImportTasks)
		{
			FExtAssetImportInfo& ImportInfo = Task.ImportInfo;
			for (FExtAssetImportTargetFileInfo& TargetFileInfo : ImportInfo.TargetFiles)
			{
				if (TargetFileInfo.TargetFile.Equals(ImportedFilePath))
				{
					MarkTaskFailed(Task, InFailedMessage);
				}
			}
		}
	}

	void SetTaskAssetDataByImportFilePath(const FString& ImportedFilePath, const FAssetData& InAssetData)
	{
		for (auto& Task : ImportTasks)
		{
			FExtAssetImportInfo& ImportInfo = Task.ImportInfo;
			for (FExtAssetImportTargetFileInfo& TargetFileInfo : ImportInfo.TargetFiles)
			{
				if (TargetFileInfo.TargetFile.Equals(ImportedFilePath))
				{
					Task.ImportedMainAssetData = InAssetData;
				}
			}
		}
	}

	void SetTaskSkipped(FExtAssetImportTask& InTask, const FString& InSkippedMessage)
	{
		InTask.Status = FExtAssetImportTask::EImportTaskStatus::Skipped;
		LastSkipMessage = InSkippedMessage;
	}

	void SetTaskSuccessImported(FExtAssetImportTask& InTask)
	{
		InTask.Status = FExtAssetImportTask::EImportTaskStatus::Imported;
	}

	int32 NumTasksByStatus(FExtAssetImportTask::EImportTaskStatus InStatus) const
	{
		int32 NumTasks = 0;
		for (const auto& Task : ImportTasks)
		{
			if (Task.Status == InStatus)
			{
				++NumTasks;
			}
		}
		return NumTasks;
	}

	void SyncWithAssetRegistryAndRedirect(FScopedSlowTask& SlowTask)
	{
		// Collect all imported package names and sandbox redirect map
		RedirectInfo.CollectImportAndRedirectInfo(ImportedFilesInSession);

		const bool bHasRediretRecord = RedirectInfo.HasRedirectRecord();
		const bool bHasNoRediretPackages = RedirectInfo.HasNoRedirectImportPackages();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		// Force rescan by file paths for fresh new imported assets
		TArray<FString> ImportedFilePaths = GetImportedFilePaths().Array();
		AssetRegistry.ScanFilesSynchronous(ImportedFilePaths, /*bForceRescan*/ true);
		
		// Force rescan by package name in case package name still been cached
		const TArray<FString>& AllImportedPackageNames = RedirectInfo.AllImportedPackageNames;
		AssetRegistry.ScanFilesSynchronous(AllImportedPackageNames, /*bForceRescan*/true);

		// Custom package name solver to redirecting game package to sandbox package for package linker
		int32 PackageResolveDelegateIndex = INDEX_NONE;
		if (bHasRediretRecord)
		{
			PackageResolveDelegateIndex = FCoreDelegates::PackageNameResolvers.Num();
			TDelegate<bool(const FString&, FString&)> PackageResolveDelegate;
			PackageResolveDelegate.BindLambda([this](const FString& InPackageName, FString& OutPackageName)
			{
				for (const auto& Pair : RedirectInfo.RedirectRecords)
				{
					const FString& OrigPackageName = Pair.Key;
					const FString& RedirectToPackageName = Pair.Value.RedirectToPackageName;

					if (InPackageName.Equals(OrigPackageName))
					{
						OutPackageName = RedirectToPackageName;
						return true;
					}
				}
				return false;
			});
			FCoreDelegates::PackageNameResolvers.Add(PackageResolveDelegate);
		}

		struct Local
		{
			static UPackage* FindLoadedPackage(const FString& PackageName)
			{
				UPackage* Package = FindPackage(NULL, *PackageName);
				if (Package)
				{
					ECB_LOG(Display, TEXT("[FindPackage]%s Found, fullyload."), *PackageName);
					Package->FullyLoad();

					return Package;
				}
				return NULL;
			}

			static UPackage* FindOrLoadPackage(const FString& PackageName)
			{
				UPackage* Package = FindPackage(NULL, *PackageName);
				if (Package)
				{
					ECB_LOG(Display, TEXT("[FindOrLoadPackge]%s Found, fullyload."), *PackageName);
					Package->FullyLoad();
				}
				else
				{
					ECB_LOG(Display, TEXT("[FindOrLoadPackge]%s not found, LoadPackage."), *PackageName);
					Package = LoadPackage(NULL, *PackageName, LOAD_None);
				}
				return Package;
			}
		};

		if (bHasNoRediretPackages)
		{
			// Load package if not found in asset registry
			const TArray<FString>& NoRedirectImportedPackageNames = RedirectInfo.NoRedirectImportedPackageNames;
			for (const FString& ImportedPackageName : NoRedirectImportedPackageNames)
			{
				SlowTask.EnterProgressFrame(1.0f / NoRedirectImportedPackageNames.Num(), FText::FromString(FString::Printf(TEXT("Validating %s"), *FPaths::GetBaseFilename(ImportedPackageName))));
				if (SlowTask.ShouldCancel())
				{
					bSessionRequestToCancel = true;
					ECB_LOG(Warning, TEXT("[SyncWithAssetRegistry] Cancel validating %s"), *FPaths::GetBaseFilename(ImportedPackageName));
					break;
				}

				TArray<FAssetData> AssetDatas;
				const bool bFoundInAssetRegistry = AssetRegistry.GetAssetsByPackageName(*ImportedPackageName, AssetDatas, /*bIncludeOnlyOnDiskAssets*/false)
					&& AssetDatas.Num() > 0 && AssetDatas[0].IsValid();

				if (!bFoundInAssetRegistry)
				{
					// Load in memory for sync
					Local::FindOrLoadPackage(ImportedPackageName);
				}
			}
		}
		if (bHasRediretRecord)
		{
			TSet<UPackage*> OrigPackages;
			TSet<UPackage*> RedirectToPackages;
			TSet<UPackage*> NoSkippedRedirectPackagesForSave;
			TArray<UObject*> NoSkippedRedirectObjects;
			TMap<UObject*, UObject*> ObjectsReplaceMap;

			const bool bUnloadRedirectToPackageForReload = true;
			if (bUnloadRedirectToPackageForReload)
			{
				TArray<UPackage*> ReloadingPackages;
				for (const auto& Pair : RedirectInfo.RedirectRecords)
				{
					const FRedirectRecord& RedirectRecord = Pair.Value;
					const FString& RedirectToPackageName = RedirectRecord.RedirectToPackageName;
					if (UPackage* RedirectToPackage = Local::FindLoadedPackage(RedirectToPackageName))
					{
						ReloadingPackages.Add(RedirectToPackage);
					}
// 					const FString& OrigPackageName = Pair.Key;
// 					if (UPackage* OrigPackage = Local::FindLoadedPackage(OrigPackageName))
// 					{
// 						ReloadingPackages.Add(OrigPackage);
// 					}
				}
				UPackageTools::UnloadPackages(ReloadingPackages);
			}

			// Custom PackageResolve only works on non-editor
			GIsEditor = false;
			{
				for (const auto& Pair : RedirectInfo.RedirectRecords)
				{
					const FString& OrigPackageName = Pair.Key;
					const FRedirectRecord& RedirectRecord = Pair.Value;
					const FString& RedirectToPackageName = RedirectRecord.RedirectToPackageName;
					const bool bSkipped = RedirectInfo.SkippedRedirectToPackageNames.Contains(RedirectToPackageName);

					UPackage* OrigPackage = Local::FindOrLoadPackage(OrigPackageName);
					UPackage* RedirectToPackage = Local::FindOrLoadPackage(RedirectToPackageName);

					if (RedirectToPackage)
					{
						const FString& AssetName = RedirectRecord.AssetName;

						RedirectToPackages.Add(RedirectToPackage);

						const bool bFindObjectByAssetName = true;

						if (!bSkipped)
						{
							NoSkippedRedirectPackagesForSave.Add(RedirectToPackage);

							if (bFindObjectByAssetName)
							{
								FString RedirectToObjectName = RedirectToPackageName + TEXT(".") + AssetName;
								UObject* RedirectToObject = RedirectToPackage ? StaticFindObject(UObject::StaticClass(), nullptr, *RedirectToObjectName) : NULL;
								if (RedirectToObject)
								{
									NoSkippedRedirectObjects.Add(RedirectToObject);
								}
							}
							else
							{
								TArray<UPackage*> NewPackages = { RedirectToPackage };
								TArray<UObject*> NewObjects;
								UPackageTools::GetObjectsInPackages(&NewPackages, NewObjects);
								if (NewObjects.Num() > 0)
								{
									NoSkippedRedirectObjects.Add(NewObjects[0]);
								}
							}
						}

						if (OrigPackage)
						{
							OrigPackages.Add(OrigPackage);
							
							if (bFindObjectByAssetName)
							{
								FString OrigObjectName = OrigPackageName + TEXT(".") + AssetName;
								UObject* OrigObject = OrigPackage ? StaticFindObject(UObject::StaticClass(), nullptr, *OrigObjectName) : NULL;

								FString RedirectToObjectName = RedirectToPackageName + TEXT(".") + AssetName;
								UObject* RedirectToObject = RedirectToPackage ? StaticFindObject(UObject::StaticClass(), nullptr, *RedirectToObjectName) : NULL;

								if (OrigObject && RedirectToObject)
								{
									ObjectsReplaceMap.Add(OrigObject, RedirectToObject);
								}
							}
							else
							{
								TArray<UPackage*> ExistingPackages = { OrigPackage };
								TArray<UObject*> ExistingObjects;
								UPackageTools::GetObjectsInPackages(&ExistingPackages, ExistingObjects);

								TArray<UPackage*> NewPackages = { RedirectToPackage };
								TArray<UObject*> NewObjects;
								UPackageTools::GetObjectsInPackages(&NewPackages, NewObjects);

								if (ExistingObjects.Num() > 0 && NewObjects.Num() > 0)
								{
									ObjectsReplaceMap.Add(ExistingObjects[0], NewObjects[0]);
								}
							}
						}
					}
				}

				struct FFixSubObjectReferencesHelper
				{
					static void FixSubObjectReferences(UObject* InObject, const TMap<UObject*, UObject*>& OldToNewInstanceMap)
					{
						// these may have the correct Outer but are not referenced by the CDO's UProperties
						TArray<UObject*> SubObjects;
						GetObjectsWithOuter(InObject, SubObjects,  /*bIncludeNestedObjects*/true);
						for (UObject* SubObject : SubObjects)
						{
							constexpr EArchiveReplaceObjectFlags ArFlags = (EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::TrackReplacedReferences); 
							FArchiveReplaceObjectRef<UObject> Replacer(SubObject, OldToNewInstanceMap, ArFlags);
						}

						// these may have the in-correct Outer but are incorrectly referenced by the CDO's UProperties
						TSet<FInstancedSubObjRef> PropertySubObjectReferences;
						UClass* ObjectClass = InObject->GetClass();
						FFindInstancedReferenceSubobjectHelper::GetInstancedSubObjects(InObject, PropertySubObjectReferences);

						for (UObject* PropertySubObject : PropertySubObjectReferences)
						{
							constexpr EArchiveReplaceObjectFlags ArFlags = (EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::TrackReplacedReferences);
							FArchiveReplaceObjectRef<UObject> Replacer(PropertySubObject, OldToNewInstanceMap, ArFlags);
						}
					}
				};

				for (auto& RedirectToObject : NoSkippedRedirectObjects)
				{
					//UObject* RedirectToObject = Pair.Value;
					{
						constexpr EArchiveReplaceObjectFlags ArFlags = (EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::TrackReplacedReferences);
						FArchiveReplaceObjectRef<UObject> ReplaceAr(RedirectToObject, ObjectsReplaceMap, ArFlags);
					}
					//FFixSubObjectReferencesHelper::FixSubObjectReferences(RedirectToObject, ObjectsReplaceMap);

					// Fix Blueprint CDO
					if (UBlueprint* BlueprintAsset = Cast<UBlueprint>(RedirectToObject))
					{
						UClass* BlueprintClass = BlueprintAsset->GeneratedClass;
						if (BlueprintClass)
						{
							//FFixSubObjectReferencesHelper::FixSubObjectReferences(BlueprintClass, ObjectsReplaceMap);

							ECB_LOG(Display, TEXT("[SyncWithAssetRegistryAndRedirect] Found Blueprint Class %s"), *BlueprintClass->GetName());

							UObject* BlueprintCDO = BlueprintClass->ClassDefaultObject;

							constexpr EArchiveReplaceObjectFlags ArFlags = (EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::TrackReplacedReferences);
							FArchiveReplaceObjectRef<UObject> ReplaceAr(BlueprintCDO, ObjectsReplaceMap, ArFlags);
							//FFixSubObjectReferencesHelper::FixSubObjectReferences(BlueprintCDO, ObjectsReplaceMap);

							FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BlueprintAsset);
						}
					}
				}

				const bool bReplaceAllReferences = false;
				if (bReplaceAllReferences)
				{
					// Main pass to go through and fix-up any references pointing to data from the old package to point to data from the new package
					for (FThreadSafeObjectIterator ObjIter(UObject::StaticClass(), false, RF_NoFlags, EInternalObjectFlags::Garbage); ObjIter; ++ObjIter)
					{
						UObject* PotentialReferencer = *ObjIter;

						// Mutating the old versions of classes can result in us replacing the SuperStruct pointer, which results
						// in class layout change and subsequently crashes because instances will not match this new class layout:
						if (UClass* AsClass = Cast<UClass>(PotentialReferencer))
						{
							if (AsClass->HasAnyClassFlags(CLASS_NewerVersionExists) || AsClass->HasAnyFlags(RF_NewerVersionExists))
							{
								continue;
							}
						}

						constexpr EArchiveReplaceObjectFlags ArFlags = (EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::TrackReplacedReferences);
						FArchiveReplaceObjectRef<UObject> ReplaceAr(PotentialReferencer, ObjectsReplaceMap, ArFlags);
					}
				}

				const bool bResetOrigLoaders = true;
				if (bResetOrigLoaders)
				{
					// Reset linkers of orig packages as they're no longer reliable
					for (UPackage* OrigPackage : OrigPackages)
					{
						if (OrigPackage)
						{
							// Detach the linkers of any loaded packages so that SCC can overwrite the files...
							ResetLoaders(OrigPackage);
							OrigPackage->ClearFlags(RF_WasLoaded);
							OrigPackage->bHasBeenFullyLoaded = false;
							OrigPackage->GetMetaData()->RemoveMetaDataOutsidePackage();
						}
					}
				}
			}
			GIsEditor = true;

			if (RedirectToPackages.Num() > 0 && RedirectInfo.RedirectMap.Num() > 0)
			{
				// Remap soft references
				IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
				AssetTools.RenameReferencingSoftObjectPaths(NoSkippedRedirectPackagesForSave.Array(), RedirectInfo.RedirectMap);

				// Save
				TArray<UPackage*> PackagesToSave;
				{
					for (UPackage* Package : NoSkippedRedirectPackagesForSave)
					{
						if (IsValid(Package) && !UPackage::IsEmptyPackage(Package))
						{
							PackagesToSave.Add(Package);
						}
					}
				}
				TArray<UPackage*> FailedToSave;
				{
					const bool bCheckDirty = false;
					const bool bPromptToSave = false;
					FEditorFileUtils::PromptForCheckoutAndSave(/*RedirectToPackages*/PackagesToSave, bCheckDirty, bPromptToSave, &FailedToSave);
				}
				if (FailedToSave.Num() > 0)
				{
					ECB_LOG(Display, TEXT("%d packages failed to save."), FailedToSave.Num());
				}
				ECB_LOG(Display, TEXT("Saving %d redirected packages."), PackagesToSave.Num());

				if (PackageResolveDelegateIndex != INDEX_NONE && PackageResolveDelegateIndex < FCoreDelegates::PackageNameResolvers.Num())
				{
					FCoreDelegates::PackageNameResolvers.RemoveAt(PackageResolveDelegateIndex);
				}

			}
		}
	}

private:

	struct FRedirectRecord
	{
		FRedirectRecord()
			: OrigPackageName(TEXT(""))
			, RedirectToPackageName(TEXT(""))
			, AssetName(TEXT(""))
			, bMainAsset(false)
		{
		}
		FRedirectRecord(const FString InOrigPackageName, const FString& InRedirectToPackageName, const FString& InAssetName, bool bInMainAsset)
			: OrigPackageName(InOrigPackageName)
			, RedirectToPackageName(InRedirectToPackageName)
			, AssetName(InAssetName)
			, bMainAsset(bInMainAsset)
		{
		}

		FString OrigPackageName;
		FString RedirectToPackageName;
		FString AssetName;
		bool bMainAsset = false;
	};
	struct FRedirectHelper
	{
		void CollectImportAndRedirectInfo(const TArray<FExtAssetImportTargetFileInfo>& InTargetFilesInSession)
		{
			static FString EmptyString(TEXT(""));

			// Collect all imported package names and sandbox redirect map
			for (const FExtAssetImportTargetFileInfo& TargetFileInfo : InTargetFilesInSession)
			{
				const bool bSkipped = TargetFileInfo.bSkipped;
				const bool bMainAsset = TargetFileInfo.bMainAsset;

				const FString& ImportedFilePath = TargetFileInfo.TargetFile;
				const FName& ImportedAssetName = TargetFileInfo.SourceAssetName;
				const FString ImportedPackageName = FPackageName::FilenameToLongPackageName(ImportedFilePath);
				const bool bRedirect = TargetFileInfo.bRedirectMe;
				const FString& RedirectFromPackageName = TargetFileInfo.RedirectFromPackageName;
				const FString& RedirectToPackageName = TargetFileInfo.RedirectToPackageName;;
				
				if (bRedirect && !RedirectFromPackageName.IsEmpty() && !RedirectToPackageName.IsEmpty())
				{
					//FString OrigPackageName = RedirectPackageRootFrom/*TEXT("/Game")*/ + ImportedPackageName.Mid(RedirectPackageRootTo.Len());
					FString AssetName = ImportedAssetName != NAME_None ? ImportedAssetName.ToString() : FPaths::GetBaseFilename(ImportedPackageName);
					// ? check(RedirectToPackageName equals ImportedPackageName)
					AddRedirectRecord(RedirectFromPackageName, RedirectToPackageName, AssetName, bMainAsset);
				}
				else
				{
					NoRedirectImportedPackageNames.Add(ImportedPackageName);
				}

				AllImportedPackageNames.Add(ImportedPackageName);

				if (bSkipped)
				{
					SkippedRedirectToPackageNames.Add(RedirectToPackageName);
				}
			}
		}

		void AddRedirectRecord(const FString& InOrigPackageName, const FString& InRedirectToPackageName, const FString& InAssetName, bool bInMainAsset)
		{
			if (!InOrigPackageName.Equals(InRedirectToPackageName))
			{
				FRedirectRecord Record(InOrigPackageName, InRedirectToPackageName, InAssetName, bInMainAsset);
				RedirectRecords.FindOrAdd(InOrigPackageName) = Record;

				RedirectMap.Add(FSoftObjectPath(InOrigPackageName + TEXT(".") + InAssetName), FSoftObjectPath(InRedirectToPackageName + TEXT(".") + InAssetName));
			}
		}

		bool HasRedirectRecord() const
		{
			return RedirectRecords.Num() > 0;
		}

		bool HasNoRedirectImportPackages() const
		{
			return NoRedirectImportedPackageNames.Num() > 0;
		}


		bool IsRedirected(const FString& InPackageName) const
		{
			if (SkippedRedirectToPackageNames.Contains(InPackageName))
			{
				return false;
			}
			for (const auto& Pair : RedirectRecords)
			{
				const auto& Record = Pair.Value;
				if (Record.RedirectToPackageName.Equals(InPackageName, ESearchCase::CaseSensitive))
				{
					return true;
				}
			}
			return false;
		}

		TMap<FString, FRedirectRecord> RedirectRecords; // OrigPackageName => Record
		TMap<FSoftObjectPath, FSoftObjectPath> RedirectMap;// /Game/SomePackage.SomeAssetName => /UAssetBrowser/Sandbox/SomePackage.SomeAssetName

		TArray<FString> AllImportedPackageNames;
		TArray<FString> NoRedirectImportedPackageNames;
		TSet<FString> SkippedRedirectToPackageNames;
	};
	FRedirectHelper RedirectInfo;

	FUAssetImportSetting ImportSetting;

	FString SesstionTempDir;
	bool bSessionRollbacked = false;
	bool bSessionRequestToCancel = false;

	// Import Result
	int32 NumSuccessImportedDependencyAssets = 0;

	FString LastSkipMessage;
	FString LastFailedMessage;
};


////////////////////////////////////////////////////

void FExtAssetImporter::ImportAssets(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& ImportSetting)
{
	if (!ValidateImportSetting(ImportSetting))
	{
		return;
	}

#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
	if (ImportSetting.bImportToPluginFolder && ImportSetting.bWarnBeforeImportToPluginFolder && !ImportSetting.bSandboxMode && !ImportSetting.bFlatMode)
	{
		FUAssetImportSetting ImportToProjectSetting = ImportSetting;
		ImportToProjectSetting.TargetContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

		if (IsValidImportToPluginName(ImportSetting.ImportToPluginName))
		{
			EAppReturnType::Type YesNoCancelReply = FMessageDialog::Open(EAppMsgType::YesNoCancel,
				FText::Format(
					LOCTEXT("Prompt_ImportToPluginOrProjectContentFolder", "Would you like to import assets into plugin: {0} ?\n\n    Yes: Import to plugin's content folder\n    No: Import to project's content folder\n    Cancel: Abort import"),
					FText::FromName(ImportSetting.ImportToPluginName)));

			switch (YesNoCancelReply)
			{
			case EAppReturnType::Yes:
				DoImportAssets(InAssetDatas, ImportSetting);
				break;

			case EAppReturnType::No:
				DoImportAssets(InAssetDatas, ImportToProjectSetting);
				break;

			case EAppReturnType::Cancel:
				return;
			}
		}
#if ECB_TODO // todo: option to import to project when plugin is invalid
		else
		{
			EAppReturnType::Type YesNoReply = FMessageDialog::Open(EAppMsgType::YesNo,
				FText::Format(
					LOCTEXT("Prompt_ImportToPluginOrProjectContentFolder", "Plugin {0} is not a valid target, would you like to import to project's content folder instead?\n\nYes: Import to project's content folder\nNo: Abort import"),
					FText::FromName(ImportSetting.ImportToPluginName)));

			switch (YesNoReply)
			{
			case EAppReturnType::Yes:
				DoImportAssets(InAssetDatas, ImportToProjectSetting);
				break;

			case EAppReturnType::No:
				return;
			}
		}
#endif
		return;
	}
#endif

	DoImportAssets(InAssetDatas, ImportSetting);
}

void FExtAssetImporter::DoImportAssets(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& ImportSetting)
{
	int32 NumMainAssetsToImport = InAssetDatas.Num();
	ECB_LOG(Display, TEXT("========== Importing %d Assets ============"), NumMainAssetsToImport);
	if (NumMainAssetsToImport <= 0)
	{
		return;
	}

	FExtAssetImportSession Session(ImportSetting);
	Session.ImportAssets(InAssetDatas);
}

void FExtAssetImporter::ImportAssetsByFilePaths(const TArray<FString>& InAssetFilePaths, const FUAssetImportSetting& ImportSetting)
{
	if (!ValidateImportSetting(ImportSetting))
	{
		return;
	}

#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
	if (ImportSetting.bImportToPluginFolder && !IsValidImportToPluginName(ImportSetting.ImportToPluginName))
	{
		return;
	}
#endif

	TArray<FExtAssetData> AssetDatas;
	for (const FString& AssetFilePath : InAssetFilePaths)
	{
		FExtAssetData* AssetData = FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheAssetByFilePath(*AssetFilePath, /*bDelayParse */false);
		if (AssetData && AssetData->IsValid())
		{
			AssetDatas.Add(*AssetData);
		}
	}

	DoImportAssets(AssetDatas, ImportSetting);
}


void FExtAssetImporter::ImportAssetsToFolderPackagePath(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& InImportSetting, const FString& InDestPackagePath)
{
	UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
	FContentBrowserItemPath DropFolderItemPath(*InDestPackagePath, EContentBrowserPathType::Virtual); //Assume Virtual Path
	FString DestPackagePath = DropFolderItemPath.HasInternalPath()? *DropFolderItemPath.GetInternalPathString() : *DropFolderItemPath.GetVirtualPathString();
	ECB_LOG(Display, TEXT("[ImportAssetsToFolderPackagePath] DropFolderItem: DestPackagePath %s"), *DestPackagePath);

	FString FolderPath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(DestPackagePath));
	FPaths::NormalizeDirectoryName(FolderPath); // no ending slash

	FUAssetImportSetting ImportSetting = InImportSetting;
	ImportSetting.TargetContentDir = FolderPath;
	ECB_LOG(Display, TEXT("[ImportAssetsWithPathPicker] %s =>> %s"), *InDestPackagePath, *FolderPath);
	
	FExtAssetImporter::DoImportAssets(InAssetDatas, ImportSetting);
}

void FExtAssetImporter::ImportAssetsWithPathPicker(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& InImportSetting)
{
	// Prepare Content Root Dir with Dialog
	{
		TSharedPtr<SWindow> PickContentPathWidget;
		SAssignNew(PickContentPathWidget, SWindow)
			.Title(LOCTEXT("SelectPath", "Select Path"))
			.ToolTipText(LOCTEXT("SelectPathTooltip", "Select the path where the uasset files will be imported to"))
			.ClientSize(FVector2D(400, 400));

		TSharedPtr<SContentBrowserPathPicker> ContentBrowserPathPicker;
		PickContentPathWidget->SetContent
		(
			SAssignNew(ContentBrowserPathPicker, SContentBrowserPathPicker, PickContentPathWidget)
			.HeadingText(LOCTEXT("ContentBrowserPathPicker_Heading", "Folder Name"))
			.CreateButtonText(LOCTEXT("ContentBrowserPathPicker_ButtonLabel", "Import"))
			.OnCreateAssetAction
			(
				FOnPathChosen::CreateLambda([=](const FString& InDestPackagePath) {
					FExtAssetImporter::ImportAssetsToFolderPackagePath(InAssetDatas, InImportSetting, InDestPackagePath);
				})
			)
		);

		TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
		if (RootWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(PickContentPathWidget.ToSharedRef(), RootWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(PickContentPathWidget.ToSharedRef());
		}
	}
}

void FExtAssetImporter::ImportProjectFolderColors(const FString& InRootOrHost)
{
	FString AssetContentDir;
	if (FExtContentBrowserSingleton::GetAssetRegistry().IsRootFolder(InRootOrHost))
	{
		FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPathInfo(InRootOrHost, nullptr, nullptr, &AssetContentDir);
	}
	else if (const FName* AssetContentDirPtr = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetContentRootHostDirs().Find(*InRootOrHost))
	{
		AssetContentDir = AssetContentDirPtr->ToString();
	}
	
	if (AssetContentDir.Len() > 0)
	{
		if (const FName* ConfigDirName = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetContentRootConfigDirs().Find(*AssetContentDir))
		{
			TMap<FName, FLinearColor> PackagePathColors;
			if (FExtConfigUtil::LoadPathColors(AssetContentDir, ConfigDirName->ToString(), PackagePathColors, false))
			{
				ECB_LOG(Display, TEXT("Applying %d colored folders"), PackagePathColors.Num());

				for (auto& Pair : PackagePathColors)
				{
					const FString PackagePath = Pair.Key.ToString();
					const FLinearColor& FolderColor = Pair.Value;

					//if (!ImportedColoredFolders.Contains(Pair.Key)) Is Valid Package Path
					FString FolderPath;
					if (FPackageName::TryConvertLongPackageNameToFilename(PackagePath, FolderPath) && FPaths::DirectoryExists(FolderPath))
					{
						ECB_LOG(Display, TEXT("  Applying %s - %s"), *PackagePath, *FolderColor.ToString());

#if ECB_WIP_IMPORT_FOLDER_COLOR_OVERRIDE
						AssetViewUtils::SetPathColor(PackagePath, TOptional<FLinearColor>()); // clear PatchColors cache
						AssetViewUtils::SetPathColor(PackagePath, TOptional<FLinearColor>(FLinearColor(FolderColor)));
#else
						ExtContentBrowserUtils::SetPathColor(PackagePath, nullptr); // clear PatchColors cache
						ExtContentBrowserUtils::SetPathColor(PackagePath, MakeShareable(new FLinearColor(FolderColor)), true);
#endif
					}
					else
					{
						ECB_LOG(Display, TEXT("  Skipping %s"), *PackagePath);
					}
				}

			}
		}
	}
}

void FExtAssetImporter::ExportAssets(const TArray<FExtAssetData>& InAssetDatas)
{
	bool bFolderSelected = false;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		void* TopWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow().IsValid() ? FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle() : nullptr;

		FString DefaultPath;
		FString FolderName;
		bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			TopWindowHandle,
			/*Title=*/TEXT("Select a folder to export uassets"),
			DefaultPath,
			FolderName
		);

		if (bFolderSelected)
		{
			ECB_LOG(Display, TEXT("Select a folder to export uassets: %s"), *FolderName);

			const FUAssetImportSetting ExportSetting = FUAssetImportSetting::GetDefaultExportSetting(FolderName);
			
			int32 NumAssetsToExport = InAssetDatas.Num();
			ECB_LOG(Display, TEXT("========== Exporting %d Assets ============"), NumAssetsToExport);
			if (NumAssetsToExport <= 0)
			{
				return;
			}

			FExtAssetImportSession Session(ExportSetting);

// 			const int32 TotalSteps = 4;
// 			FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("GatheringDependencies", "Gathering  dependencies for {0} Assets..."), FText::AsNumber(NumAssetsToExport)));
// 			SlowTask.MakeDialog();

			ECB_LOG(Display, TEXT("------------ Phase 1: Gathering dependencies -----------------"));
			{
// 				FString FirstAssetFilePath = InAssetDatas[0].PackageFilePath.ToString();
// 				FString Progress = FString::Printf(TEXT("Gathering dependencies for %s%s...")
// 					, *FPaths::GetBaseFilename(FirstAssetFilePath)
// 					, (NumAssetsToExport ? TEXT("") : *FString::Printf(TEXT(" and other %d assets"), NumAssetsToExport - 1)));
// 				SlowTask.EnterProgressFrame(1, FText::FromString(Progress));

				Session.PrepareImportTasksWithDependencies(InAssetDatas);

				if (Session.IsSessionRequestToCancel())
				{
					FString FirstAssetFilePath = InAssetDatas[0].PackageFilePath.ToString();
					FString AbortMessage = FString::Printf(TEXT("Export %s%s aborted!")
						, *FPaths::GetBaseFilename(FirstAssetFilePath)
						, (NumAssetsToExport ? TEXT("") : *FString::Printf(TEXT(" and other %d assets"), NumAssetsToExport - 1)));

					ExtContentBrowserUtils::NotifyMessage(FText::FromString(AbortMessage));
					return;
				}
			}

			ECB_LOG(Display, TEXT("------------ Phase 2: Exporting asset files -----------------"));
			{
// 				SlowTask.EnterProgressFrame(1, LOCTEXT("ExportingAssets_ImportingFile", "Exporting files..."));
// 				Session.RunImportTasks();
				const int32 NumAllAsssetsToExport = Session.GetNumAllAssetsToImport();
				const float TotalSteps = NumAllAsssetsToExport + 1;

				TSharedPtr<FExtAssetCoreUtil::FExtAssetFeedbackContext> FeedbackContext(new FExtAssetCoreUtil::FExtAssetFeedbackContext);
				FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("ExportingAssets_ImportingFile", "Exporting {0} Assets..."), FText::AsNumber(NumAllAsssetsToExport))
					, /*bEnabled*/ true,
					*FeedbackContext.Get()
				);

				SlowTask.MakeDialog(/*bShowCancelDialog =*/ true);

				Session.RunImportTasks(SlowTask);
			}

			ECB_LOG(Display, TEXT("------------ Phase 3: Rollback and cleanup -----------------"));
			{
// 				SlowTask.EnterProgressFrame(1, LOCTEXT("ExportingAssets_Finishing", "Finishing export..."));
// 				Session.PostImport();
				FScopedSlowTask SlowTask(1, LOCTEXT("ExportingAssets_Finishing", "Finish Exporting..."));
				SlowTask.MakeDialog(/*bShowCancelDialog =*/ false);

				const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
				if (/*!Session.IfAllTasksFailed() && */ ExtContentBrowserSetting->bOpenFolderAfterExport)
				{
					FPlatformProcess::ExploreFolder(*FolderName);
				}
			}
		}
	}
}

void FExtAssetImporter::ZipUpSourceAssets(const TArray<FExtAssetData>& InAssetDatas)
{
	int32 NumAssetsToExport = InAssetDatas.Num();
	ECB_LOG(Display, TEXT("========== Exporting %d Assets ============"), NumAssetsToExport);
	if (NumAssetsToExport <= 0)
	{
		return;
	}

	bool bOpened = false;
	TArray<FString> SaveFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform != NULL)
	{
		FString DefaultFileName = FPaths::GetBaseFilename(InAssetDatas[0].PackageFilePath.ToString()) + TEXT(".zip");
		bOpened = DesktopPlatform->SaveFileDialog(
			NULL,
			LOCTEXT("ZipUpAssets", "Zip file location").ToString(),
			TEXT(""),
			DefaultFileName,
			TEXT("Zip file|*.zip"),
			EFileDialogFlags::None,
			SaveFilenames);
	}

	if (bOpened && SaveFilenames.Num() > 0)
	{
		FString SaveFileName = SaveFilenames[0];
		FString SaveFolder = FPaths::GetPath(SaveFileName);

		{
			ECB_LOG(Display, TEXT("Select a zip file to export uassets: %s"), *SaveFileName);

			FUAssetImportSetting ExportSetting = FUAssetImportSetting::GetZipExportSetting();
			ExportSetting.TargetContentDir = FExtAssetData::GetZipExportSessionTempDir();

			FExtAssetImportSession Session(ExportSetting);

			const int32 TotalSteps = 3;
			FScopedSlowTask SlowTask(TotalSteps, FText::Format(LOCTEXT("GatheringDependencies", "Gathering  dependencies for {0} Assets..."), FText::AsNumber(NumAssetsToExport)));
			SlowTask.MakeDialog();

			ECB_LOG(Display, TEXT("------------ Phase 1: Gathering dependencies -----------------"));
			{
				FString FirstAssetFilePath = InAssetDatas[0].PackageFilePath.ToString();
				FString Progress = FString::Printf(TEXT("Gathering dependencies for %s%s...")
					, *FPaths::GetBaseFilename(FirstAssetFilePath)
					, (NumAssetsToExport ? TEXT("") : *FString::Printf(TEXT(" and other %d assets"), NumAssetsToExport - 1)));
				SlowTask.EnterProgressFrame(1, FText::FromString(Progress));

				Session.PrepareImportTasksWithDependencies(InAssetDatas);
			}

			ECB_LOG(Display, TEXT("------------ Phase 2: Exporting asset files -----------------"));
			{
				SlowTask.EnterProgressFrame(1, LOCTEXT("ExportingAssets_ImportingFile", "Exporting files..."));
				Session.RunZipImportTasks();
			}

			ECB_LOG(Display, TEXT("------------ Phase 3: Ziping -----------------"));
			{
				SlowTask.EnterProgressFrame(1, LOCTEXT("ExportingAssets_ZipFile", "Archiving files..."));
				Session.ZipSourceFiles(SaveFileName);
			}

			ECB_LOG(Display, TEXT("------------ Phase 4: cleanup -----------------"));
			{
				SlowTask.EnterProgressFrame(1, LOCTEXT("ExportingAssets_Finishing", "Finishing export..."));
				Session.Cleanup();

				Session.NotifyResult();

				const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
				if (/*!Session.IfAllTasksFailed() && */ ExtContentBrowserSetting->bOpenFolderAfterExport)
				{
					FPlatformProcess::ExploreFolder(*SaveFolder);
				}
			}
		}
	}
}

bool FExtAssetImporter::IsValidImportToPluginName(const FName& InPluginName, FString* OutInValidReason, FString *OutValidPluginContentDir)
{
	if (InPluginName == NAME_None)
	{
		if (OutInValidReason)
		{
			*OutInValidReason = FString::Printf(TEXT("No plugin been specified for import!"), *InPluginName.ToString());
		}
		return false;
	}

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(InPluginName.ToString());
	if (!Plugin.IsValid())
	{
		if (OutInValidReason)
		{
			*OutInValidReason = FString::Printf(TEXT("Import to Plugin: %s not found!"), *InPluginName.ToString());
		}
		
		return false;
	}

	FString PluginContentDir = FPaths::ConvertRelativePathToFull(Plugin->GetContentDir());
	FString ProjectDIr = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	ECB_LOG(Display, TEXT("[FExtAssetImporter::IsValidImportToPluginName] PluginContentDir: %s, PrjectDir: %s"), *PluginContentDir, *ProjectDIr);
	if (PluginContentDir.StartsWith(ProjectDIr))
	{
		if (OutValidPluginContentDir)
		{
			*OutValidPluginContentDir = PluginContentDir;
		}
		return true;
	}
	else
	{
		if (OutInValidReason)
		{
			*OutInValidReason = FString::Printf(TEXT("Import to Plugin: %s should be inside project folder!"), *InPluginName.ToString());
		}
		return false;
	}

	return false;
}

bool FExtAssetImporter::ValidateImportSetting(const FUAssetImportSetting& ImportSetting)
{
	if (!ImportSetting.IsValid())
	{
		ExtContentBrowserUtils::NotifyMessage(ImportSetting.GetInvalidReason());
		return false;
	}

	if (ImportSetting.bImportToPluginFolder && ImportSetting.bFlatMode)
	{
		ExtContentBrowserUtils::NotifyMessage(LOCTEXT("", "Flat Import to Plugin is not supported!"));
		return false;
	}

	return true;
}

//--------------------------------------------------
// FExtAssetImportSetting Impl
//

FUAssetImportSetting::FUAssetImportSetting()
{
	TargetContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	FPaths::NormalizeDirectoryName(TargetContentDir);
}

FUAssetImportSetting::FUAssetImportSetting(const FString& InTargetContentDir)
	: TargetContentDir(InTargetContentDir)
{
}

FUAssetImportSetting FUAssetImportSetting::GetDefaultExportSetting(const FString& InTargetDir)
{
	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();

	FUAssetImportSetting ImportSetting;

	ImportSetting.bExportMode = true;

	ImportSetting.bSkipImportIfAnyDependencyMissing = ExtContentBrowserSetting->bSkipExportIfAnyDependencyMissing;
#if ECB_FEA_IMPORT_IGNORE_SOFT_ERROR
	ImportSetting.bIgnoreSoftReferencesError = ExtContentBrowserSetting->bExportIgnoreSoftReferencesError;
#endif
	ImportSetting.bOverwriteExistingFiles = ExtContentBrowserSetting->bExportOverwriteExistingFiles;
	ImportSetting.bRollbackIfFailed = ExtContentBrowserSetting->bRollbackExportIfFailed;

	ImportSetting.bSyncAssetsInContentBrowser = false;
	ImportSetting.bSyncExistingAssets = false;
	ImportSetting.bLoadAssetAfterImport = false;

	ImportSetting.TargetContentDir = InTargetDir;
	ImportSetting.bSandboxMode = false;
	ImportSetting.bFlatMode= false;

	return ImportSetting;
}

FUAssetImportSetting FUAssetImportSetting::GetZipExportSetting()
{
	FUAssetImportSetting ImportSetting;
	
	ImportSetting.bExportMode = true;

	ImportSetting.bSkipImportIfAnyDependencyMissing = false;
#if ECB_FEA_IMPORT_IGNORE_SOFT_ERROR
	ImportSetting.bIgnoreSoftReferencesError = true;
#endif
	ImportSetting.bOverwriteExistingFiles = true;
	ImportSetting.bRollbackIfFailed = false;
	ImportSetting.bSyncAssetsInContentBrowser = false;
	ImportSetting.bSyncExistingAssets = false;
	ImportSetting.bLoadAssetAfterImport = false;

	ImportSetting.bSandboxMode = false;
	ImportSetting.bFlatMode = false;

	return ImportSetting;
}

FUAssetImportSetting FUAssetImportSetting::GetSavedImportSetting()
{
	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();

	FUAssetImportSetting ImportSetting;
	ImportSetting.bSkipImportIfAnyDependencyMissing = ExtContentBrowserSetting->bSkipImportIfAnyDependencyMissing;
	ImportSetting.bOverwriteExistingFiles = ExtContentBrowserSetting->bImportOverwriteExistingFiles;
	ImportSetting.bRollbackIfFailed = ExtContentBrowserSetting->bRollbackImportIfFailed;
	ImportSetting.bSyncAssetsInContentBrowser = ExtContentBrowserSetting->bImportSyncAssetsInContentBrowser;
	ImportSetting.bSyncExistingAssets = ExtContentBrowserSetting->bImportSyncExistingAssets;
	ImportSetting.bLoadAssetAfterImport = ExtContentBrowserSetting->bLoadAssetAfterImport;
#if ECB_WIP_IMPORT_ADD_TO_COLLECTION
	ImportSetting.bAddImportedAssetsToCollection = ExtContentBrowserSetting->bAddImportedAssetsToCollection;
	ImportSetting.bUniqueCollectionNameForEachImportSession = ExtContentBrowserSetting->bUniqueCollectionNameForEachImportSession;
	ImportSetting.ImportedUAssetCollectionName = ExtContentBrowserSetting->DefaultImportedUAssetCollectionName;
#endif

#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
	ImportSetting.bImportToPluginFolder = ExtContentBrowserSetting->bImportToPluginFolder;
	ImportSetting.bWarnBeforeImportToPluginFolder = ExtContentBrowserSetting->bWarnBeforeImportToPluginFolder;
	ImportSetting.ImportToPluginName = ExtContentBrowserSetting->ImportToPluginName;
#endif

#if ECB_WIP_IMPORT_FOLDER_COLOR
	ImportSetting.bImportFolderColor = ExtContentBrowserSetting->bImportFolderColor;
#if ECB_WIP_IMPORT_FOLDER_COLOR_OVERRIDE
	ImportSetting.bOverrideExistingFolderColor = ExtContentBrowserSetting->bOverrideExistingFolderColor;
#else
	ImportSetting.bOverrideExistingFolderColor = false;
#endif
#endif

#if ECB_FEA_IMPORT_IGNORE_SOFT_ERROR
	ImportSetting.bIgnoreSoftReferencesError = ExtContentBrowserSetting->bImportIgnoreSoftReferencesError;
#endif

	ImportSetting.ResolveTargetContentDir();

	return ImportSetting;
}

FUAssetImportSetting FUAssetImportSetting::GetSandboxImportSetting()
{
	FUAssetImportSetting ImportSetting;
	{
		ImportSetting.TargetContentDir = FExtAssetData::GetSandboxDir();
		ImportSetting.bSandboxMode = true;
		ImportSetting.bFlatMode = false;

		ImportSetting.bSkipImportIfAnyDependencyMissing = false;
		ImportSetting.bOverwriteExistingFiles = true;

		ImportSetting.bRollbackIfFailed = /*true*/false; // no need to handle rollback for sandbox
		ImportSetting.bSyncAssetsInContentBrowser = true;
		ImportSetting.bSyncExistingAssets = true;
		ImportSetting.bLoadAssetAfterImport = true;
#if ECB_WIP_IMPORT_ADD_TO_COLLECTION
		ImportSetting.bAddImportedAssetsToCollection = false;
		ImportSetting.bUniqueCollectionNameForEachImportSession = false;
		ImportSetting.ImportedUAssetCollectionName = NAME_None;
#endif

#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
		ImportSetting.bImportToPluginFolder = false;
		ImportSetting.bWarnBeforeImportToPluginFolder = false;
		ImportSetting.ImportToPluginName = NAME_None;
#endif

#if ECB_WIP_IMPORT_FOLDER_COLOR
		ImportSetting.bImportFolderColor = false;
		ImportSetting.bOverrideExistingFolderColor = false;
#endif

#if ECB_FEA_IMPORT_IGNORE_SOFT_ERROR
		ImportSetting.bIgnoreSoftReferencesError = true;
#endif
	}

	return ImportSetting;
}

FUAssetImportSetting FUAssetImportSetting::GetSandboxImportForDumpSetting()
{
	FUAssetImportSetting ImportSetting = GetSandboxImportSetting();
	{
		ImportSetting.bDumpAsset = true;

		ImportSetting.bFlatMode = true;
		ImportSetting.bSyncAssetsInContentBrowser = false;
		ImportSetting.bSyncExistingAssets = false;

		{
			const FString SessionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
			ImportSetting.TargetContentDir = FPaths::Combine(FExtAssetData::GetSandboxDir(), SessionId);
		}
	}
	return ImportSetting;
}

FUAssetImportSetting FUAssetImportSetting::GetFlatModeImportSetting()
{
	FUAssetImportSetting ImportSetting = GetSavedImportSetting();
	{
		ImportSetting.bFlatMode = true;

		ImportSetting.bLoadAssetAfterImport = true;
	}

	return ImportSetting;
}

FUAssetImportSetting FUAssetImportSetting::GetDirectCopyModeImportSetting()
{
	FUAssetImportSetting ImportSetting = GetSavedImportSetting();
	{
		ImportSetting.bDirectCopyMode = true;
		ImportSetting.bWarnBeforeImportToPluginFolder = false;
	}

	return ImportSetting;
}

void FUAssetImportSetting::ResolveTargetContentDir()
{
#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
	if (!bSandboxMode && bImportToPluginFolder)
	{
		FString ValidPluginContentDir;
		InValidReason.Empty();
		bValid = FExtAssetImporter::IsValidImportToPluginName(ImportToPluginName, &InValidReason, &ValidPluginContentDir);

		if (bValid)
		{
			TargetContentDir = ValidPluginContentDir;
		}
	}
#endif
}

#if 0
//--------------------------------------------------
// FUAssetBatchImportSetting Impl
//
FUAssetBatchImportSetting::FUAssetBatchImportSetting()
	: FUAssetImportSetting()
{
	bWarnBeforeImportToPluginFolder = false;

	bSyncAssetsInContentBrowser = false;
	bSyncExistingAssets = false;
	bLoadAssetAfterImport = false;

	bUniqueCollectionNameForEachImportSession = false;
}


FUAssetBatchImportSetting::FUAssetBatchImportSetting(const FUAssetImportSetting& BaseSetting)
	: FUAssetImportSetting(BaseSetting)
{
}
#endif


//--------------------------------------------------
// FExtAssetRegistryStateCache Impl
//

void FExtAssetRegistryStateCache::Reset()
{
	CachedAssetsByFilePath.Empty();
	CachedUnParsedAssets.Empty();
	CachedDependencyInfoByFilePath.Empty();

	CachedAssetsByFolder.Empty();
	CachedAssetsByClass.Empty();
	CachedAssetsByTag.Empty();
	CachedFilePathsByFolder.Empty();
	CachedSubPaths.Empty();
	CachedEmptyFoldersStatus.Empty();
	CachedFullyParsedFolders.Empty();

	CachedPackageNameToFilePathMap.Empty();
	CachedAssetContentRoots.Empty();
	CachedAssetContentType.Empty();
	CachedAssetContentRootConfigDirs.Empty();
	CachedAssetContentRootHosts.Empty();
	CachedFolderColorIndices.Empty();
	CachedFolderColors.Empty();

	CachedThumbnails.Empty();
}

void FExtAssetRegistryStateCache::ClearThumbnailCache()
{
	CachedThumbnails.Empty();
}

void FExtAssetRegistryStateCache::CacheNewAsset(FExtAssetData* InAssetToCache)
{
	// Add to CachedAssetsByFolder
	{
		const FString Dir = InAssetToCache->GetFolderPath();
		TArray<FExtAssetData*>& CachedAssets = CachedAssetsByFolder.FindOrAdd(*Dir);
		CachedAssets.AddUnique(InAssetToCache);
	}

	// Add asset to CachedAssetsByFilePath
	CachedAssetsByFilePath.Add(InAssetToCache->PackageFilePath, InAssetToCache);

	// PackageName to file path map
	if (InAssetToCache->PackageName != NAME_None)
	{
		CachedPackageNameToFilePathMap.FindOrAdd(InAssetToCache->PackageName) = InAssetToCache->PackageFilePath;
	}

	if (InAssetToCache->IsParsed())
	{
		CacheValidAssetInfo(InAssetToCache);
	}
	else
	{
		CachedUnParsedAssets.Add(InAssetToCache->PackageFilePath, InAssetToCache);
	}
}

void FExtAssetRegistryStateCache::ReCacheParsedAsset(FExtAssetData* InAssetToCache)
{
	if (!InAssetToCache->IsParsed())
	{
		return;
	}

	for (auto It = CachedUnParsedAssets.CreateIterator(); It; ++It)
	{
		if (It->Key == InAssetToCache->PackageFilePath)
		{
			CacheValidAssetInfo(It->Value);
		}
		It.RemoveCurrent();
	}
}

void FExtAssetRegistryStateCache::UpdateCacheByRemovedFolders(const TArray<FString>& InRemovedFolders)
{
	ECB_LOG(Display, TEXT("Before UpdateCache:"));
	PrintStatus();

	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();

	for (const FString& InRemovedFolder : InRemovedFolders)
	{
		FName FolderName(*InRemovedFolder);

		TSet<FName> AllFoldersToRemove;
		TSet<FName> AllFilesToRemove;

		AllFoldersToRemove.Add(FolderName);

		{
			TSet<FName> AllSubPaths;
			FContentPathUtil::ExpandCachedSubsRecursively(CachedSubPaths, *InRemovedFolder, AllSubPaths);
			AllFoldersToRemove.Append(AllSubPaths);
		}
	
		// exact match
		{
			for (const FName& FolderPath : AllFoldersToRemove)
			{
				// CachedSubPaths, CachedEmptyFoldersStatus
				CachedSubPaths.Remove(FolderPath);
				CachedEmptyFoldersStatus.Remove(FolderPath);
				CachedFullyParsedFolders.Remove(FolderPath);

				// CachedAssetsByFolder
				if (!ExtContentBrowserSetting->bKeepCachedAssetsWhenRootRemoved)
					CachedAssetsByFolder.Remove(FolderPath);

				// CachedFilePathsByFolder
				if (auto* FilePathsPtr = CachedFilePathsByFolder.Find(FolderPath))
				{
					AllFilesToRemove.Append(*FilePathsPtr);
				}
				CachedFilePathsByFolder.Remove(FolderPath);
			}
		}


		// CachedPackageNameToFilePathMap
		for (auto It = CachedPackageNameToFilePathMap.CreateIterator(); It; ++It)
		{
			if (AllFilesToRemove.Contains(It->Value))
			{
				It.RemoveCurrent();
			}
		}

		// CachedAssetContentRootDirs, CachedAssetContentType
		{
			for (int32 Index = CachedAssetContentRoots.Num() - 1; Index >= 0; --Index)
			{
				FName& CachedContentRoot = CachedAssetContentRoots[Index];
				//if (CachedAssetContentRootDirs[Index].ToString().StartsWith(InRemovedFolder))
				if (FPathsUtil::IsSubOrSamePath(CachedContentRoot.ToString(), InRemovedFolder))
				{
					CachedAssetContentType.Remove(CachedContentRoot);
					CachedAssetContentRootConfigDirs.Remove(CachedContentRoot);
					if (const FName* HostDirPtr = CachedAssetContentRootHosts.FindKey(CachedContentRoot))
					{
						CachedAssetContentRootHosts.Remove(*HostDirPtr);
					}
					if (auto* Folders = CachedFolderColorIndices.Find(CachedContentRoot))
					{
						for (auto& Folder : *Folders)
						{
							CachedFolderColors.Remove(Folder);
						}
					}
					CachedFolderColorIndices.Remove(CachedContentRoot);

					CachedAssetContentRoots.RemoveAt(Index);
				}
			}
		}

		if (ExtContentBrowserSetting->bKeepCachedAssetsWhenRootRemoved)
			continue;

		// CachedAssetsByFilePath, CachedDependencyInfoByFilePath, CachedUnParsedAssets, CachedThumbnails
		for (auto& FilePath : AllFilesToRemove)
		{
			CachedAssetsByFilePath.Remove(FilePath);
			CachedUnParsedAssets.Remove(FilePath);
			CachedDependencyInfoByFilePath.Remove(FilePath);
#if ECB_WIP_THUMB_CACHE
			CachedThumbnails.Remove(FilePath);
#endif
		}

		// CachedAssetsByClass
		for (auto It = CachedAssetsByClass.CreateIterator(); It; ++It)
		{
			TArray<FExtAssetData*>& Assets = It->Value;
			int32 NumAssets = It->Value.Num();

			for (int32 Index = NumAssets - 1; Index >= 0; --Index)
			{
				if (!Assets[Index] || AllFilesToRemove.Contains(Assets[Index]->PackageFilePath))
				{
					Assets.RemoveAt(Index);
				}
			}
			if (Assets.Num() == 0)
			{
				It.RemoveCurrent();
			}
		}

		// CachedAssetsByTag
		for (auto It = CachedAssetsByTag.CreateIterator(); It; ++It)
		{
			TArray<FExtAssetData*>& Assets = It->Value;
			int32 NumAssets = It->Value.Num();
			for (int32 Index = NumAssets - 1; Index >= 0; --Index)
			{
				if (!Assets[Index] || AllFilesToRemove.Contains(Assets[Index]->PackageFilePath))
				{
					Assets.RemoveAt(Index);
				}
			}
			if (Assets.Num() == 0)
			{
				It.RemoveCurrent();
			}
		}


		ECB_LOG(Display, TEXT("%d folder, %d files removed from cache of %s"), AllFoldersToRemove.Num(), AllFilesToRemove.Num(), *InRemovedFolder);
	}

	for (const FString& InRemovedFolder : InRemovedFolders)
	{
		for (auto It = CachedSubPaths.CreateIterator(); It; ++It)
		{
			const FName ParentPath = It->Key;
			//if (ParentPath.ToString().StartsWith(InRemovedFolder))
			if (FPathsUtil::IsSubOrSamePath(ParentPath.ToString(), InRemovedFolder))
			{
				It.RemoveCurrent();
			}
		}
	}

	//CollectGarbage(RF_NoFlags);

	ECB_LOG(Display, TEXT("After UpdateCache:"));
	PrintStatus();
}

bool FExtAssetRegistryStateCache::PurgeAssets(bool bSilent/* = true*/)
{
	int32 NumFoldersPurged = 0;
	int32 NumAssetsPurged = 0;

	ECB_LOG(Display, TEXT("Before PurgeAssets:"));
	PrintStatus();

	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();

	TArray<FString> RootContentPaths;
	FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPaths(RootContentPaths);

	TSet<FName> AllFoldersToRemove;
	for (auto It = CachedSubPaths.CreateIterator(); It; ++It)
	{
		const FName ParentPath = It->Key;
		bool bFoundInRoot = false;
		for (auto& RootPath : RootContentPaths)
		{
			if (FPathsUtil::IsSubOrSamePath(ParentPath.ToString(), RootPath))
			{
				bFoundInRoot = true;
				break;
			}
		}

		if (bFoundInRoot)
		{
			continue;
		}

		AllFoldersToRemove.Add(It->Key);
		//AllFoldersToRemove.Append(It->Value);

		TSet<FName>& ToRemoves = It->Value;
		for (auto& ToRemove : ToRemoves)
		{
			TSet<FName> AllSubPaths;
			FContentPathUtil::ExpandCachedSubsRecursively(CachedSubPaths, *ToRemove.ToString(), AllSubPaths);
			AllFoldersToRemove.Append(AllSubPaths);
		}
	}

	for (const FName& FolderPath : AllFoldersToRemove)
	{
		// CachedSubPaths, CachedEmptyFoldersStatus, CachedFullyParsedFolders, CachedFilePathsByFolder
		CachedSubPaths.Remove(FolderPath);
		CachedEmptyFoldersStatus.Remove(FolderPath);
		CachedFullyParsedFolders.Remove(FolderPath);
		CachedFilePathsByFolder.Remove(FolderPath);
	}

	ECB_LOG(Display, TEXT("%d folders removed from cache"), AllFoldersToRemove.Num());
	
	TSet<FName> AllFilesToRemove;
	// CachedAssetsByFolder
	for (auto It = CachedAssetsByFolder.CreateIterator(); It; ++It)
	{
		const FName ParentPath = It->Key;
		bool bFoundInRoot = false;
		for (auto& RootPath : RootContentPaths)
		{
			if (FPathsUtil::IsSubOrSamePath(ParentPath.ToString(), RootPath))
			{
				bFoundInRoot = true;
				break;
			}
		}

		if (bFoundInRoot)
		{
			continue;
		}

		for (auto& AsssetData : It->Value)
		{
			AllFilesToRemove.Add(AsssetData->PackageFilePath);
		}
		It.RemoveCurrent();
	}
	
	
	{
		// CachedAssetsByFilePath, CachedDependencyInfoByFilePath, CachedUnParsedAssets, CachedThumbnails, CachedPackageNameToFilePathMap
		for (auto& FilePath : AllFilesToRemove)
		{
			CachedAssetsByFilePath.Remove(FilePath);
			CachedUnParsedAssets.Remove(FilePath);
			CachedDependencyInfoByFilePath.Remove(FilePath);
#if ECB_WIP_THUMB_CACHE
			CachedThumbnails.Remove(FilePath);
#endif
			// CachedPackageNameToFilePathMap
			for (auto It = CachedPackageNameToFilePathMap.CreateIterator(); It; ++It)
			{
				if (AllFilesToRemove.Contains(It->Value))
				{
					It.RemoveCurrent();
				}
			}
		}

		// CachedAssetsByClass
		for (auto It = CachedAssetsByClass.CreateIterator(); It; ++It)
		{
			TArray<FExtAssetData*>& Assets = It->Value;
			int32 NumAssets = It->Value.Num();

			for (int32 Index = NumAssets - 1; Index >= 0; --Index)
			{
				if (!Assets[Index] || AllFilesToRemove.Contains(Assets[Index]->PackageFilePath))
				{
					Assets.RemoveAt(Index);
				}
			}
			if (Assets.Num() == 0)
			{
				It.RemoveCurrent();
			}
		}

		// CachedAssetsByTag
		for (auto It = CachedAssetsByTag.CreateIterator(); It; ++It)
		{
			TArray<FExtAssetData*>& Assets = It->Value;
			int32 NumAssets = It->Value.Num();
			for (int32 Index = NumAssets - 1; Index >= 0; --Index)
			{
				if (!Assets[Index] || AllFilesToRemove.Contains(Assets[Index]->PackageFilePath))
				{
					Assets.RemoveAt(Index);
				}
			}
			if (Assets.Num() == 0)
			{
				It.RemoveCurrent();
			}
		}

		ECB_LOG(Display, TEXT("%d files removed from cache"), AllFilesToRemove.Num());
	}

	//CollectGarbage(RF_NoFlags);

	ECB_LOG(Display, TEXT("After UpdateCache:"));
	PrintStatus();

	NumFoldersPurged = AllFoldersToRemove.Num();
	NumAssetsPurged = AllFilesToRemove.Num();

	if (!bSilent)
	{
		FString Message = FString::Printf(TEXT("%d folders and %d assets data purged from cache."), NumFoldersPurged, NumAssetsPurged);
		ExtContentBrowserUtils::NotifyMessage(Message);
	}

	return NumFoldersPurged > 0 || NumAssetsPurged > 0;
}

void FExtAssetRegistryStateCache::PrintStatus() const
{
	ECB_LOG(Display, TEXT("  FExtAssetRegistryStateCache"));
	ECB_LOG(Display, TEXT("  ================================="));
	ECB_LOG(Display, TEXT("  CachedAssetsByFilePath:           %d"), CachedAssetsByFilePath.Num());
	ECB_LOG(Display, TEXT("  CachedUnParsedAssets:             %d"), CachedUnParsedAssets.Num());
	ECB_LOG(Display, TEXT("  CachedDependencyInfoByFilePath:   %d"), CachedDependencyInfoByFilePath.Num());
	ECB_LOG(Display, TEXT("  CachedAssetsByFolder:             %d"), CachedAssetsByFolder.Num());
	ECB_LOG(Display, TEXT("  CachedAssetsByClass:              %d"), CachedAssetsByClass.Num());
	ECB_LOG(Display, TEXT("  CachedAssetsByTag:                %d"), CachedAssetsByTag.Num());

	ECB_LOG(Display, TEXT("  CachedRootContentPaths:           %d"), CachedRootContentPaths.Num());

	ECB_LOG(Display, TEXT("  CachedSubPaths:                   %d"), CachedSubPaths.Num());
	ECB_LOG(Display, TEXT("  CachedEmptyFolders:               %d"), CachedEmptyFoldersStatus.Num());
	ECB_LOG(Display, TEXT("  CachedFullyParsedFolders:         %d"), CachedFullyParsedFolders.Num());
	ECB_LOG(Display, TEXT("  CachedPackageNameToFilePathMap:   %d"), CachedPackageNameToFilePathMap.Num());
	ECB_LOG(Display, TEXT("  CachedAssetContentRoots:		   %d"), CachedAssetContentRoots.Num());
	ECB_LOG(Display, TEXT("  CachedAssetContentType:           %d"), CachedAssetContentType.Num());
	ECB_LOG(Display, TEXT("  CachedAssetContentRootConfigDir:  %d"), CachedAssetContentRootConfigDirs.Num());
	ECB_LOG(Display, TEXT("  CachedAssetContentRootHostDirs:   %d"), CachedAssetContentRootHosts.Num());
	ECB_LOG(Display, TEXT("  CachedFolderColors:               %d"), CachedFolderColors.Num());
	ECB_LOG(Display, TEXT("  CachedFolderColorIndices:         %d"), CachedFolderColorIndices.Num());
#if ECB_WIP_THUMB_CACHE
	ECB_LOG(Display, TEXT("  CachedThumbnails:                 %d"), CachedThumbnails.Num());
#endif
}

void FExtAssetRegistryStateCache::CacheValidAssetInfo(FExtAssetData* InAssetToCache)
{
	if (!InAssetToCache || !InAssetToCache->IsValid())
	{
		return;
	}

	// Add asset to CachedAssetsByClass
	TArray<FExtAssetData*>& ClassAssets = CachedAssetsByClass.FindOrAdd(InAssetToCache->AssetClass);
	ClassAssets.Add(InAssetToCache);

	// Add asset to CachedAssetsByTag
	for (auto TagIt = InAssetToCache->TagsAndValues.CreateConstIterator(); TagIt; ++TagIt)
	{
		FName Key = TagIt.Key();

		TArray<FExtAssetData*>& TagAssets = CachedAssetsByTag.FindOrAdd(Key);
		TagAssets.Add(InAssetToCache);
	}

#if ECB_WIP_THUMB_CACHE
	if (!CachedThumbnails.Contains(InAssetToCache->PackageFilePath))
	{
		FObjectThumbnail Thumb;
		if (InAssetToCache->LoadThumbnail(Thumb))
		{
			CachedThumbnails.FindOrAdd(InAssetToCache->PackageFilePath) = MoveTemp(Thumb);
		}
	}	
#endif
}

////////////////////////////////////
// FExtFolderGatherer Impl
//

FExtFolderGatherer::FExtFolderGatherer(const FString& InGatheringFolder)
	: GatherTime(0.0)
	, Thread(nullptr)
	, bRequestToStop(false)
	, bRequestToSkipCurrentSearchFolder(false)
{
	CurrentSearchFolder = TEXT("");
	DirectoriesToSearch.Add(InGatheringFolder);
	Thread = FRunnableThread::Create(this, TEXT("FExtFolderGatherer"), 0, TPri_BelowNormal);
	checkf(Thread, TEXT("Failed to create folder gatherer thread"));
}

FExtFolderGatherer::FExtFolderGatherer(const FString& InParentFolder, const FExtFolderGatherPolicy& InGatherPolicy)
	: FExtFolderGatherer(InParentFolder)
{
	GatherPolicy = InGatherPolicy;
}

FExtFolderGatherer::~FExtFolderGatherer()
{
}

bool FExtFolderGatherer::Init()
{
	return true;
}

uint32 FExtFolderGatherer::Run()
{
	while (!bRequestToStop)
	{
		TArray<FString> LocalDirectoriesToSearch;		
		{
			FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
			LocalDirectoriesToSearch = DirectoriesToSearch;
		}

		if (LocalDirectoriesToSearch.Num() > 0)
		{
			double GatherStartTime = FPlatformTime::Seconds();
			//for (const FString& LocalDir : LocalDirectoriesToSearch)
			const FString& LocalDir = LocalDirectoriesToSearch.Last();
			{
				CurrentSearchFolder = LocalDir;
				bRequestToSkipCurrentSearchFolder = false;

				TArray<FName> SubDirs;
				TMap<FName, TArray<FName>> RecursiveSubDirs;
				TMap<FName, TArray<FName>> Pakages;
				const bool bSuccess = GetAllDirectoriesAndPackagesRecursively(*LocalDir, SubDirs, RecursiveSubDirs, Pakages);

				//if (bSuccess)
				{
					FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

					DirectoriesToSearch.Remove(LocalDir);
					RootDirGatherResults.Emplace(*LocalDir, *LocalDir, SubDirs, RecursiveSubDirs, Pakages);
					GatherTime = FPlatformTime::Seconds() - GatherStartTime;
				}
			}
		}
		CurrentSearchFolder = TEXT("");

		// No work to do. Sleep for a little and try again later.
		FPlatformProcess::Sleep(0.1);
	}

	return 0;
}

void FExtFolderGatherer::Stop()
{
	bRequestToStop = true;
}

void FExtFolderGatherer::Exit()
{
}

void FExtFolderGatherer::EnsureCompletion()
{
	{
		FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
		DirectoriesToSearch.Empty();
	}

	Stop();

	if (Thread != nullptr)
	{
		//Thread->WaitForCompletion();
		Thread->Kill(true);
		delete Thread;
		Thread = nullptr;
	}
}

void FExtFolderGatherer::AddSearchFolder(const FString& InFolderToSearch, const FExtFolderGatherPolicy* InUpdatedGatherPolicy)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	if (!DirectoriesToSearch.Contains(InFolderToSearch))
	{
		DirectoriesToSearch.Add(InFolderToSearch);
	}

	if (InUpdatedGatherPolicy != NULL)
	{
		GatherPolicy.UpdateWith(*InUpdatedGatherPolicy);
	}
}

void FExtFolderGatherer::StopSearchFolder(const FString& InFolderToStopSearch)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	if (DirectoriesToSearch.Contains(InFolderToStopSearch))
	{
		DirectoriesToSearch.Remove(InFolderToStopSearch);
	}

	if (CurrentSearchFolder.Equals(InFolderToStopSearch))
	{
		bRequestToSkipCurrentSearchFolder = true;
	}
}

bool FExtFolderGatherer::GetAndTrimGatherResult(TArray<FExtFolderGatherResult>& OutGatherResults, double& OutGatherTime, TArray<FExtFolderGatherResult>& OutSubDirGatherResults, TArray<FExtAssetContentRootGatherResult>& OutAssetContentRootGatherResults)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	bool bMainGathered = RootDirGatherResults.Num() > 0;
	if (bMainGathered)
	{
		OutGatherResults.Append(MoveTemp(RootDirGatherResults));
		RootDirGatherResults.Reset();

		OutGatherTime = GatherTime;
		GatherTime = 0.0;
	}	
	else
	{
		if (bool bSubDirGathered = SubDirGatherResults.Num() > 0)
		{
			OutSubDirGatherResults.Append(MoveTemp(SubDirGatherResults));
			SubDirGatherResults.Reset();
		}
		if (bool bAssetContentRootGathered = AssetContentRootGatherResults.Num() > 0)
		{
			OutAssetContentRootGatherResults.Append(MoveTemp(AssetContentRootGatherResults));
			AssetContentRootGatherResults.Reset();
		}
	}

	return bMainGathered;
}

void FExtFolderGatherer::GetDirectoriesAndPackages(const FName& InBaseDirectory, TArray<FName>& OutDirList, TArray<FName>& OutFilePaths)
{
	if (IFileManager::Get().DirectoryExists(*InBaseDirectory.ToString()))
	{
		FExtFolderGatherPolicy::FFolderVisitor& FolderVisitor = GatherPolicy.GetResetedFolderVisitor();
		IFileManager::Get().IterateDirectory(*InBaseDirectory.ToString(), FolderVisitor);

		OutDirList = FolderVisitor.Directories;
		OutFilePaths = FolderVisitor.PackageFiles;
	}
}

bool FExtFolderGatherer::GetAllDirectoriesAndPackagesRecursively(const FName& InBaseDirectory, TArray<FName>& OutSubPaths, TMap<FName, TArray<FName>>& OutRecusriveSubPaths, TMap<FName, TArray<FName>>& OutFilePaths)
{
	double GetAllStartTime = FPlatformTime::Seconds();

	if (IFileManager::Get().DirectoryExists(*InBaseDirectory.ToString()))
	{
		TMap<FName, FPathTree> PathTrees;
		GetChildDirectoriesAndPackages(InBaseDirectory, /*Parent=*/ NAME_None, { InBaseDirectory }, OutSubPaths, OutRecusriveSubPaths, OutFilePaths, PathTrees);
	}

	FString SkipOrCompleted = bRequestToSkipCurrentSearchFolder ? TEXT("skipped") : TEXT("completed");
	ECB_LOG(Display, TEXT("GetAllDirectoriesAndPackagesRecursively %s in %0.4f seconds"), *SkipOrCompleted, FPlatformTime::Seconds() - GetAllStartTime);

	return !bRequestToSkipCurrentSearchFolder;
}

void FExtFolderGatherer::GetChildDirectoriesAndPackages(const FName& InRoot, const FName& InParent, const TArray<FName>& InBaseDirs, TArray<FName>& OutSubPaths, TMap<FName, TArray<FName>>& OutRecusriveSubPaths, TMap<FName, TArray<FName>>& OutputFilePaths, TMap<FName, FPathTree>& OutputPathTrees, bool bFoundAssetContentRoot)
{
	if (bRequestToStop)
	{
		return;
	}

	if (bRequestToSkipCurrentSearchFolder)
	{
		return;
	}

	const bool bIsRoot = InParent == NAME_None;

	for (const auto& BaseDir : InBaseDirs)
	{
		if (bRequestToSkipCurrentSearchFolder)
		{
			return;
		}

		TArray<FName> ChildDirs;
		TArray<FName> ChildPackages;
		GetDirectoriesAndPackages(BaseDir, ChildDirs, ChildPackages);

		const bool bSortAlphabetical = true;
		if (bSortAlphabetical)
		{
			ChildDirs.Sort([](const FName& InA, const FName& InB) { return InA.LexicalLess(InB); });
		}

		if (bIsRoot)
		{
			OutSubPaths = ChildDirs;
		}

#if ECB_WIP // Gather Empty Folder 
		{
			bool bPotentialEmptyFolder = ChildPackages.Num() == 0;
			FPathTree BasePath(InParent, BaseDir, bPotentialEmptyFolder);
			if (!bPotentialEmptyFolder)
			{
				BasePath.MarkPathTreeNotEmpty(OutputPathTrees);
			}
			OutputPathTrees.Add(BaseDir, BasePath);
		}
#endif

		if (!bFoundAssetContentRoot)
		{
			FString AssetContentRoot = BaseDir.ToString();
			if (AssetContentRoot.EndsWith(TEXT("/Content")))
			{
				FExtAssetData::EContentType ContentType = FExtAssetData::EContentType::Unknown;

				FString VaultCacheHost = AssetContentRoot / TEXT("../..");
				FPaths::CollapseRelativeDirectories(VaultCacheHost);
				
				FString PluginOrProjectHost = AssetContentRoot / TEXT("..");
				FPaths::CollapseRelativeDirectories(PluginOrProjectHost);

				FString AssetContentRootHost = PluginOrProjectHost;

				if (FExtContentDirFinder::FindWithFolder(VaultCacheHost, TEXT("manifest"), /*bExtension*/ false))
				{
					ContentType = FExtAssetData::EContentType::VaultCache;
					AssetContentRootHost = VaultCacheHost;
					bFoundAssetContentRoot = true;
				}
				else if (FExtContentDirFinder::FindWithFolder(PluginOrProjectHost, TEXT(".uplugin"), /*bExtension*/ true))
				{
					ContentType = FExtAssetData::EContentType::Plugin;
					bFoundAssetContentRoot = true;
				}
				else if (FExtContentDirFinder::FindWithFolder(PluginOrProjectHost, TEXT(".uproject"), /*bExtension*/ true))
				{
					ContentType = FExtAssetData::EContentType::Project;
					bFoundAssetContentRoot = true;
				}

				if (bFoundAssetContentRoot)
				{
					FString ContentRootConfigDir = FExtConfigUtil::GetConfigDirByContentRoot(AssetContentRoot);

					FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
					AssetContentRootGatherResults.Emplace(/*AssetContentRootDir*/BaseDir, /*AssetContentRootParentDir*/ *AssetContentRootHost, /*AssetContentRootConfigDir*/*ContentRootConfigDir, /*AssetContentType*/ContentType);
				}
			}
		}

		//TArray<FName> RecursiveChildDirs;
		OutRecusriveSubPaths.FindOrAdd(BaseDir).Append(ChildDirs);
		//OutputPathList.Append(ChildDirs);
		OutputFilePaths.FindOrAdd(BaseDir) = ChildPackages;
		GetChildDirectoriesAndPackages(InRoot, /*InParent*/ BaseDir, /*InBaseDirs*/ChildDirs, OutSubPaths, /*OutputPathList*/OutRecusriveSubPaths, OutputFilePaths, OutputPathTrees, bFoundAssetContentRoot);

		//OutRecusriveSubPaths.Append(RecursiveChildDirs);

		if (!bIsRoot && ChildDirs.Num() > 0) // root already covered by main gather result
		{
			TMap<FName, TArray<FName>> MyFilePaths;
			MyFilePaths.Add(BaseDir, OutputFilePaths[BaseDir]);

			FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
			SubDirGatherResults.Emplace(InRoot, BaseDir, ChildDirs, OutRecusriveSubPaths, MyFilePaths);
		}
	}
}

////////////////////////////////////
// FExtAssetGatherer Impl
//

void FExtAssetGatherResult::AddReuslt(const FName& InFilePath, const FExtAssetData& InParsedAsset)
{
	if (InParsedAsset.IsParsed())
	{
		ParsedAssets.Emplace(InFilePath, InParsedAsset);
	}
}

void FExtAssetGatherResult::Reset()
{
	ParsedAssets.Reset();
}

bool FExtAssetGatherResult::HasResult() const
{
	return ParsedAssets.Num() > 0;
}

FExtAssetGatherer::FExtAssetGatherer()
	:GatherStartTime(0.0)
	, GatherTime(0.0)
	, Thread(nullptr)
	, bRequestToStop(false)
{
	Thread = FRunnableThread::Create(this, TEXT("FExtAssetGatherer"), 0, TPri_BelowNormal);
	checkf(Thread, TEXT("Failed to create asset gatherer thread"));
}

FExtAssetGatherer::~FExtAssetGatherer()
{
}

bool FExtAssetGatherer::Init()
{
	return true;
}

uint32 FExtAssetGatherer::Run()
{
	while (!bRequestToStop)
	{
		int32 NumLeftToGather = 0;
		FName LocalFilePathToGather;
		{
			FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
			NumLeftToGather = GatherBatch.Num();
			if (NumLeftToGather > 0)
			{
				LocalFilePathToGather = GatherBatch.Pop(/*bAllowShrinking*/ false);
			}
		}

		if (NumLeftToGather > 0)
		{
			FExtAssetData LocalAssetDataToParse;
			if (DoGatherAssetByFilePath(LocalFilePathToGather, LocalAssetDataToParse))
			{
				FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

				GatherResult.AddReuslt( LocalFilePathToGather, LocalAssetDataToParse);
			}
		}

		NumLeftToGather = GatherBatch.Num();

		if (NumLeftToGather == 0)
		{
			GatherTime = FPlatformTime::Seconds() - GatherStartTime;

			// No work to do. Sleep for a little and try again later.
			FPlatformProcess::Sleep(0.1);
		}
	}

	return 0;
}

void FExtAssetGatherer::Stop()
{
	bRequestToStop = true;
}

void FExtAssetGatherer::Exit()
{
}

void FExtAssetGatherer::EnsureCompletion()
{
	{
		FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
		GatherBatch.Empty();
	}

	Stop();

	if (Thread != nullptr)
	{
		//Thread->WaitForCompletion();
		Thread->Kill(true);
		delete Thread;
		Thread = nullptr;
	}
}

void FExtAssetGatherer::AddBatchToGather(const TArray<FName>& InBatch)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	GatherBatch = InBatch;

	GatherResult.Reset();

	GatherStartTime = FPlatformTime::Seconds();
}

void FExtAssetGatherer::CancelGather()
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	GatherBatch.Empty();
}

bool FExtAssetGatherer::GetAndTrimGatherResult(FExtAssetGatherResult& OutGatherResult, double& OutGatherTime, int32& OutLeft)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	bool bHasAnyGathered = GatherResult.HasResult();
	if (bHasAnyGathered)
	{
		OutGatherResult = GatherResult;
		GatherResult.Reset();
	}

	OutLeft = GatherBatch.Num();

	bool bBatchDone = OutLeft == 0;;

	if (bBatchDone)
	{
		OutGatherTime = GatherTime;
		GatherTime = 0.0;
	}

	return bHasAnyGathered;
}


bool FExtAssetGatherer::DoGatherAssetByFilePath(const FName& InFilePath, FExtAssetData& OutExtAssetData) const
{
	OutExtAssetData = FExtAssetData(InFilePath.ToString(), /*bDelayParse*/false);
	return OutExtAssetData.IsParsed();
}

/////////////////////////////////////////////////////
// FExtAssetCounter
//
int32 FExtAssetCounter::CountAssetsByFolder(const FString& InFolder, FExtAssetCountInfo& OutCountInfo)
{
	FExtAssetRegistry& ExtAssetRegistry = FExtContentBrowserSingleton::GetAssetRegistry();
	FName FolderName(*InFolder);
	if (const TArray<FExtAssetData*>* AssetsByFolder = ExtAssetRegistry.GetOrCacheAssetsByFolder(FolderName, /*bDealyParse*/true).Find(FolderName))
	{
		for (const FExtAssetData* AssetData : *AssetsByFolder)
		{
			if (AssetData == nullptr)
			{
				continue;
			}

			++OutCountInfo.TotalAssetsIncludeUMap;

			FString AssetFile = AssetData->PackageFilePath.ToString();

			if (AssetData->IsValid())
			{
				OutCountInfo.TotalFileSizes += AssetData->FileSize;
			}
			else
			{
				++OutCountInfo.TotalInvalid;
				int32 FileSize = IFileManager::Get().FileSize(*AssetFile);
				if (FileSize != INDEX_NONE)
				{
					OutCountInfo.TotalFileSizes += FileSize;
				}
			}

			FString Extenstion = FPaths::GetExtension(AssetFile, /*bIncludeDot*/true);
			if (Extenstion.Equals(FExtAssetSupport::AssetPackageExtension, ESearchCase::IgnoreCase))
			{
				++OutCountInfo.TotalUAsset;
			}
			else if(Extenstion.Equals(FExtAssetSupport::MapPackageExtension, ESearchCase::IgnoreCase))
			{
				++OutCountInfo.TotalUMap;
			}
		}

		return OutCountInfo.TotalAssetsIncludeUMap;
	}

	return 0;
}

////////////////////////////////////////////////
// FExtContentDirFinder
//

namespace FExtContentDirFinderHelpers
{
	struct FFileVisitor : public IPlatformFile::FDirectoryVisitor
	{
		FFileVisitor(const TCHAR* InMatch, bool bInMatchExtension)
			: Match(InMatch)
			, bMatchExtension(bInMatchExtension)
			, bFound(false)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				FString FileName = FPaths::GetCleanFilename(FilenameOrDirectory);
				bFound = bMatchExtension ? FileName.EndsWith(*Match) : FileName.Equals(*Match, ESearchCase::CaseSensitive);
				if (bFound)
				{
					return false;
				}
			}
			return true;
		}

		FString Match;
		bool bMatchExtension;
		bool bFound;
	};

	struct FFolderVisitor : public IPlatformFile::FDirectoryVisitor
	{
		FFolderVisitor(const TCHAR* InMatch)
			: Match(InMatch)
			, bFound(false)
		{
			FPaths::NormalizeDirectoryName(Match);
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				FString NormalizedDir(FilenameOrDirectory);
				FPaths::NormalizeDirectoryName(NormalizedDir);
				
				bFound = Match.Equals(*Match, ESearchCase::CaseSensitive);
				if (bFound)
				{
					return false;
				}
			}
			return true;
		}

		FString Match;
		bool bFound;
	};

	struct FFileFinder
	{
		static bool FindFile(const FString& InputFilePath, FFileVisitor& InputFileVisitor, const TCHAR* InputFind, FString& OutputContentPath, FString& OutputRelativePath)
		{
			bool bFound = false;
			FString ContentPath;

			int32 FindIndex = InputFilePath.Find(InputFind, ESearchCase::CaseSensitive, ESearchDir::FromEnd, InputFilePath.Len());
			for (; FindIndex != INDEX_NONE; FindIndex = InputFilePath.Find(InputFind, ESearchCase::CaseSensitive, ESearchDir::FromEnd, FindIndex))
			{
				ContentPath = InputFilePath.Mid(0, FindIndex);

				if (IFileManager::Get().DirectoryExists(*ContentPath))
				{
					IFileManager::Get().IterateDirectory(*ContentPath, InputFileVisitor);
					bFound = InputFileVisitor.bFound;

					if (bFound)
					{
						ContentPath = FPaths::Combine(ContentPath, InputFind);
						OutputContentPath = ContentPath;
						OutputRelativePath = InputFilePath.Mid(FCString::Strlen(*ContentPath) + 1);
						break;
					}
				}
			}

			return bFound;
		}

		static bool FindFolder(const FString& InputFilePath, FFolderVisitor& InputFolderVisitor, const TCHAR* InputFind, FString& OutputContentPath, FString& OutputRelativePath)
		{
			bool bFound = false;
			FString ContentPath;

			int32 FindIndex = InputFilePath.Find(InputFind, ESearchCase::CaseSensitive, ESearchDir::FromEnd, InputFilePath.Len());
			for (; FindIndex != INDEX_NONE; FindIndex = InputFilePath.Find(InputFind, ESearchCase::CaseSensitive, ESearchDir::FromEnd, FindIndex))
			{
				ContentPath = InputFilePath.Mid(0, FindIndex);

				if (IFileManager::Get().DirectoryExists(*ContentPath))
				{
					IFileManager::Get().IterateDirectory(*ContentPath, InputFolderVisitor);
					bFound = InputFolderVisitor.bFound;

					if (bFound)
					{
						ContentPath = FPaths::Combine(ContentPath, InputFind);
						OutputContentPath = ContentPath;
						OutputRelativePath = InputFilePath.Mid(FCString::Strlen(*ContentPath) + 1);
						break;
					}
				}
			}

			return bFound;
		}

		static bool FindFolder(const FString& InputFolder, FFileVisitor& InputFileVisitor)
		{
			bool bFound = false;
			if (IFileManager::Get().DirectoryExists(*InputFolder))
			{
				IFileManager::Get().IterateDirectory(*InputFolder, InputFileVisitor);
				bFound = InputFileVisitor.bFound;
			}
			return bFound;
		}
	};
};

bool FExtContentDirFinder::FindWithFile(const FString& InUAssetFilePath, const FString& InFileToFind, bool bExtension, const FString& InContentFolderPattern, FString& OutContentRootPath, FString& OutRelativePath)
{
	FExtContentDirFinderHelpers::FFileVisitor FileVisitor(*InFileToFind, /*bExtension*/ bExtension);
	const bool bFoundRootPath = FExtContentDirFinderHelpers::FFileFinder::FindFile(InUAssetFilePath, FileVisitor, *InContentFolderPattern, OutContentRootPath, OutRelativePath);
	return bFoundRootPath;
}

bool FExtContentDirFinder::FindFolder(const FString& InUAssetFilePath, const FString& InContentFolderPattern, FString& OutContentRootPath, FString& OutRelativePath)
{
	FExtContentDirFinderHelpers::FFolderVisitor FolderVisitor(*InContentFolderPattern);
	const bool bFoundRootPath = FExtContentDirFinderHelpers::FFileFinder::FindFolder(InUAssetFilePath, FolderVisitor, *InContentFolderPattern, OutContentRootPath, OutRelativePath);
	return bFoundRootPath;
}

bool FExtContentDirFinder::FindWithFolder(const FString& InFolderPath, const FString& InFileToFind, bool bExtension)
{
	FExtContentDirFinderHelpers::FFileVisitor FileVisitor(*InFileToFind, /*bExtension*/ bExtension);
	const bool bFoundRootPath = FExtContentDirFinderHelpers::FFileFinder::FindFolder(InFolderPath, FileVisitor);
	return bFoundRootPath;
}


////////////////////////////////////////////////
// FAssetDataUtil
//

void FAssetDataUtil::PlaceToCurrentLevelViewport(const TArray<FAssetData>& InAssets)
{
	FLevelEditorViewportClient* ViewportClient = GCurrentLevelEditingViewportClient;

	if (InAssets.Num() < 1 && !ViewportClient)
	{
		return;
	}

	TArray< UObject* > Assets;
	for (int Index = 0; Index < InAssets.Num(); Index++)
	{
		const FAssetData& AssetData = InAssets[Index];
		if (AssetData.IsValid())
		{
			if (UObject* Asset = AssetData.GetAsset())
			{
				Assets.Add(Asset);
			}
		}
	}

	TArray<AActor*> OutNewActors;
	const bool bCreateDropPreview = false;
	const bool SelectActor = false;
	const FViewport* const Viewport = ViewportClient->Viewport;

	bool AllAssetsCanBeDropped = true;
	// Determine if we can drop the assets
	for (auto AssetIt = Assets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		UObject* Asset = *AssetIt;
		FDropQuery DropResult = ViewportClient->CanDropObjectsAtCoordinates(Viewport->GetMouseX(), Viewport->GetMouseY(), FAssetData(Asset));
		if (!DropResult.bCanDrop)
		{
			// At least one of the assets can't be dropped.
			ViewportClient->DestroyDropPreviewActors();
			AllAssetsCanBeDropped = false;
		}
	}

	if (AllAssetsCanBeDropped)
	{
		ViewportClient->DropObjectsAtCoordinates(Viewport->GetMouseX(), Viewport->GetMouseY(), Assets, OutNewActors, /*bOnlyDropOnTarget*/false, bCreateDropPreview, SelectActor);
	}
}

void FAssetDataUtil::ExportAssetsWithDialog(const TArray<FAssetData>& InAssets)
{
	struct Local
	{
		static void GatherAssets(const TArray<FAssetData>& InputAssets, TArray<UObject*>& OutObjects, bool bInputSkipRedirectors)
		{
			for (int32 AssetIdx = 0; AssetIdx < InputAssets.Num(); ++AssetIdx)
			{
				if (bInputSkipRedirectors && (InputAssets[AssetIdx].AssetClassPath.GetPackageName() == UObjectRedirector::StaticClass()->GetFName()))
				{
					// Don't operate on Redirectors
					continue;
				}

				UObject* Object = InputAssets[AssetIdx].GetAsset();

				if (Object)
				{
					OutObjects.Add(Object);
				}
			}
		}
	};

	TArray<UObject*> ObjectsToExport;
	const bool bSkipRedirectors = true;
	Local::GatherAssets(InAssets, ObjectsToExport, bSkipRedirectors);

	if (ObjectsToExport.Num() > 0)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

		AssetToolsModule.Get().ExportAssetsWithDialog(ObjectsToExport, /*bPromptForIndividualFilenames*/true);

		const bool bKillAfterExport = true;
		if (bKillAfterExport)
		{
			for (UObject* Asset : ObjectsToExport)
			{
				if (IsValid(Asset))
				{
					Asset->MarkAsGarbage();
				}
			}
		}
	}
}

///////////////////////////////
// Serializer

constexpr uint32 ExtAssetRegistryNumberedNameBit = 0x80000000;

bool FExtAssetRegistryVersion::SerializeVersion(FArchive& Ar, Type& Version)
{
	FGuid Guid = FExtAssetRegistryVersion::GUID;

	if (Ar.IsLoading())
	{
		Version = FExtAssetRegistryVersion::BeforeCustomVersionWasAdded;
	}

	Ar << Guid;

	if (Ar.IsError())
	{
		return false;
	}

	if (Guid == FExtAssetRegistryVersion::GUID)
	{
		int32 VersionInt = Version;
		Ar << VersionInt;
		Version = (FExtAssetRegistryVersion::Type)VersionInt;

		Ar.SetCustomVersion(Guid, VersionInt, TEXT("ExtAssetRegistry"));
	}
	else
	{
		return false;
	}

	return !Ar.IsError();
}

//////////////////////////////////////////////////////////////

FAssetDataTagMapSharedView LoadTags(class FExtAssetRegistryReader& Reader);

class FExtAssetRegistryReader : public FArchiveProxy
{
public:
	/// @param NumWorkers > 0 for parallel loading
	FExtAssetRegistryReader(FArchive& Inner)
		: FArchiveProxy(Inner)
	{
		check(IsLoading());
		Names = LoadNameBatch(Inner);
		Tags = FixedTagPrivate::LoadStore(*this);
	}

	~FExtAssetRegistryReader()	{}

	virtual FArchive& operator<<(FName& Out) override
	{
		checkf(Names.Num() > 0, TEXT("Attempted to load FName before name batch loading has finished"));

		uint32 Index = 0;
		uint32 Number = NAME_NO_NUMBER_INTERNAL;

		*this << Index;

		if (Index & ExtAssetRegistryNumberedNameBit)
		{
			Index -= ExtAssetRegistryNumberedNameBit;
			*this << Number;
		}

		Out = Names[Index].ToName(Number);

		return *this;
	}

	void SerializeTagsAndBundles(FExtAssetData& Out)
	{
		Out.TagsAndValues = LoadTags(*this);
	}

private:
	TArray<FDisplayNameEntryId> Names;
	TRefCountPtr<const FixedTagPrivate::FStore> Tags;

	friend FAssetDataTagMapSharedView LoadTags(FExtAssetRegistryReader& Reader);
};

FAssetDataTagMapSharedView LoadTags(FExtAssetRegistryReader& Reader)
{
	uint64 MapHandle;
	Reader << MapHandle;
	return FAssetDataTagMapSharedView(FixedTagPrivate::FPartialMapHandle::FromInt(MapHandle).MakeFullHandle(Reader.Tags->Index));
}

class FExtAssetRegistryWriterBase
{
protected:
	FLargeMemoryWriter MemWriter;
};

/**
 * Indexes FName and tag maps and serializes out deduplicated indices instead.
 *
 * Unlike previous FNameTableArchiveWriter:
 * - Name data stored as name batches, which is faster
 * - Name batch is written as a header instead of footer for faster seek-free loading
 * - Numberless FNames are serialized as a single 32-bit int
 * - Deduplicates all tag values, not just names
 *
 * Use in conjunction with FNameBatchReader.
 *
 * Data is written to inner archive in destructor to achieve seek-free loading.
 */
class FExtAssetRegistryWriter : public FExtAssetRegistryWriterBase, public FArchiveProxy
{
public:
	FExtAssetRegistryWriter(FArchive& Out)
		: FArchiveProxy(MemWriter)
		, Tags(FixedTagPrivate::FOptions())
		, TargetAr(Out)
	{
		check(!IsLoading());
	}

	static TArray<FDisplayNameEntryId> FlattenIndex(const TMap<FDisplayNameEntryId, uint32>& Names)
	{
		TArray<FDisplayNameEntryId> Out;
		Out.SetNumZeroed(Names.Num());
		for (TPair<FDisplayNameEntryId, uint32> Pair : Names)
		{
			Out[Pair.Value] = Pair.Key;
		}
		return Out;
	}

	~FExtAssetRegistryWriter()
	{
		// Save store data and collect FNames
		int32 BodySize = MemWriter.TotalSize();
		SaveStore(Tags.Finalize(), *this);

		// Save in load-friendly order - names, store then body / tag maps
#if ECB_WIP_CACHEDB_LOADNAMEBATCH
		SaveNameBatch(FlattenIndex(Names), TargetAr);
#endif
		TargetAr.Serialize(MemWriter.GetData() + BodySize, MemWriter.TotalSize() - BodySize);
		TargetAr.Serialize(MemWriter.GetData(), BodySize);
	}

	virtual FArchive& operator<<(FName& Value) override
	{
		FDisplayNameEntryId EntryId(Value);

		uint32 Index = Names.FindOrAdd(EntryId, Names.Num());
		check((Index & ExtAssetRegistryNumberedNameBit) == 0);

		if (Value.GetNumber() != NAME_NO_NUMBER_INTERNAL)
		{
			Index |= ExtAssetRegistryNumberedNameBit;
			uint32 Number = Value.GetNumber();
			return *this << Index << Number;
		}

		return *this << Index;
	}

	void SaveTags(FExtAssetRegistryWriter& Writer, const FAssetDataTagMapSharedView& Map)
	{
		uint64 MapHandle = Writer.Tags.AddTagMap(Map).ToInt();
		Writer << MapHandle;
	}

	void SerializeTagsAndBundles(const FExtAssetData& Out)
	{
		SaveTags(*this, Out.TagsAndValues);
	}

private:
	TMap<FDisplayNameEntryId, uint32> Names;
	FixedTagPrivate::FStoreBuilder Tags;
	FArchive& TargetAr;

	friend void SaveTags(FExtAssetRegistryWriter& Writer, const FAssetDataTagMapSharedView& Map);
};
//////////////////////////////////////////////////////////////

#if ECB_WIP_CACHEDB
template<class Archive>
void FExtAssetData::SerializeForCache(Archive&& Ar)
{
	Ar << PackageFilePath;
	Ar << PackageName;
	Ar << PackagePath;
	Ar << ObjectPath;
	Ar << AssetName;
	Ar << AssetClass;
	//FAssetData AssetData;  (Dummy data for FAssetData based interface)
	Ar << AssetContentRoot;
	Ar << AssetRelativePath;
	Ar << AssetContentType; // EContentType 
	//Ar << AssetData.TagsAndValues; // FAssetDataTagMapSharedView
	Ar.SerializeTagsAndBundles(*this);
	Ar << HardDependentPackages; // TSet<FName> 
	Ar << SoftReferencesList; // TSet<FName> 
	Ar << FileVersionUE4;
	Ar << SavedByEngineVersion; // FEngineVersion 
	Ar << CompatibleWithEngineVersion; // FEngineVersion 
	Ar << InvalidReason; // EInvalidReason
	// Debug info
	Ar << AssetCount;
	Ar << ThumbCount;
	Ar << FileSize;
	// Status
	Ar << bParsed;
	Ar << bValid;
	Ar << bHasThumbnail;
	Ar << bCompatible;
}
#endif


FORCEINLINE FArchive& operator<<(FArchive& Ar, FExtAssetDependencyInfo& DependencyInfo)
{
	Ar << DependencyInfo.ValidDependentFiles;
	Ar << DependencyInfo.InvalidDependentFiles;
	Ar << DependencyInfo.MissingDependentFiles;

	Ar << DependencyInfo.SkippedPackageNames;
	Ar << DependencyInfo.MissingPackageNames;

	Ar << DependencyInfo.AllDepdencyPackages;
	Ar << DependencyInfo.AssetDepdencies;
	Ar << DependencyInfo.AssetStatus;

	return Ar;
}

FORCEINLINE FArchive& operator<<(FArchive& Ar, FExtAssetDependencyNode& DependencyNode)
{
	Ar << DependencyNode.PackageName;
	Ar << DependencyNode.ReferenceType;
	Ar << DependencyNode.NodeStatus;

	return Ar;
}

FORCEINLINE FArchive& operator<<(FArchive& Ar, FRootContentPathInfo& RootContentPathInfo)
{
	Ar << RootContentPathInfo.ContentType;
	Ar << RootContentPathInfo.AssetContentRoot;
	Ar << RootContentPathInfo.DisplayName;

	return Ar;
}

//////////////////////////////////////////////////////////////

#if ECB_WIP_CACHEDB

template<class Archive>
bool FExtAssetRegistryStateCache::SerializeForCache(Archive&& Ar)
{
	return Ar.IsSaving() ? Save(Ar) : Load(Ar);
}

bool FExtAssetRegistryStateCache::Save(FArchive& OriginalAr)
{
	check(!OriginalAr.IsLoading());

	FExtAssetRegistryVersion::Type Version = FExtAssetRegistryVersion::LatestVersion;
	FExtAssetRegistryVersion::SerializeVersion(OriginalAr, Version);

	// Set up fixed asset registry writer
	FExtAssetRegistryWriter Ar(OriginalAr);

	// serialize number of objects
	int32 AssetCount = CachedAssetsByFilePath.Num();
	Ar << AssetCount;
	NumAssetsInCache = AssetCount;

	// Write asset data first
	for (TPair<FName, FExtAssetData*>& Pair : CachedAssetsByFilePath)
	{
		FExtAssetData& AssetData(*Pair.Value);
		AssetData.SerializeForCache(Ar);
	}

	SerializeDependencyInfos(Ar);

	NumFoldersInCache = CachedSubPaths.Num();
	SerializeMisc(Ar);

	PrintStatus();

	return !Ar.IsError();
}

bool FExtAssetRegistryStateCache::Load(FArchive& Ar)
{
	check(Ar.IsLoading());
	
	FExtAssetRegistryVersion::Type Version = FExtAssetRegistryVersion::LatestVersion;
	FExtAssetRegistryVersion::SerializeVersion(Ar, Version);

	FSoftObjectPathSerializationScope SerializationScope(NAME_None, NAME_None, ESoftObjectPathCollectType::NeverCollect, ESoftObjectPathSerializeType::AlwaysSerialize);

	if (Version < FExtAssetRegistryVersion::SaveLoadNameBatch)
	{
		ECB_LOG(Warning, TEXT("Can't load cache db: version too old."));
		return false;
	}
	else
	{
		FExtAssetRegistryReader Reader(Ar);

		if (Reader.IsError())
		{
			return false;
		}

		Load(Reader, Version);
	}

	return !Ar.IsError();
}

template<class Archive>
void FExtAssetRegistryStateCache::Load(Archive&& Ar, FExtAssetRegistryVersion::Type Version)
{
	// serialize number of objects
	int32 LocalNumAssets = 0;
	Ar << LocalNumAssets;

	NumAssetsInCache = LocalNumAssets;

	// allocate one single block for all asset data structs (to reduce tens of thousands of heap allocations)
	TArrayView<FExtAssetData> PreallocatedAssetDataBuffer(new FExtAssetData[LocalNumAssets], LocalNumAssets);
	PreallocatedAssetDataBuffers.Add(PreallocatedAssetDataBuffer.GetData());
	for (FExtAssetData& NewAssetData : PreallocatedAssetDataBuffer)
	{
		NewAssetData.SerializeForCache(Ar);
	}

	SetAssetDatas(PreallocatedAssetDataBuffer);

	SerializeDependencyInfos(Ar);

	SerializeMisc(Ar);
	NumFoldersInCache = CachedSubPaths.Num();

	PrintStatus();
}


void FExtAssetRegistryStateCache::SetAssetDatas(TArrayView<FExtAssetData> AssetDatas)
{
	int32 NumAssets = AssetDatas.Num();

	// CachedAssetsByFolder
	// CachedAssetsByFilePath
	// CachedPackageNameToFilePathMap
	// CachedUnParsedAssets
	// CachedAssetsByClass
	// CachedAssetsByTag
	auto SetPathCache = [&]()
	{
		CachedAssetsByFilePath.Empty(AssetDatas.Num());
		for (FExtAssetData& AssetData : AssetDatas)
		{
			CacheNewAsset(&AssetData);
		}
	};
	SetPathCache();
}

void FExtAssetRegistryStateCache::SerializeDependencyInfos(FArchive& Ar)
{
	Ar << CachedDependencyInfoByFilePath;
}

void FExtAssetRegistryStateCache::SerializeMisc(FArchive& Ar)
{
	Ar << CachedRootContentPaths;

	Ar << CachedSubPaths;
	Ar << CachedEmptyFoldersStatus;
	Ar << CachedFullyParsedFolders;
	Ar << CachedFilePathsByFolder;

	Ar << CachedAssetContentRoots;
	Ar << CachedAssetContentType;
	Ar << CachedAssetContentRootHosts;
	Ar << CachedAssetContentRootConfigDirs;

	Ar << CachedFolderColorIndices;
	Ar << CachedFolderColors;
}

#endif

//////////////////////////////////////////////////////////////

#if ECB_WIP_CACHEDB
namespace CachDBFileUtil
{
	bool SaveBufferToFile(const FString& FullFilePath, FBufferArchive& ToBinary)
	{
		//No Data 
		if (ToBinary.Num() <= 0) return false;

		if (FFileHelper::SaveArrayToFile(ToBinary, *FullFilePath))
		{
			ToBinary.FlushCache();
			ToBinary.Empty();
			UE_LOG(LogECB, Warning, TEXT("[SaveBufferToFile] success!"));
			return true;
		}
		// Free Binary Array 
		ToBinary.FlushCache();
		ToBinary.Empty();
		UE_LOG(LogECB, Warning, TEXT("[SaveBufferToFile] fail!"));
		return false;
	}
}

bool FExtAssetRegistry::LoadCacheDB(bool bSilent/* = true*/)
{
	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	FString FullFilePath = ExtContentBrowserSetting->CacheFilePath.FilePath;

	TArray<uint8> TheBinaryArray;

	if (!FFileHelper::LoadFileToArray(TheBinaryArray, *FullFilePath))
	{
		ECB_LOG(Warning, TEXT("[FExtAssetRegistry::LoadCacheDB] Failed to Load Cache DB from File!"));
		return false;
	}

	//File Load Error 
	if (TheBinaryArray.Num() <= 0)
	{
		return false;
	}

	FMemoryReader* FromBinary = new FMemoryReader(TheBinaryArray, true);
	//true, free data after done 
	FromBinary->Seek(0);

	bool bSerializeResult = State.SerializeForCache(*FromBinary);
	FromBinary->FlushCache();

	// Empty & Close Buffer 
	TheBinaryArray.Empty();
	FromBinary->Close();

	delete FromBinary;
	FromBinary = nullptr;	

	State.PrintStatus();

	if (bSerializeResult && !bSilent)
	{
		FString Message = FString::Printf(TEXT("Cache file \"%s\" loaded."), *FullFilePath);
		ExtContentBrowserUtils::NotifyMessage(Message);
	}

	return bSerializeResult;
}

bool FExtAssetRegistry::LoadCacheDBWithFilePicker()
{
	return false;
}

bool FExtAssetRegistry::SaveCacheDB(bool bSilent/* = true*/)
{
	UExtContentBrowserSettings* ExtContentBrowserSetting = GetMutableDefault<UExtContentBrowserSettings>();
	FExtContentBrowserModule& ExtContentBrowserModule = FModuleManager::GetModuleChecked<FExtContentBrowserModule>(TEXT("ExtContentBrowser"));

	FString FullFilePath = ExtContentBrowserSetting->CacheFilePath.FilePath;

	FBufferArchive ToBinary;

	if (State.SerializeForCache(ToBinary))
	{
		if (CachDBFileUtil::SaveBufferToFile(FullFilePath, ToBinary))
		{
			if (!bSilent)
			{
				FString Message = FString::Printf(TEXT("Cache file \"%s\" saved."), *FullFilePath);
				ExtContentBrowserUtils::NotifyMessage(Message);
				return true;
			}
		}
	}

	return false;
}

bool FExtAssetRegistry::PurgeCacheDB(bool bSilent/* = true*/)
{
	if (State.PurgeAssets(bSilent))
	{
		return SaveCacheDB();
	}
	
	return false;
}


void FExtAssetRegistry::SwitchCacheMode()
{
	StopAllBackgroundGathering();

	State.Reset();

	const UExtContentBrowserSettings* ExtContentBrowserSetting = GetDefault<UExtContentBrowserSettings>();
	const bool bCacheMode = ExtContentBrowserSetting->bCacheMode;
	if (bCacheMode)
	{
 		LoadCacheDB();
	}

	{
		LoadRootContentPaths();
	}
}

#endif

#undef LOCTEXT_NAMESPACE

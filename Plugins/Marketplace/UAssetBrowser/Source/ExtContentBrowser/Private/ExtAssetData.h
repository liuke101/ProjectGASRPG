// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtContentBrowser.h"
#include "ExtContentBrowserTypes.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/LinkerLoad.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetDataTagMap.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/AssetRegistryInterface.h"
#include "HAL/Runnable.h"

#include "ExtFeedbackContextEditor.h"
#include "HAL/FeedbackContextAnsi.h"
#include "Serialization/BufferArchive.h"

#include "ExtAssetData.generated.h"

struct FExtAssetIdentifier;
struct FARFilter;

struct FExtAssetDependencyNode;
struct FExtAssetDependencyWalker;

/**
* 
*/
struct FExtAssetRegistryVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,

		SaveLoadNameBatch, // 4.27+

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

	/** Read/write the custom version to the archive, should call at the very beginning */
	static bool SerializeVersion(FArchive& Ar, FExtAssetRegistryVersion::Type& Version);

private:
	FExtAssetRegistryVersion() {}
};

/**
 * Holds info of supported asset type/extensions
 */
struct FExtAssetSupport
{
	static FString AssetPackageExtension;
	static FString MapPackageExtension;
	static FString TextAssetPackageExtension;
	static FString TextMapPackageExtension;

	static FORCEINLINE bool IsSupportedPackageFilename(const FString& Filename)
	{
		const bool bShouldSupportMapPackage = true;

		const bool bIsAssetPackage = Filename.EndsWith(FExtAssetSupport::AssetPackageExtension);
		const bool bIsMapMapPackage = Filename.EndsWith(FExtAssetSupport::MapPackageExtension);

		return bShouldSupportMapPackage ? (bIsAssetPackage || bIsMapMapPackage) : bIsAssetPackage;
	}
};

/**
 * Holds Constants used in ExtAsstData
 */
namespace FExtAssetContants
{
	const FString ExtTempPackagePath(TEXT("/Temp/UAssetBrowser"));
	const FString ExtSandboxPackagePath(TEXT("/UAssetBrowser/Sandbox"));
	const FName DefaultImportedUAssetCollectionName(TEXT("ImportedUAssets"));
};

#if ECB_WIP_ASSETREGISTRY_SERIALIZATION

/** Version used for serializing ext asset registry caches */
struct ASSETREGISTRY_API FExtAssetRegistryVersion
{
	enum Type
	{
		PreVersioning = 0,		// From before file versioning was implemented

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	/** The GUID for this custom version number */
	const static FGuid GUID;

	/** Read/write the custom version to the archive, should call at the very beginning */
	static bool SerializeVersion(FArchive& Ar, FAssetRegistryVersion::Type& Version);

private:
	FExtAssetRegistryVersion() {}
};

#endif

/**
 * A struct to hold external asset information
 */
USTRUCT(BlueprintType)
struct FExtAssetData
{
	GENERATED_BODY()

public:

	enum class EInvalidReason :uint8
	{
		NotParsed,

		Valid,

		// File Path
		FileNotFound,
		FilePathEmptyOrInvalid,
		FilePathContainsInvalidCharacter,

		// From LinkerLoad
		FailedToLoadPackage,

		// From Package Reader
		PackageFileVersionEmptyOrTooOld,
		PackageFileOrCustomVersionTooNew,
		PackageFileMalformed,
		PackageFileCustomVersionMissing,
		PackageFileFailedToLoad,

		// Basic info
		NoAssetData,
		AssetNameEmpty,
		ClassNameEmpty,

		// Not Supported Type
		GenericAssetTypeNotSupported,
		RedirectorNotSupported,
		ClassAssetNotSupported,
		TextForamtNotSupported,
		CookedPackageNotSupported,

		NotCompatiableWithCurrentEngineVersion,

		Unknown
	};

	enum class EContentType :uint8
	{
		Plugin,
		Project,
		VaultCache,
		Orphan,

		PartialProject,

		Unknown
	};

public:

	/** Full file path to package disk file */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FName PackageFilePath;

	/** The name of the package in which the asset is found, this is the full long package name such as /Game/Path/Package */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FName PackageName;

	/** The path to the package in which the asset is found, this is /Game/Path with the Package stripped off */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FName PackagePath;

	/** The object path for the asset in the form PackageName.AssetName. e.g. /Game/Path/Package.AssetName */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FName ObjectPath;

	/** The name of the asset without the package */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FName AssetName;

	/** The name of the asset's class */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FName AssetClass;

	/** Asset data to satisfy FAssetData based interface. */
	UPROPERTY(BlueprintReadOnly, Category = AssetData, transient)
		FAssetData AssetData;

	// Content root of current asset
	FName AssetContentRoot = NAME_None;

	// Content root folder name, can get from AssetContentRootDir but cache here for performance
	//FName AssetContentRootDirName = NAME_None;

	// Relative path to asset content root
	FName AssetRelativePath = NAME_None;

	// Asset content type: plugin/project/vaultcahce/orphan
	EContentType AssetContentType;

	/** Asset registry tags */
	FAssetDataTagMapSharedView TagsAndValues;

	/** Hard Dependencies */
	TSet<FName> HardDependentPackages;

	/** Soft References */
	TSet<FName> SoftReferencesList;

	/** UE4 file version */
	int32 FileVersionUE4 = 0;

	/** Engine version this package was saved with.*/
	FEngineVersion SavedByEngineVersion;

	/** Engine version this package is compatible with. See SavedByEngineVersion. */
	FEngineVersion CompatibleWithEngineVersion;

	/** Gathered invalid reason */
	EInvalidReason InvalidReason;

#if 1 //ECB_DEBUG
	int32 AssetCount = 0;
	int32 ThumbCount = 0;
	int64 FileSize = 0;
#endif

public:

	/** Default Constructor. */
	FExtAssetData();

	/** Constructor taking a package file path. */
	FExtAssetData(const FString& InPackageFilePath, bool bDelayParse = false);

	/** Constructor taking a UObject. By default trying to create one for a blueprint class will create one for the UBlueprint instead, but this can be overridden */
	FExtAssetData(const UObject* InAsset, bool bAllowBlueprintClass = false);

	/** Constructor taking a package name. For support wrapping an internal package */
	FExtAssetData(const FName& InPackageName);

	/** Parse .uasset file to gather asset data, assume package file path has been assigned in constructor */
	bool Parse();
	bool ReParse();

	/** Check if has thumbnail data. */
	bool HasThumbnail() const;

	void SetHasThumbnail(bool bInHasThumbnail)
	{
		bHasThumbnail = bInHasThumbnail;
	}

	/** Check if asset has valid content root (project or vault cache data folder) */
	bool HasContentRoot() const;

	/** Load and check if has valid thumbnail data. */
	bool HasValidThumbnail() const;

	/** Check if can import this asset in fastest way */
	bool CanImportFast() const;

	/** Load thumbnail from package. */
	bool LoadThumbnail(FObjectThumbnail& OutThumbnail) const;

#if ECB_LEGACY
	/** Load soft package references from package. */
	void LoadSoftReferences(TArray<FName>& OutSoftReferences) const;
#endif

	/** Get saved engine version string */
	FString GetSavedEngineVersionForDisplay() const;
	const FString& GetAssetContentTypeForDisplay() const;

	/** Checks compatibility with current engine version. */
	bool IsCompatibleWithCurrentEngineVersion() const;

	FString GetInvalidReason() const;

public: // AssetData compatible interface

	/** Prints the details of the asset to the log */
	void PrintAssetData() const;

	/** Check if is parsed and has valid asset name */
	bool IsValid() const
	{
		return bParsed && bValid;//&& AssetName != NAME_None;
	}

	bool IsUAsset() const;

	/**
	 * Returns true iff the Asset is a TopLevelAsset (not a subobject, its outer is a UPackage).
	 * Only TopLevelAssets can be PrimaryAssets in the AssetManager.
	 * A TopLevelAsset is not necessarily the main asset in a package; see IsUAsset.
	 * Note that this is distinct from UObject::IsAsset because IsAsset can be overloaded (see e.g. AActor::IsAsset)
	 */
	bool IsTopLevelAsset() const;

	bool IsUMap() const;

	/** Check if is parsed and has valid asset name */
	bool IsParsed() const
	{
		return bParsed;
	}

	/** Returns true if the asset is loaded */
	bool IsAssetLoaded() const
	{
		return IsValid() && FindObjectSafe<UObject>(NULL, *ObjectPath.ToString()) != NULL;
	}

	/** Try and get the value associated with the given tag as a type converted value */
	template <typename ValueType>
	bool GetTagValue(const FName InTagName, ValueType& OutTagValue) const;

	UClass* GetIconClass(bool* bOutIsClassType) const;

	/** Returns the asset UObject if it is loaded or loads the asset if it is unloaded then returns the result */
	UObject* GetAsset() const
	{
		if (!IsValid())
		{
			// Dont even try to find the object if the objectpath isn't set
			return NULL;
		}

		UObject* Asset = FindObject<UObject>(NULL, *ObjectPath.ToString());
		if (Asset == NULL)
		{
			Asset = LoadObject<UObject>(NULL, *ObjectPath.ToString());
		}

		return Asset;
	}

	/** Returns the full name for the asset in the form: Class ObjectPath */
	FString GetFullName() const
	{
		FString FullName;
		GetFullName(FullName);
		return FullName;
	}

	/** Convert to a SoftObjectPath. PackageName.AssetName for ToplevelAssets. OptionalOuterName and AssetName joined by : or . for sub-object assets */
	FSoftObjectPath GetSoftObjectPath() const;

	/** Returns the folder path of the asset package file */
	FString GetFolderPath() const;

	/** Populates OutFullName with the full name for the asset in the form: Class ObjectPath */
	void GetFullName(FString& OutFullName) const
	{
		OutFullName.Reset();
		AssetClass.AppendString(OutFullName);
		OutFullName.AppendChar(' ');
		ObjectPath.AppendString(OutFullName);
	}

	/** Returns the class UClass if it is loaded. It is not possible to load the class if it is unloaded since we only have the short name. */
	UClass* GetClass() const
	{
		if (!IsValid())
		{
			// Dont even try to find the class if the objectpath isn't set
			return NULL;
		}

		UClass* FoundClass = FindObject<UClass>(nullptr, *AssetClass.ToString());

		if (!FoundClass)
		{
			// Look for class redirectors
			FName NewPath = FLinkerLoad::FindNewNameForClass(AssetClass, false);

			if (NewPath != NAME_None)
			{
				FoundClass = FindObject<UClass>(nullptr, *NewPath.ToString());
			}
		}
		return FoundClass;
	}

	bool operator==(const FExtAssetData& Other) const
	{
		return ObjectPath == Other.ObjectPath;
	}

	bool operator!=(const FExtAssetData& Other) const
	{
		return ObjectPath != Other.ObjectPath;
	}

#if ECB_WIP_CACHEDB
	template<class Archive>
	void SerializeForCache(Archive&& Ar);
#endif

	static FString ConvertFilePathToTempPackageName(const FString& InFilePath);

	static FString& GetSandboxPackagePath();
	static FString& GetSandboxPackagePathWithSlash();

	static FString& GetSandboxDir();

	static FString& GetUAssetBrowserTempDir();

	static FString& GetImportSessionTempDir();

	static FString GetZipExportSessionTempDir();

public: 

private:

	/** Parse .uasset file to gather asset data */
	bool PreParse();

	/** Parse .uasset file to gather asset data */
	bool DoParse();


	void AppendObjectPath(FString& String) const
	{
		TStringBuilder<FName::StringBufferSize> Builder;
		AppendObjectPath(Builder);
		String.Append(FString(Builder));
	}

	/** Append the object path to the given string builder. */
	void AppendObjectPath(FStringBuilderBase& Builder) const
	{
		if (!IsValid())
		{
			return;
		}

#if WITH_EDITORONLY_DATA
		if (!OptionalOuterPath.IsNone())
		{
			UE::AssetRegistry::Private::ConcatenateOuterPathAndObjectName(Builder, OptionalOuterPath, AssetName);
		}
		else
#endif
		{
			Builder << PackageName << '.' << AssetName;
		}

	}

	void CheckIfCompatibleWithCurrentEngineVersion();

private:

	/** If the package has been parsed */
	bool bParsed;

	/** If the package is valid */
	bool bValid;

	/** If has thumbnail data */
	bool bHasThumbnail;

	/** If the package is compatible with current engine version */
	bool bCompatible;

#if WITH_EDITORONLY_DATA
	/**
	 * If this object is not a top level asset, this contains the path of the outer of this object.
	 * Non top-level assets may only be used in the editor.
	 * For some assets (such as external actors) this may not start with PackageName.
	 * e.g. PackageName			= /Game/__EXTERNAL_ACTORS__/Maps/MyMap/ABCDE12345
			OptionalOuterPath	= /Game/Maps/MyMap.MyMap:PersistentLevel
			AssetName			= SomeExternalActor
	*/
	FName OptionalOuterPath;
#endif
};

FORCEINLINE uint32 GetTypeHash(const FExtAssetData& AssetData)
{
	return GetTypeHash(AssetData.PackageFilePath);
}

struct FExtAssetDataUtil
{
#if 0
	static bool RemapPackagesToFullFilePath(const FString& InPackageName, const TArray<FString>& InRootPackageNamesToReplace, const FString& InAssetContentDir, FString& OutFullFilePath);
#endif
	static FString GetPluginNameFromAssetContentRoot(const FString& InAssetContentDir);

	static FString GetPackageRootFromFullPackagePath(const FString& InFullPackagePath);

	static bool RemapGamePackageToFullFilePath(const FString& InGameRoot, const FString& InPackageName, const FString& InAssetContentDir, FString& OutFullFilePath, bool bPackageIsFolder = false);

	static bool SyncToContentBrowser(const FExtAssetData& InAssetData);
};

namespace FExtAssetCoreUtil
{
	class FExtAssetFeedbackContext : public FExtFeedbackContextEditor
	{
	public:
		/**
		 * cache the cancel result as FExtFeedbackContextEditor::ReceivedUserCancel seems clears the result right away
		 */
		virtual bool ReceivedUserCancel() override;
	private:
		bool bTaskWasCancelledCache = false;
	};

	class IExtAssetProgressReporter
	{
	public:
		virtual ~IExtAssetProgressReporter() = default;

		virtual void BeginWork(const FText& InDescription, float InAmountOfWork, bool bInterruptible = true) = 0;

		virtual void EndWork() = 0;

		virtual void ReportProgress(float IncrementOfWork, const FText& InMessage) = 0;

		virtual bool IsWorkCancelled() = 0;

		virtual FFeedbackContext* GetFeedbackContext() const = 0;
	};

	class FExtAssetWorkReporter
	{
	public:

		FExtAssetWorkReporter(const TSharedPtr<IExtAssetProgressReporter>& InReporter, const FText& InDescription, float InAmountOfWork, float InIncrementOfWork, bool bInterruptible);
		
		~FExtAssetWorkReporter();

		void ReportNextStep(const FText& InMessage, float InIncrementOfWork);

		void ReportNextStep(const FText& InMessage);

		bool IsWorkCancelled() const;

	private:

		TSharedPtr<IExtAssetProgressReporter> Reporter;

		float DefaultIncrementOfWork;
	};

	class FExtAssetProgressUIReporter : public IExtAssetProgressReporter
	{
	public:
		FExtAssetProgressUIReporter()
			: bIsCancelled(false)
		{
		}

		FExtAssetProgressUIReporter(TSharedRef<FFeedbackContext> InFeedbackContext)
			: FeedbackContext(InFeedbackContext)
			, bIsCancelled(false)
		{
		}

		virtual ~FExtAssetProgressUIReporter()
		{
		}

		/* begin IExtAssetProgressReporter interface */
		virtual void BeginWork(const FText& InTitle, float InAmountOfWork, bool bInterruptible = true) override;
		virtual void EndWork() override;
		virtual void ReportProgress(float Progress, const FText& InMessage) override;
		virtual bool IsWorkCancelled() override;
		FFeedbackContext* GetFeedbackContext() const override;
		/* end IExtAssetProgressReporter interface */

	private:
		TArray< TSharedPtr< FScopedSlowTask > > ProgressTasks;
		TSharedPtr< FFeedbackContext > FeedbackContext;
		bool bIsCancelled;
	};

	class FExtAssetProgressTextReporter : public IExtAssetProgressReporter
	{
	public:
		FExtAssetProgressTextReporter()
			: TaskDepth(0)
			, FeedbackContext(new FFeedbackContextAnsi)
		{
		}

		virtual ~FExtAssetProgressTextReporter()
		{
		}

		// Begin IDataprepProgressReporter interface
		virtual void BeginWork(const FText& InTitle, float InAmountOfWork, bool bInterruptible = true) override;
		virtual void EndWork() override;
		virtual void ReportProgress(float Progress, const FText& InMessage) override;
		virtual bool IsWorkCancelled() override;
		virtual FFeedbackContext* GetFeedbackContext() const override;

	private:
		int32 TaskDepth;
		TUniquePtr<FFeedbackContextAnsi> FeedbackContext;
	};
}

/**
 *  Collection Util
 */
struct FExtCollectionUtil
{
	static FName GetOrCreateCollection(FName CollectionName, bool bUniqueCollection);

	static bool AddAssetsToCollection(const TArray<FAssetData>& InAssets, FName InCollectionName, bool bUniqueCollection);
};

/**
 *  FAssetData Util
 */
struct FAssetDataUtil
{
	static void PlaceToCurrentLevelViewport(const TArray<FAssetData>& InAssets);
	static void ExportAssetsWithDialog(const TArray<FAssetData>& InAssets);
};


/** 
 *A structure defining a thing that can be reference by something else in the asset registry. Represents either a package of a primary asset id
 */
struct FExtAssetIdentifier
{
	/** The name of the package that is depended on, this is always set unless PrimaryAssetType is */
	FName PackageName;
	/** The primary asset type, if valid the ObjectName is the PrimaryAssetName */
	FPrimaryAssetType PrimaryAssetType;
	/** Specific object within a package. If empty, assumed to be the default asset */
	FName ObjectName;
	/** Name of specific value being referenced, if ObjectName specifies a type such as a UStruct */
	FName ValueName;

	/** Can be implicitly constructed from just the package name */
	FExtAssetIdentifier(FName InPackageName, FName InObjectName = NAME_None, FName InValueName = NAME_None)
		: PackageName(InPackageName), PrimaryAssetType(NAME_None), ObjectName(InObjectName), ValueName(InValueName)
	{}

	/** Construct from a primary asset id */
	FExtAssetIdentifier(const FPrimaryAssetId& PrimaryAssetId, FName InValueName = NAME_None)
		: PackageName(NAME_None), PrimaryAssetType(PrimaryAssetId.PrimaryAssetType), ObjectName(PrimaryAssetId.PrimaryAssetName), ValueName(InValueName)
	{}

	FExtAssetIdentifier(UObject* SourceObject, FName InValueName)
	{
		if (SourceObject)
		{
			UPackage* Package = SourceObject->GetOutermost();
			PackageName = Package->GetFName();
			ObjectName = SourceObject->GetFName();
			ValueName = InValueName;
		}
	}

	FExtAssetIdentifier()
		: PackageName(NAME_None), PrimaryAssetType(NAME_None), ObjectName(NAME_None), ValueName(NAME_None)
	{}

	/** Returns primary asset id for this identifier, if valid */
	FPrimaryAssetId GetPrimaryAssetId() const
	{
		if (PrimaryAssetType != NAME_None)
		{
			return FPrimaryAssetId(PrimaryAssetType, ObjectName);
		}
		return FPrimaryAssetId();
	}

	/** Returns true if this represents a package */
	bool IsPackage() const
	{
		return PackageName != NAME_None && !IsObject() && !IsValue();
	}

	/** Returns true if this represents an object, true for both package objects and PrimaryAssetId objects */
	bool IsObject() const
	{
		return ObjectName != NAME_None && !IsValue();
	}

	/** Returns true if this represents a specific value */
	bool IsValue() const
	{
		return ValueName != NAME_None;
	}

	/** Returns true if this is a valid non-null identifier */
	bool IsValid() const
	{
		return PackageName != NAME_None || GetPrimaryAssetId().IsValid();
	}

	/** Returns string version of this identifier in Package.Object::Name format */
	FString ToString() const
	{
		FString Result;
		if (PrimaryAssetType != NAME_None)
		{
			Result = GetPrimaryAssetId().ToString();
		}
		else
		{
			Result = PackageName.ToString();
			if (ObjectName != NAME_None)
			{
				Result += TEXT(".");
				Result += ObjectName.ToString();
			}
		}
		if (ValueName != NAME_None)
		{
			Result += TEXT("::");
			Result += ValueName.ToString();
		}
		return Result;
	}

	/** Converts from Package.Object::Name format */
	static FExtAssetIdentifier FromString(const FString& String)
	{
		// To right of :: is value
		FString PackageString;
		FString ObjectString;
		FString ValueString;

		// Try to split value out
		if (!String.Split(TEXT("::"), &PackageString, &ValueString))
		{
			PackageString = String;
		}

		// Check if it's a valid primary asset id
		FPrimaryAssetId PrimaryId = FPrimaryAssetId::FromString(PackageString);

		if (PrimaryId.IsValid())
		{
			return FExtAssetIdentifier(PrimaryId, *ValueString);
		}

		// Try to split on first . , if it fails PackageString will stay the same
		FString(PackageString).Split(TEXT("."), &PackageString, &ObjectString);

		return FExtAssetIdentifier(*PackageString, *ObjectString, *ValueString);
	}

	friend inline bool operator==(const FExtAssetIdentifier& A, const FExtAssetIdentifier& B)
	{
		return A.PackageName == B.PackageName && A.ObjectName == B.ObjectName && A.ValueName == B.ValueName;
	}

	friend inline uint32 GetTypeHash(const FExtAssetIdentifier& Key)
	{
		uint32 Hash = 0;

		// Most of the time only packagename is set
		if (Key.ObjectName.IsNone() && Key.ValueName.IsNone())
		{
			return GetTypeHash(Key.PackageName);
		}

		Hash = HashCombine(Hash, GetTypeHash(Key.PackageName));
		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetType));
		Hash = HashCombine(Hash, GetTypeHash(Key.ObjectName));
		Hash = HashCombine(Hash, GetTypeHash(Key.ValueName));
		return Hash;
	}

	/** Identifiers may be serialized as part of the registry cache, or in other contexts. If you make changes here you must also change FAssetRegistryVersion */
	friend FArchive& operator<<(FArchive& Ar, FExtAssetIdentifier& AssetIdentifier)
	{
		// Serialize bitfield of which elements to serialize, in general many are empty
		uint8 FieldBits = 0;

		if (Ar.IsSaving())
		{
			FieldBits |= (AssetIdentifier.PackageName != NAME_None) << 0;
			FieldBits |= (AssetIdentifier.PrimaryAssetType != NAME_None) << 1;
			FieldBits |= (AssetIdentifier.ObjectName != NAME_None) << 2;
			FieldBits |= (AssetIdentifier.ValueName != NAME_None) << 3;
		}

		Ar << FieldBits;

		if (FieldBits & (1 << 0))
		{
			Ar << AssetIdentifier.PackageName;
		}
		if (FieldBits & (1 << 1))
		{
			FName TypeName = AssetIdentifier.PrimaryAssetType.GetName();
			Ar << TypeName;

			if (Ar.IsLoading())
			{
				AssetIdentifier.PrimaryAssetType = TypeName;
			}
		}
		if (FieldBits & (1 << 2))
		{
			Ar << AssetIdentifier.ObjectName;
		}
		if (FieldBits & (1 << 3))
		{
			Ar << AssetIdentifier.ValueName;
		}

		return Ar;
	}
};

enum class EDependencyNodePackageType : uint8
{
	File,
	Script,
	Engine,
	Other,
};

enum class EDependencyNodeReferenceType : uint8
{
	Hard,
	Soft
};

enum class EDependencyNodeStatus : uint8
{
	Unknown,
	AbortGathering,

	Missing,
	Invalid,
	Valid,
	ValidWithSoftReferenceIssue
};

/**
 * Dependency info gathered for asset data
 */
struct FExtAssetDependencyInfo
{
	// ---------------- Source -----------------
	TArray<FString> ValidDependentFiles;
	TArray<FString> InvalidDependentFiles;
	TArray<FString> MissingDependentFiles;

	TArray<FString> SkippedPackageNames;
	TArray<FString> MissingPackageNames;

	TMap<FName, EDependencyNodeStatus> AllDepdencyPackages; // PakcaegName - DependencyStatus map, dependencies only (not include main asset)
	TMap<FName, TArray<FExtAssetDependencyNode>> AssetDepdencies; // Asset File Path - DependencyNode map
	EDependencyNodeStatus AssetStatus = EDependencyNodeStatus::Unknown;

	bool IsValid(bool bIgnoreSoftIssue = true);

	void Print();

#if ECB_WIP_CACHEDB
	friend FArchive& operator<<(FArchive& Ar, FExtAssetDependencyInfo& InDependencyInfo);
#endif

};

struct FExtAssetDependencyNode
{
	FExtAssetDependencyNode(const FName& InPackageName, EDependencyNodeStatus InNodeStatus, bool bSoftReference)
		: PackageName(InPackageName)
		, NodeStatus(InNodeStatus)
	{
		ReferenceType = bSoftReference ? EDependencyNodeReferenceType::Soft : EDependencyNodeReferenceType::Hard;
	}
	FExtAssetDependencyNode() {}

	FName PackageName = NAME_None;
	EDependencyNodeReferenceType ReferenceType = EDependencyNodeReferenceType::Hard;
	EDependencyNodeStatus NodeStatus = EDependencyNodeStatus::Valid;

	static FString GetStatusString(EDependencyNodeStatus InStatus);

	FString ToString() const;

#if ECB_WIP_CACHEDB
	friend FArchive& operator<<(FArchive& Ar, FExtAssetDependencyNode& InDependencyNode);
#endif
};

struct FExtAssetDependencyWalker
{
	static EDependencyNodeStatus GatherDependencies(const FExtAssetData& InAssetData, const FString& InAssetContentDir, const FExtAssetData::EContentType& InAssetContentType, FExtAssetDependencyInfo& InOuDependencyInfo, bool bShowProgess = false);

private:

	static EDependencyNodeStatus GatherDependencies(const FExtAssetData& InAssetData, const FString& InAssetContentDir, const FExtAssetData::EContentType& InAssetContentType, FExtAssetDependencyInfo& InOuDependencyInfo, int32 Level, FScopedSlowTask* SlowTaskPtr = nullptr);

	static EDependencyNodeStatus GatherPackageDependencies(const FName& DependPackageName, FExtAssetDependencyInfo& InOuDependencyInfo
		, const FString& InAssetContentDir, const FExtAssetData::EContentType& InAssetContentType
		, int32& Level, bool bSoftReference, FScopedSlowTask* SlowTaskPtr = nullptr);

};

struct FRootContentPathInfo
{
	FExtAssetData::EContentType ContentType;
	FString AssetContentRoot;
	FText DisplayName;

#if ECB_WIP_CACHEDB
	friend FArchive& operator<<(FArchive& Ar, FRootContentPathInfo& InDependencyInfo);
#endif
};

/**
 * Cache asset registry state in memory or disk, currently only in memory cache
 */
class FExtAssetRegistryStateCache
{
public:
	void Reset();

	void ClearThumbnailCache();

	void CacheNewAsset(FExtAssetData* InAssetToCache);

	void ReCacheParsedAsset(FExtAssetData* InAssetToCache);

	void UpdateCacheByRemovedFolders(const TArray<FString>& InRemovedFolders);

	bool PurgeAssets(bool bSilent = true);

	void PrintStatus() const;

	/** Returns memory size of entire registry, optionally logging sizes */
	//uint32 GetAllocatedSize(bool bLogDetailed = false) const;

#if ECB_WIP_CACHEDB
	template<class Archive>
	bool SerializeForCache(Archive&& Ar);

	int32 NumFoldersInCache = 0;
	int32 NumAssetsInCache = 0;
#endif
private:
#if ECB_WIP_CACHEDB
	bool Save(FArchive& Ar);
	bool Load(FArchive& Ar);

	template<class Archive>
	void Load(Archive&& Ar, FExtAssetRegistryVersion::Type Version);

	void SetAssetDatas(TArrayView<FExtAssetData> AssetDatas);
	void SerializeDependencyInfos(FArchive& Ar);
	void SerializeMisc(FArchive& Ar);

	TArray<FExtAssetData*> PreallocatedAssetDataBuffers;
#endif
public:

	/** Mirrored Root Content Paths Info Copied from ExtAssetRegistry */
	TArray<FString> CachedRootContentPaths;

	/** A one-to-many mapping between a parent path and its child paths*/
	TMap<FName, TSet<FName>> CachedSubPaths;

	/** The array of empty folders which contains no package recursively */
	TMap<FName, bool> CachedEmptyFoldersStatus;

	/** The set of folders have uasset been fully parsed */
	TSet<FName> CachedFullyParsedFolders;

	/** The map of long package folder path to asset data */
	TMap<FName, TArray<FExtAssetData*> > CachedAssetsByFolder;

	/** A one-to-many mapping between a folder to packages' file path in the folder */
	TMap<FName, TArray<FName>> CachedFilePathsByFolder;

	/** The map of asset file path to asset data */
	TMap<FName, FExtAssetData*> CachedAssetsByFilePath;

	/** Assets been cached but not yet parsed, subset of  CachedAssetsByFilePath */
	TMap<FName, FExtAssetData*> CachedUnParsedAssets;

	/** The map of asset file path to asset data */
	TMap<FName, FExtAssetDependencyInfo> CachedDependencyInfoByFilePath;

	/** Cached Asset Thumbnail data, could grows very fast when viewing large amount of assets */
	TMap<FName, FObjectThumbnail> CachedThumbnails; // todo: FIFO queue?

	/** The map of class name to asset data for assets saved to disk */
	TMap<FName, TArray<FExtAssetData*> > CachedAssetsByClass;

	/** The map of asset tag to asset data for assets saved to disk */
	TMap<FName, TArray<FExtAssetData*> > CachedAssetsByTag;

	/** The map of asset package name to file path */
	TMap<FName, FName> CachedPackageNameToFilePathMap;

	/** The array of asset content root dirs, sorted from longest to shortest */
	TArray<FName> CachedAssetContentRoots;
	TMap<FName, FExtAssetData::EContentType> CachedAssetContentType; // ContentRoot - ContentType
	TMap<FName, FName> CachedAssetContentRootHosts; // Host - ContentRoot
	TMap<FName, FName> CachedAssetContentRootConfigDirs; // ContentRoot - ConfigDir

	TMap<FName, TArray<FName>> CachedFolderColorIndices; // ContentRoot - Folders
	TMap<FName, FLinearColor> CachedFolderColors; // Folder - Color


private:
	void CacheValidAssetInfo(FExtAssetData* InAssetToCache);

};

struct FExtFolderGatherResult
{
	FExtFolderGatherResult(const FName& InRootPath, const FName& InParentPath,  const TArray<FName>& InSubPaths, const TMap<FName, TArray<FName>>& InRecurseSubPaths, const TMap<FName, TArray<FName>>& InFolderPackages)
		: RootPath(InRootPath)
		, ParentPath(InParentPath)
		, SubPaths(InSubPaths)
		, RecurseSubPaths(InRecurseSubPaths)
		, FolderPakages(InFolderPackages)
	{}
	FName RootPath;
	FName ParentPath;
	TArray<FName> SubPaths;
	TMap<FName, TArray<FName>> RecurseSubPaths;
	TMap<FName, TArray<FName>> FolderPakages;
};

struct FExtAssetContentRootGatherResult
{
	FExtAssetContentRootGatherResult(const FName& InAssetContentRootDir, const FName& InAssetContentRootHostDir, const FName& InAssetContentRootConfigDir, FExtAssetData::EContentType InAssetContentType)
		: AssetContentRoot(InAssetContentRootDir)
		, AssetContentRootHost(InAssetContentRootHostDir)
		, AssetContentRootConfigDir(InAssetContentRootConfigDir)
		, AssetContentType(InAssetContentType)
	{}
	FName AssetContentRoot;
	FName AssetContentRootHost;
	FName AssetContentRootConfigDir;
	FExtAssetData::EContentType AssetContentType;
};

struct FExtAssetGatherResult
{
	FExtAssetGatherResult()
	{}
	TMap<FName, FExtAssetData> ParsedAssets;

	void AddReuslt(const FName& InFilePath, const FExtAssetData& InParsedAsset);
	bool HasResult() const;
	void Reset();
	const TMap<FName, FExtAssetData>& GetParsedAssets() const { return ParsedAssets; };
};


/**
 * Gather and cache information about uasset files and paths
 */
class FExtAssetRegistry
{
public:

	///////////////////////////////////////////////
	// Events
	//
	DECLARE_EVENT_OneParam(FExtAssetRegistry, FPathAddedEvent, const FString& /*Path*/);
	FPathAddedEvent& OnRootPathAdded() { return RootPathAddedEvent; }

	DECLARE_EVENT_OneParam(FExtAssetRegistry, FPathRemovedEvent, const FString& /*Path*/);
	FPathRemovedEvent& OnRootPathRemoved() { return RootPathRemovedEvent; }

	DECLARE_EVENT(FExtAssetRegistry, FPathUpdatedEvent);
	FPathUpdatedEvent& OnRootPathUpdated() { return RootPathUpdatedEvent; }

	DECLARE_EVENT_OneParam(FExtAssetRegistry, FFolderStartGatheringEvent, const TArray<FString>& /*Paths*/);
	FFolderStartGatheringEvent& OnFolderStartGathering() { return FolderStartGatheringEvent; }

	DECLARE_EVENT_TwoParams(FExtAssetRegistry, FFolderFinishGatheringEvent, const FString& /*GatheredPath*/, const FString& /*RootGatheredPath*/);
	FFolderFinishGatheringEvent& OnFolderFinishGathering() { return FolderFinishGatheringEvent; }

	DECLARE_EVENT_TwoParams(FExtAssetRegistry, FAssetGatheredEvent, const FExtAssetData& /*GatheredAsset*/, int32 /*Left*/);
	FAssetGatheredEvent& OnAssetGathered() { return AssetGatheredEvent; }

	DECLARE_EVENT_OneParam(FExtAssetRegistry, FAssetUpdatedEvent, const FExtAssetData& /*UpdatedAssets*/);
	FAssetUpdatedEvent& OnAssetUpdated() { return AssetUpdatedEvent; }
	void BroadcastAssetUpdatedEvent(const FExtAssetData& /*UpdatedAssets*/);

	///////////////////////////////////////////////
	// Async Folder Gather
	//
	bool GetAndTrimFolderGatherResult();
	bool IsFolderBackgroundGathering(const FString& InFolder) const;
	FName GetCurrentGatheringFolder() const { return BackgroundGatheringSubFolder; }

	///////////////////////////////////////////////
	// Async Asset Gather
	//
	bool GetAndTrimAssetGatherResult();
	//bool IsFolderBackgroundGathering(const FString& InFolder) const;
	//FName GetCurrentGatheringFolder() const { return BackgroundGatheringSubFolder; }

private:
	/** The delegate to execute when an root content path is added to the registry */
	FPathAddedEvent RootPathAddedEvent;

	/** The delegate to execute when an root content path is removed from the registry */
	FPathRemovedEvent RootPathRemovedEvent;

	/** The delegate to execute when an root content path is removed from the registry */
	FPathUpdatedEvent RootPathUpdatedEvent;

	/** The delegate to execute when a folder path is start gathering from the registry */
	FFolderStartGatheringEvent FolderStartGatheringEvent;

	/** The delegate to execute when a folder path is finishing gathering from the registry */
	FFolderFinishGatheringEvent FolderFinishGatheringEvent;

	/** The delegate to execute when an asset is finishing gathering from the registry */
	FAssetGatheredEvent AssetGatheredEvent;

	FAssetUpdatedEvent AssetUpdatedEvent;

public:
	FExtAssetRegistry();
	virtual ~FExtAssetRegistry();

public:

	///////////////////////////////////////////////
	// IAssetRegistry compatible interface
	//

	/** Gets asset data for all assets that match the filter. Alias of GetOrCacheExtAssets */
	bool GetAssets(const struct FARFilter& InFilter, TArray<FExtAssetData>& OutAssetData);

	/** Trims items out of the asset data list that do not pass the supplied filter */
	void RunAssetsThroughFilter(TArray<FExtAssetData>& AssetDataList, const FARFilter& Filter) const;

	/**
	 * Gets a list of packages and searchable names that reference the supplied package or name. (On disk references ONLY)
	 *
	 * @param AssetIdentifier	the name of the package/name for which to gather dependencies
	 * @param OutReferencers	a list of things that reference AssetIdentifier
	 * @param InReferenceType	which kinds of reference to include in the output list
	 */
	bool GetReferencers(const FExtAssetIdentifier& AssetIdentifier, TArray<FExtAssetIdentifier>& OutReferencers, EAssetRegistryDependencyType::Type InReferenceType = EAssetRegistryDependencyType::All) const { return false; }

	/**
	 * Gets a list of packages that reference the supplied package. (On disk references ONLY)
	 *
	 * @param PackageName		the name of the package for which to gather dependencies
	 * @param OutReferencers	a list of packages that reference the package whose path is PackageName
	 * @param InReferenceType	which kinds of reference to include in the output list
	 */
	bool GetReferencers(FName PackageName, TArray<FName>& OutReferencers, EAssetRegistryDependencyType::Type InReferenceType = EAssetRegistryDependencyType::Packages) const { return false; }

	/**
	 * Gets a list of packages and searchable names that are referenced by the supplied package or name. (On disk references ONLY)
	 *
	 * @param AssetIdentifier	the name of the package/name for which to gather dependencies
	 * @param OutDependencies	a list of things that are referenced by AssetIdentifier
	 * @param InDependencyType	which kinds of dependency to include in the output list
	 */
	bool GetDependencies(const FExtAssetIdentifier& AssetIdentifier, TArray<FExtAssetIdentifier>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType = EAssetRegistryDependencyType::All);

	/**
	 * Gets a list of paths to objects that are referenced by the supplied package. (On disk references ONLY)
	 *
	 * @param PackageName		the name of the package for which to gather dependencies
	 * @param OutDependencies	a list of packages that are referenced by the package whose path is PackageName
	 * @param InDependencyType	which kinds of dependency to include in the output list
	 */
	bool GetDependencies(FName PackageName, TArray<FName>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType = EAssetRegistryDependencyType::Packages);

	////////////////////////////////
	// Asset gather progress update
		/** Payload data for a file progress update */
	struct FAssetGatherProgressUpdateData
	{
		FAssetGatherProgressUpdateData(int32 InNumTotalAssets, int32 InNumAssetsProcessedByAssetRegistry, int32 InNumAssetsPendingDataLoad, bool InIsDiscoveringAssetFiles)
			: NumTotalAssets(InNumTotalAssets)
			, NumAssetsProcessedByAssetRegistry(InNumAssetsProcessedByAssetRegistry)
			, NumAssetsPendingDataLoad(InNumAssetsPendingDataLoad)
			, bIsDiscoveringAssetFiles(InIsDiscoveringAssetFiles)
		{
		}

		int32 NumTotalAssets;
		int32 NumAssetsProcessedByAssetRegistry;
		int32 NumAssetsPendingDataLoad;
		bool bIsDiscoveringAssetFiles;
	};

	/** Event to update the progress of the background file load */
	DECLARE_EVENT_OneParam(FExtAssetRegistry, FAssetGatherProgressUpdatedEvent, const FAssetGatherProgressUpdateData& /*ProgressUpdateData*/);
	FAssetGatherProgressUpdatedEvent& OnAssetGatherProgressUpdated()
	{
		return AssetGatherProgressUpdatedEvent;
	}

	/** Returns true if the asset registry is currently gathering uasset files */
	bool IsGatheringAssets() const;

private:
	/** The delegate to execute while loading files to update progress */
	FAssetGatherProgressUpdatedEvent AssetGatherProgressUpdatedEvent;

	bool bIsGatheringAssets = false;
	int32 NumFilteredAssets = 0;
	int32 NumToGatherAssets = 0;

public:
	///////////////////////////////////////////////
	// AssetIdentifier interface
	//

	/** Returns valid PrimaryAssetId if this is a fake AssetData for a primary asset */
	static FPrimaryAssetId ExtractPrimaryAssetIdFromFakeAssetData(const FExtAssetData& InAssetData);

	static void ExtractAssetIdentifiersFromAssetDataList(const TArray<FExtAssetData>& AssetDataList, TArray<FExtAssetIdentifier>& OutAssetIdentifiers);

public:

	void Shutdown();

	/** Load and cache root content folders from config */
	void LoadRootContentPaths();

	/** Merge input paths to current root content folders */
	void MergeRootContentPathsWith(const TArray<FString>& InPathsToMerge, bool bReplaceCurrent);

	/** Replace current content folders with input */
	void ReplaceRootContentPathsWith(const TArray<FString>& InPaths);

	/** Save root content folders to config file */
	void SaveRootContentPaths();

	/** Get root content folders */
	void QueryRootContentPaths(TArray<FString>& OutRootContentPaths) const;

	/** Get root content folder from file path */
	bool QueryRootContentPathFromFilePath(const FString& InFilePath, FString& OutRootContentPath);

	/** Get root content folders */
	bool QueryRootContentPathInfo(const FString& InRootContentPath, FText* OutDisplayName = nullptr, FExtAssetData::EContentType* OutContentType = nullptr, FString* OutAssetContentRoot = nullptr) const;

	/** Get relative path to any root path, return false if not match any */
	bool ParseAssetContentRoot(const FString& InFilePath, FString& OutRelativePath, FString& OutRootPath, FExtAssetData::EContentType& OutAssetContentType, TSet<FName>* InAllDependencies = nullptr);

	/** Cache found asset content dir */
	void AddAssetContentRoot(const FString& InAssetContentRootDir, const FString& InAssetContentRootHostDir, const FString& ConfigDir, FExtAssetData::EContentType InContentType);

	/** Get cached asset content dir, return false if not found */
	bool GetAssetContentRoot(const FString& InFilePath, FString& OutFoundAssetContentDir) const;
	FExtAssetData::EContentType GetAssetContentRootContentType(const FString& InAssetContentRoot) const;

	/** Get cached folder color*/
	bool GetFolderColor(const FString& InFolderPath, FLinearColor& OutFolderColor) const;
	bool IsAsetContentRootHasFolderColor(const FString& InAssetContentRoot) const;
	void GetAssetContentRootColoredFolders(const FString& InAssetContentRoot, TArray<FName>& OutColoredFolders) const;
	void GetAssetContentRootFolderColors(const FString& InAssetContentRoot, TMap<FName, FLinearColor>& OutFoldersColor) const;

	/** If input path is root content path */
	bool IsRootFolder(const FString& InPath) const;

	/** If input path is root content path */
	bool IsRootFolders(const TArray<FString>& InPaths) const;

	/** Add root content folder */
	bool AddRootFolder(const FString& InPath, TArray<FString>* OutAdded = nullptr, TArray<FString>* OutCombined = nullptr);

	/** Reload root content folders */
	bool ReloadRootFolders(const TArray<FString>& InPaths, TArray<FString>* OutReloaded = nullptr);

	/** Remove root content folders */
	bool RemoveRootFolders(const TArray<FString>& InPaths, TArray<FString>* OutRemovd = nullptr);

	/** Rescan root content folders */
	void ReGatheringFolders(const TArray<FString>& InPaths);

	/** Cache asset data for all assets that match the filter. */
	void CacheAssets(const struct FARFilter& InFilter);

	/** Kick off aysnc cache asset data for all assets that match the filter. */
	void CacheAssetsAsync(const struct FARFilter& InFilter);

	/** Gets a list of all paths that are currently cached below the passed-in base path. */
	void GetOrCacheSubPaths(const FName& InBasePath, TSet<FName>& OutPathList, bool bInRecurse);

	/** Gets a list of all paths that are currently cached below the passed-in base path. */
	void GetCachedSubPaths(const FName& InBasePath, TSet<FName>& OutPathList, bool bInRecurse);

	/** Get or cache asset dependency info. */
	FExtAssetDependencyInfo GetOrCacheAssetDependencyInfo(const FExtAssetData& InAssetData, bool bShowProgess = false);

	/** Get cached asset dependency info. */
	const FExtAssetDependencyInfo* GetCachedAssetDependencyInfo(const FExtAssetData& InAssetData) const;
	bool RemoveCachedAssetDependencyInfo(const FExtAssetData& InAssetData);

	bool IsCachedDependencyInfoInValid(const FExtAssetData& InAssetData, bool bTreatSoftErrorAsInvalid = false) const;

	/** Get or cache an assets by file path. */
	FExtAssetData* GetOrCacheAssetByFilePath(const FName& InFilePath, bool bDelayParse = false);
	
	/** Gets assets by directory, cache them if not already. */
	TMap<FName, TArray<FExtAssetData*>>& GetOrCacheAssetsByFolder(const FName& InDirectory, bool bDelayParse = false);

	/** Gets assets by asset class, cache them if not already. */
	TMap<FName, TArray<FExtAssetData*>>& GetOrCacheAssetsByClass(const TSet<FName>& InPaths, bool bRecursively);

	/** Get cached asset by file path. */
	FExtAssetData* GetCachedAssetByFilePath(const FName& InFilePath);

	/** Get cached asset by package name. */
	FExtAssetData* GetCachedAssetByPackageName(const FName& InPackageName);

	/** Gets cached assets by directory. */
	TArray<FExtAssetData*> GetCachedAssetsByFolder(const FName& InDirectory);

	/** Gets cached assets by asset class. */
	TArray<FExtAssetData*> GetCachedAssetsByClass(const FName& InClass);

	const TMap<FName, FName>& GetCachedAssetContentRootConfigDirs() const;
	const TMap<FName, FName>& GetCachedAssetContentRootHostDirs() const;

#if ECB_WIP_THUMB_CACHE
	/** Searches for an object's thumbnail in memory and returns it if found */
	const FObjectThumbnail* FindCachedThumbnail(const FName& InFilePath);
#endif

	/** If input folder has any packages. */
	bool IsEmptyFolder(const FString& FolderPath);

	/** If assets in the input folder been fully parsed. */
	bool IsFolderAssetsFullyParsed(const FString& FolderPath) const;

	/** Checks a filter to make sure there are no illegal entries */
	bool IsFilterValid(const FARFilter& Filter, bool bAllowRecursion);

	/** Modifies passed in filter to make it safe for use on FAssetRegistryState. This expands recursive paths and classes */
	void ExpandRecursiveFilter(const FARFilter& InFilter, FARFilter& ExpandedFilter);

	/** Helper functions to extract asset import information from asset registry tags */
	TOptional<FAssetImportInfo> ExtractAssetImportInfo(const FExtAssetData& AssetData) const;

	void PrintCacheStatus();

	/** Clear cached assets and paths */
	void ClearCache();

	void UpdateCacheByRemovedFolders(const TArray<FString>& InRemovedFolders);

	void StartFolderGathering(const FString& InParentFolder);
	void StartOrCancelAssetGathering(const TArray<FName>& InAssetPaths, int32 TotoalFiltered);

public:
#if ECB_WIP_CACHEDB
	bool LoadCacheDBWithFilePicker();
	bool LoadCacheDB(bool bSilent = true);
	bool SaveCacheDB(bool bSilent = true);
	bool PurgeCacheDB(bool bSilent = true);

	void SwitchCacheMode();

	int32 GetNumFoldersInCache() const { return State.NumFoldersInCache; }
	int32 GetNumAssetsInCache() const { return State.NumAssetsInCache; }
	int32 GetNumFoldersInMem() const { return State.CachedSubPaths.Num(); }
	int32 GetNumAssetsInMem() const { return State.CachedAssetsByFilePath.Num(); }
#endif

private:

	bool CombineRootContentPaths();
	void CacheRootContentPathInfo();
	void CacheFolderColor(const FName& InAssetContentRootDir);

	/** Returns the names of all subclasses of the class whose name is ClassName */
	void GetSubClasses(const TArray<FTopLevelAssetPath>& InClassPaths, const TSet<FTopLevelAssetPath>& ExcludedClassNames, TSet<FTopLevelAssetPath>& SubClassNames) const;
	void GetSubClasses_Recursive(const FTopLevelAssetPath& InClassName, TSet<FTopLevelAssetPath>& SubClassNames, TSet<FTopLevelAssetPath>& ProcessedClassNames, const TMap<FTopLevelAssetPath, TSet<FTopLevelAssetPath>>& ReverseInheritanceMap, const TSet<FTopLevelAssetPath>& ExcludedClassNames) const;

	void StopAllBackgroundGathering();
	
private:

	/** The array of root content folders */
	TArray<FString> RootContentPaths;

	TMap<FString, FRootContentPathInfo> RootContentPathsInfo;

	/** Asset data and folder caches */
	FExtAssetRegistryStateCache State;

	/** Folder Gathering */
	TSharedPtr<class FExtFolderGatherer> BackgroundFolderGatherer;
	TSet<FName> BackgroundGatheringFolders;
	FName BackgroundGatheringSubFolder = NAME_None;

	/** Asset Gathering */
	TSharedPtr<class FExtAssetGatherer> BackgroundAssetGatherer;
};

/**
 * Helpers to export root content paths
 */
struct FRootContentPathsExporter
{
	//static void ImportAssets(const TArray<FExtAssetData>& InAssetDatas, const FExtAssetImportSetting& ImportSetting);
};

/**
 * Helpers to import uasset files into current project
 */
struct FExtAssetImporter
{
	static void ImportAssets(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& ImportSetting);
	static void ImportAssetsByFilePaths(const TArray<FString>& InAssetFilePaths, const FUAssetImportSetting& ImportSetting);
	static void ImportAssetsToFolderPackagePath(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& ImportSetting, const FString& InDestPackagePath);
	static void ImportAssetsWithPathPicker(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& ImportSetting);

	static void ImportProjectFolderColors(const FString& InRootOrHost);

	static void ExportAssets(const TArray<FExtAssetData>& InAssetDatas);

	static void ZipUpSourceAssets(const TArray<FExtAssetData>& InAssetDatas);

	static bool IsValidImportToPluginName(const FName& InPluginName, FString* OutInValidReason = nullptr, FString* OutValidPluginContentDir = nullptr);

private:
	static void DoImportAssets(const TArray<FExtAssetData>& InAssetDatas, const FUAssetImportSetting& ImportSetting);
	static bool ValidateImportSetting(const FUAssetImportSetting& ImportSetting);
};

/**
 * Helpers to validate a uasset file
 */
struct FExtAssetValidator
{
	static bool ValidateDependency(const TArray<FExtAssetData*>& InAssetDatas, FString* OutValidateResultPtr = nullptr, bool bShowProgess = false, EDependencyNodeStatus* OutAssetStatus = nullptr);
	static bool ValidateDependency(const TArray<FExtAssetData>& InAssetDatas, FString* OutValidateResultPtr = nullptr, bool bShowProgess = false, EDependencyNodeStatus* OutAssetStatus = nullptr);
	static void InValidateDependency(const TArray<FExtAssetData>& InAssetDatas);
};

/**
 * Helpers count asset data 
 */
struct FExtAssetCountInfo
{
	int32 TotalAssetsIncludeUMap;
	int32 TotalUAsset;
	int32 TotalUMap;
	int32 TotalInvalid;
	int32 TotalFileSizes;
};

struct FExtAssetCounter
{
	static int32 CountAssetsByFolder(const FString& InFolder, FExtAssetCountInfo& OutCountInfo);
};

/**
 *  Folder gathering policy
 */
struct FExtFolderGatherPolicy
{
	struct FFolderVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				bool bShouldIgnore = false;
				if (bIgnoreFoldersStartWithDot || IgnoreFolders.Num() > 0)
				{
					FString PathLeft = FPaths::GetPathLeaf(FilenameOrDirectory);

					if (bIgnoreFoldersStartWithDot && PathLeft.StartsWith(TEXT(".")))
					{
						bShouldIgnore = true;
					}

					if (!bShouldIgnore && IgnoreFolders.Num() > 0)
					{
						for (const FString& IgnoreFolder : IgnoreFolders)
						{
							if (IgnoreFolder.Equals(PathLeft, ESearchCase::IgnoreCase))
							{
								bShouldIgnore = true;
								break;
							}
						}
					}
				}

				if (!bShouldIgnore)
				{
					Directories.Add(FilenameOrDirectory);
				}
			}
			else if (FExtAssetSupport::IsSupportedPackageFilename(FilenameOrDirectory))
			{
				PackageFiles.Add(FilenameOrDirectory);
			}
#if ECB_WIP // Gather Content Dir
			else
			{

			}
#endif
			return true;
		}

		void Reset()
		{
			Directories.Reset();
			PackageFiles.Reset();
		}

		FFolderVisitor()
		{
		}

		FFolderVisitor(bool bInIgnoreFoldersStartWithDot, const TArray<FString>& InIgnoreFolders)
			: bIgnoreFoldersStartWithDot(bInIgnoreFoldersStartWithDot)
			, IgnoreFolders(InIgnoreFolders)
		{
		}

		TArray<FName> Directories;
		TArray<FName> PackageFiles;

		bool bIgnoreFoldersStartWithDot = false;
		TArray<FString> IgnoreFolders;
	};

	FFolderVisitor& GetResetedFolderVisitor()
	{
		FolderVisitor.Reset();
		return FolderVisitor;
	}

	void UpdateWith(const FExtFolderGatherPolicy& InNewPolicy)
	{
		FolderVisitor.bIgnoreFoldersStartWithDot = InNewPolicy.IsIgnoreFoldersStartWithDot();
		FolderVisitor.IgnoreFolders = InNewPolicy.GetIgnoreFolders();
	}

	bool IsIgnoreFoldersStartWithDot() const 
	{
		return FolderVisitor.bIgnoreFoldersStartWithDot;
	}

	const TArray<FString>& GetIgnoreFolders() const
	{
		return FolderVisitor.IgnoreFolders;
	}

	FExtFolderGatherPolicy(bool bInIgnoreFoldersStartWithDot, const TArray<FString>& InIgnoreFolders)
		: FolderVisitor(bInIgnoreFoldersStartWithDot, InIgnoreFolders)
	{
	}

	FExtFolderGatherPolicy() {}

private:
	FFolderVisitor FolderVisitor;
};

/**
 *  Gather folders recursively
 */
class FExtFolderGatherer : public FRunnable
{
public:
	FExtFolderGatherer(const FString& InGatheringFolder);
	FExtFolderGatherer(const FString& InGatheringFolder, const FExtFolderGatherPolicy& InGatherPolicy);
	virtual ~FExtFolderGatherer();

	// FRunnable implementation
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	void EnsureCompletion();

	void AddSearchFolder(const FString& InFolderToSearch, const FExtFolderGatherPolicy* InUpdatedGatherPolicy);

	void StopSearchFolder(const FString& InFolderToStopSearch);

	bool GetAndTrimGatherResult(TArray<FExtFolderGatherResult>& OutGatherResults, double& OutGatherTime, TArray<FExtFolderGatherResult>& OutSubDirGatherResults, TArray<FExtAssetContentRootGatherResult>& OutAssetContentRootDirGatherResults);

private:
	FExtFolderGatherPolicy GatherPolicy;

private:
	struct FPathTree
	{
		FPathTree(const FName& InParent, const FName& InFolder, bool bInEmpty)
			: Parent(InParent)
			, Folder(InFolder)
			, bEmptyFolder(bInEmpty)
		{}
		FName Parent;
		FName Folder;
		bool bEmptyFolder;

		void MarkPathTreeNotEmpty(TMap<FName, FPathTree>& PathTrees)
		{
			bEmptyFolder = false;
			for (FPathTree* ParentTreePtr = PathTrees.Find(Parent); ParentTreePtr; ParentTreePtr = PathTrees.Find(ParentTreePtr->Parent))
			{
				ParentTreePtr->bEmptyFolder = false;
			}
		}
	};

	/** Get all child directories of input base directory */
	void GetDirectoriesAndPackages(const FName& InBaseDirectory, TArray<FName>& OutDirList, TArray<FName>& OutFilePaths);

	/** Get all child directories and packages of input base directory, return false if skipped */
	bool GetAllDirectoriesAndPackagesRecursively(const FName& InBaseDirectory, TArray<FName>& OutSubPaths, TMap<FName, TArray<FName>>& OutRecusriveSubPaths, TMap<FName, TArray<FName>>& OutFilePaths);

	void GetChildDirectoriesAndPackages(const FName& InRoot, const FName& InParent, const TArray<FName>& InBaseDirs, TArray<FName>& OutSubPaths, TMap<FName, TArray<FName>>& OutRecusriveSubPaths, TMap<FName, TArray<FName>>& OutputFilePaths, TMap<FName, FPathTree>& OutputPathTrees, bool bFoundAssetContentRoot = false);

private:
	TArray<FString> DirectoriesToSearch;

	TArray<FExtFolderGatherResult> RootDirGatherResults;
	TArray<FExtFolderGatherResult> SubDirGatherResults;
	TArray<FExtAssetContentRootGatherResult> AssetContentRootGatherResults;

	/** How many seconds spent on last gathering */
	double GatherTime;

	/** A critical section to protect data transfer to other threads */
	FCriticalSection WorkerThreadCriticalSection;

	/** Thread to run the cleanup FRunnable on */
	FRunnableThread* Thread;

	/** Signal to stop */
	bool bRequestToStop;

	/** Signal to skip current search folder */
	bool bRequestToSkipCurrentSearchFolder;

	/** Current searching folder */
	FString CurrentSearchFolder;
};

/**
 *  Gather folders recursively
 */
class FExtAssetGatherer : public FRunnable
{
public:
	FExtAssetGatherer();
	virtual ~FExtAssetGatherer();

	// FRunnable implementation
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	void EnsureCompletion();

	void AddBatchToGather(const TArray<FName>& InBatch);

	void CancelGather();

	bool GetAndTrimGatherResult(FExtAssetGatherResult& OutGatherResults, double& OutGatherTime, int32& OutLeft);

private:
	bool DoGatherAssetByFilePath(const FName& InFilePath, FExtAssetData& OutExtAssetData) const;

private:
	TArray<FName> GatherBatch;

	FExtAssetGatherResult GatherResult;

	/** How many seconds spent on last gathering */
	double GatherStartTime;
	double GatherTime;

	/** A critical section to protect data transfer to other threads */
	FCriticalSection WorkerThreadCriticalSection;

	/** Thread to run the cleanup FRunnable on */
	FRunnableThread* Thread;

	/** Signal to stop */
	bool bRequestToStop;
};


////////////////////////////////////////////////
// FExtContentDirFinder
//
struct FExtContentDirFinder
{
	static bool FindWithFile(const FString& InUAssetFilePath, const FString& InFileToFind, bool bExtension, const FString& InContentFolderPattern, FString& OutContentRootPath, FString& OutRelativePath);

	static bool FindFolder(const FString& InUAssetFilePath, const FString& InContentFolderPattern, FString& OutContentRootPath, FString& OutRelativePath);

	static bool FindWithFolder(const FString& InFolderPath, const FString& InFileToFind, bool bExtension);
};

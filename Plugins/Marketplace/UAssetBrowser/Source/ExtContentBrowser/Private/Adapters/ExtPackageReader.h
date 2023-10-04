// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/ArchiveUObject.h"
#include "UObject/ObjectResource.h"
#include "UObject/PackageFileSummary.h"
#include "UObject/Linker.h"
#include "AssetRegistry/AssetData.h"

struct FExtAssetData;
class FExtPackageDependencyData;
class FObjectThumbnail;

/**
 * Class for read and parse package file
 */
class FExtPackageReader : public FArchiveUObject
{
public:
	FExtPackageReader();
	~FExtPackageReader();

	enum class EOpenPackageResult : uint8
	{
		Success,
		NoLoader,
		MalformedTag,
		VersionTooOld,
		VersionTooNew,
		CustomVersionMissing,
		FailedToLoad
	};

	/** Creates a loader for the filename */
	bool OpenPackageFile(const FString& PackageFilename, EOpenPackageResult* OutErrorCode = nullptr);
	bool OpenPackageFile(FArchive* Loader, EOpenPackageResult* OutErrorCode = nullptr);
	bool OpenPackageFile(EOpenPackageResult* OutErrorCode = nullptr);

	/** Reads information from the asset registry data table and converts it to FAssetData */
	bool ReadAssetRegistryData(TArray<FAssetData*>& AssetDataList);

	/** Attempts to get the class name of an object from the thumbnail cache for packages older than VER_UE4_ASSET_REGISTRY_TAGS */
	bool ReadAssetDataFromThumbnailCache(TArray<FAssetData*>& AssetDataList);

	/** Creates asset data reconstructing all the required data from cooked package info */
	bool ReadAssetRegistryDataIfCookedPackage(TArray<FAssetData*>& AssetDataList, TArray<FString>& CookedPackageNamesWithoutAssetData);

	/** Reads information used by the dependency graph */
	bool ReadDependencyData(FExtPackageDependencyData& OutDependencyData);

	/** Serializers for different package maps */
	bool SerializeNameMap();
	bool SerializeImportMap();
	bool SerializeExportMap();
	bool SerializeSoftPackageReferenceList(TArray<FName>& OutSoftPackageReferenceList);
	bool SerializeSearchableNamesMap(FExtPackageDependencyData& OutDependencyData);

	/** Returns flags the asset package was saved with */
	uint32 GetPackageFlags() const;

	// Farchive implementation to redirect requests to the Loader
	void Serialize( void* V, int64 Length );
	bool Precache( int64 PrecacheOffset, int64 PrecacheSize );
	void Seek( int64 InPos );
	int64 Tell();
	int64 TotalSize();
	FArchive& operator<<( FName& Name );
	virtual FString GetArchiveName() const override
	{
		return PackageFilename;
	}

	//////////////////////////////////
	// For FExtAssetData 
	const FPackageFileSummary& GetPackageFileSummary() const;
	int32 GetSoftPackageReferencesCount() const;
	/** Reads information from the asset registry data table and converts it to FAssetData */
	bool ReadAssetRegistryData(FExtAssetData& OutAssetData);
	/** Attempts to get the class name of an object from the thumbnail cache for packages older than VER_UE4_ASSET_REGISTRY_TAGS */
	bool ReadAssetDataFromThumbnailCache(FExtAssetData& OutAssetData);
	/** Reads information used by the dependency graph */
	bool ReadDependencyData(FExtAssetData& OutAssetData);
	bool ReadThumbnail(FObjectThumbnail& OutThumbnail);

private:
	bool StartSerializeSection(int64 Offset);

	FString PackageFilename;
	FArchive* Loader;
	FPackageFileSummary PackageFileSummary;
	TArray<FName> NameMap;
	TArray<FObjectImport> ImportMap;
	TArray<FObjectExport> ExportMap;
	int64 PackageFileSize;
};

/**
 * Support class for gathering package dependency data
 */
class FExtPackageDependencyData : public FLinkerTables
{
public:
	/** The name of the package that dependency data is gathered from */
	FName PackageName;

	/** Asset Package data, gathered at the same time as dependency data */
	FAssetPackageData PackageData;

	/**
	 * Return the package name of the UObject represented by the specified import.
	 *
	 * @param	PackageIndex	package index for the resource to get the name for
	 *
	 * @return	the path name of the UObject represented by the resource at PackageIndex, or the empty string if this isn't an import
	 */
	FName GetImportPackageName(int32 ImportIndex);

	/**
	 * Serialize as part of the registry cache. This is not meant to be serialized as part of a package so  it does not handle versions normally
	 * To version this data change FAssetRegistryVersion or CacheSerializationVersion
	 */
	void SerializeForCache(FArchive& Ar)
	{
		Ar << PackageName;
		//Ar << ImportMap;
		Ar << SoftPackageReferenceList;
		Ar << SearchableNamesMap;

		PackageData.SerializeForCache(Ar);
	}
};

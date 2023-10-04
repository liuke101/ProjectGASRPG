// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtAssetData.h"

#include "UObject/ObjectMacros.h"
#include "Misc/ObjectThumbnail.h"

struct FExtAssetData;

/**
 * Helper functions to deal with packages and directories
 */
struct FExtPackageUtils
{
public:

	/** Parse and gather external asset data */
	static bool ParsePackageWithPackageReader(FExtAssetData& OutExtAssetData);

	/** Load thumbnail from external asset */
	static bool LoadThumbnailWithPackageReader(const FExtAssetData& InExtAssetData, FObjectThumbnail& InOutThumbnail);

#if ECB_LEGACY

	/** Parse and gather external asset data */
	static bool ParsePackage(FExtAssetData& OutExtAssetData);

	/** Load thumbnail from external asset */
	static bool LoadThumbnail(const FExtAssetData& InExtAssetData, FObjectThumbnail& OutThumbnail);

	/** Load soft references */
	static void LoadSoftReferences(const FExtAssetData& InExtAssetData, TArray<FName>& OutSoftPackageReferenceList);

#endif

	/** Unload package by file path */
	static void UnloadPackage(const TArray<FString>& InFilePaths, TArray<UPackage*>* OutUnloadPackage = nullptr);

	/** Get all child directories of input base directory */
	static void GetDirectories(const FString& InBaseDirectory, TArray<FString>& OutPathList);

	/** Get all child directories of input base directory */
	static void GetDirectoriesRecursively(const FString& InBaseDirectory, TArray<FString>& OutPathList);

	/** Get all packages' full file path in input root directory, recursively if asked */
	static void GetAllPackages(const FString& InRootDirectory, TArray<FString>& OutPathList, bool bResursively);

	/** Get all packages' full file path in input root directory, recursively if asked */
	static void GetAllPackages(const FName& InRootDirectory, TArray<FName>& OutPathList);

	/** Check if input directory contains any valid package */
	static bool HasPackages(const FString& InDirectory, bool bResursively);

	/** Check if a package exist */
	static bool DoesPackageExist(const FString PackageName);

private:

	/**	Writes information about the linker to the log. */
	static void GeneratePackageReport(class FLinkerLoad* InLinker, uint32 InInfoFlags, bool bInHideOffsets);

#if ECB_LEGACY

	/** Loads thumbnails from the specified package file name */
	static bool LoadThumbnailsFromPackage(const FString& InPackageFileName, const TSet< FName >& InObjectFullNames, FThumbnailMap& InOutThumbnails);

	/** Loads thumbnails from a package unless they're already cached in that package's thumbnail map */
	static bool ConditionallyLoadThumbnailsFromPackage(const FString& InPackageFileName, const TSet< FName >& InObjectFullNames, FThumbnailMap& InOutThumbnails);
#endif
};

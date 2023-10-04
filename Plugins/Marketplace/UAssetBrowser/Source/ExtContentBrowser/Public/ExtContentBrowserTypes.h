// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ExtContentBrowserTypes.generated.h"

/**
 * Asset Import Setting
 */
USTRUCT(BlueprintType)
struct FUAssetImportSetting
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category="Import", DisplayName="Abort If Dependency Missing")
	bool bSkipImportIfAnyDependencyMissing = true;

	//#if ECB_FEA_IMPORT_IGNORE_SOFT_ERROR
	UPROPERTY(BlueprintReadWrite, Category = "Import")
	bool bIgnoreSoftReferencesError = true;
	//#endif

	UPROPERTY(BlueprintReadWrite, Category = "Import", DisplayName="Overwrite Existing Assets")
	bool bOverwriteExistingFiles = false;

	UPROPERTY(BlueprintReadWrite, Category = "Import")
	bool bRollbackIfFailed = true;

	//#if ECB_WIP_IMPORT_FOLDER_COLOR
	UPROPERTY(BlueprintReadWrite, Category = "Folder")
	bool bImportFolderColor = false;

	UPROPERTY(BlueprintReadWrite, Category = "Folder")
	bool bOverrideExistingFolderColor = false;
	//#endif

	//#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
	UPROPERTY(BlueprintReadWrite, Category = "Plugin")
	bool bImportToPluginFolder = false;

	UPROPERTY(BlueprintReadWrite, Category = "Plugin")
	bool bWarnBeforeImportToPluginFolder = true;

	UPROPERTY(BlueprintReadWrite, Category = "Plugin")
	FName ImportToPluginName = NAME_None;
	//#endif

	UPROPERTY(BlueprintReadWrite, Category = "PostImport", DisplayName = "Sync Imporrted Assets in Content Browser")
	bool bSyncAssetsInContentBrowser = true;

	UPROPERTY(BlueprintReadWrite, Category = "PostImport", DisplayName = "Sync Skipped Existing Assets")
	bool bSyncExistingAssets = true;

	UPROPERTY(BlueprintReadWrite, Category = "PostImport", DisplayName = "Load/Reload Assets After Import")
	bool bLoadAssetAfterImport = true;

	//#if ECB_WIP_IMPORT_ADD_TO_COLLECTION
	UPROPERTY(BlueprintReadWrite, Category = "Collection")
	bool bAddImportedAssetsToCollection = false;

	UPROPERTY(BlueprintReadWrite, Category = "Collection")
	bool bUniqueCollectionNameForEachImportSession = false;

	UPROPERTY(BlueprintReadWrite, Category = "Collection")
	FName ImportedUAssetCollectionName = NAME_None;
	//#endif

	FString TargetContentDir;
	bool bSandboxMode = false;
	bool bFlatMode = false;
	bool bDirectCopyMode = false;

	// Place Imported Assets to LevelEditor
	bool bPlaceImportedAssets = false;

	bool bDumpAsset = false; // import, dump, forget

	bool bSilentMode = false;

	bool bExportMode = false;

	FUAssetImportSetting();
	FUAssetImportSetting(const FString& InTargetContentDir);

	static FUAssetImportSetting& GetDefaultImportSetting()
	{
		static FUAssetImportSetting DefaultSetting;
		return DefaultSetting;
	}

	static FUAssetImportSetting GetDefaultExportSetting(const FString& InTargetDir);

	static FUAssetImportSetting GetZipExportSetting();

	static FUAssetImportSetting GetSavedImportSetting();

	static FUAssetImportSetting GetSandboxImportSetting();
	static FUAssetImportSetting GetSandboxImportForDumpSetting();

	static FUAssetImportSetting GetFlatModeImportSetting(); // import to any folder and flatten file paths

	static FUAssetImportSetting GetDirectCopyModeImportSetting(); // direct copy without dependency gathering

	bool IsValid() const
	{
		return bValid;
	}

	const FString& GetInvalidReason() const
	{
		return InValidReason;
	}

	void ResolveTargetContentDir();

private:
	bool bValid = true;
	FString InValidReason;
};


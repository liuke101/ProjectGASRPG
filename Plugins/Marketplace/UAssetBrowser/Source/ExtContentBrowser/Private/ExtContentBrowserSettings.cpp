// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtContentBrowserSettings.h"
#include "ExtContentBrowser.h"
#include "ExtAssetData.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Model.h"
#include "EngineGlobals.h"
#include "UnrealWidget.h"
#include "EditorModeManager.h"
#include "UnrealEdMisc.h"
#include "CrashReporterSettings.h"
#include "Misc/ConfigCacheIni.h" // for FConfigCacheIni::GetString()


/////////////////////////////////////////////////////////////
// UExtContentBrowserSettings implementation
//

UExtContentBrowserSettings::FSettingChangedEvent UExtContentBrowserSettings::SettingChangedEvent;

UExtContentBrowserSettings::UExtContentBrowserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)

	, NumObjectsToLoadBeforeWarning(0)
	, bOpenSourcesPanelByDefault(true)

	, SearchAndFilterRecursively(true)
	, UseStraightLineInDependencyViewer(true)
	, ShowDependencyViewerUnderAssetView(false)

	, NumObjectsInRecentList(10)
	, bShowFullCollectionNameInToolTip(true)
{
	ResetCacheSettings();
	ResetThumbnailPoolSettings();
	ResetImportSettings();
	ResetExportSettings();

	ResetViewSettings();

	CommonNonContentFolders = {
		TEXT("Binaries"),
		TEXT("Config"),
		TEXT("DerivedDataCache"),
		TEXT("Intermediate"),
		TEXT("Resources"),
		TEXT("Saved"),
		TEXT("Source")
	};

	ExternalContentFolders = {
		TEXT("__ExternalActors__"),
		TEXT("__ExternalObjects__")
	};
}

void UExtContentBrowserSettings::ResetCacheSettings()
{
	bCacheMode = false;
	CacheModeBorderColor = FLinearColor(1.0f, 0.8f, 0.f, .6f);
	bAutoSaveCacheOnExit = true;
	bAutoSaveCacheOnSwitchToCacheMode = false;
	bKeepCachedAssetsWhenRootRemoved = true;
	bShowCacheStatusBarInLiveMode = true;
	CacheFilePath.FilePath = FPaths::ProjectSavedDir() / "UAssetBrowser.cachedb";
}

void UExtContentBrowserSettings::ResetThumbnailPoolSettings()
{
#if ECB_WIP_OBJECT_THUMB_POOL
	bUseThumbnailPool = true;
#else
	bUseThumbnailPool = false;
#endif
	NumThumbnailsInPool = 1024;
}

void UExtContentBrowserSettings::ResetImportSettings()
{
	bSkipImportIfAnyDependencyMissing = true;
	bImportOverwriteExistingFiles = false;
	bRollbackImportIfFailed = true;
	bImportSyncAssetsInContentBrowser = true;
	bImportSyncExistingAssets = true;
	bLoadAssetAfterImport = true;

	bAddImportedAssetsToCollection = false;
	bUniqueCollectionNameForEachImportSession = false;
	DefaultImportedUAssetCollectionName = FExtAssetContants::DefaultImportedUAssetCollectionName;

	bImportToPluginFolder = false;
	bWarnBeforeImportToPluginFolder = true;
	ImportToPluginName = NAME_None;

	bImportFolderColor = false;
	bOverrideExistingFolderColor = false;

	bImportIgnoreSoftReferencesError = true;
}

void UExtContentBrowserSettings::ResetExportSettings()
{
	bSkipExportIfAnyDependencyMissing = false;
	bExportIgnoreSoftReferencesError = true;
	bExportOverwriteExistingFiles = true;
	bOpenFolderAfterExport = true;
}

void UExtContentBrowserSettings::ResetViewSettings()
{
	DisplayFolders = true;
	DisplayEmptyFolders = true;
	DisplayAssetTooltip = true;
	DisplayEngineVersionOverlay = false;
#if ECB_FEA_VALIDATE_OVERLAY
	DisplayValidationStatusOverlay = true;
#else
	DisplayValidationStatusOverlay = false;
#endif
#if ECB_WIP_CONTENT_TYPE_OVERLAY
	DisplayContentTypeOverlay = false;
#else
	DisplayContentTypeOverlay = false;
#endif

	DisplayInvalidAssets = false;
	DisplayToolbarButton = true;

	bIgnoreFoldersStartWithDot = true;
	bIgnoreCommonNonContentFolders = true;
	bIgnoreExternalContentFolders = true;
	bIgnoreMoreFolders = false;
}

void UExtContentBrowserSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}

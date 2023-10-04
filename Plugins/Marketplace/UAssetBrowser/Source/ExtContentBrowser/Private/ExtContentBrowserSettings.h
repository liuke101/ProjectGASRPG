// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "ExtContentBrowserSettings.generated.h"

/**
 * Implements external content browser settings.  These are global not per-project
 */
UCLASS(config=EditorSettings)
class UExtContentBrowserSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** The number of objects to load at once in the Content Browser before displaying a warning about loading many assets */
	UPROPERTY(EditAnywhere, config, Category=ContentBrowser, meta=(DisplayName = "Assets to Load at Once Before Warning", ClampMin = "1"))
	int32 NumObjectsToLoadBeforeWarning;

	/** Whether the Content Browser should open the Sources Panel by default */
	UPROPERTY(EditAnywhere, config, Category = ContentBrowser)
	bool bOpenSourcesPanelByDefault;

	/** Whether to display folders in the asset view of the content browser. Note that this implies 'Show Only Assets in Selected Folders'. */
	UPROPERTY(config)
	bool DisplayFolders;

	/** Whether to empty display folders in the asset view of the content browser. */
	UPROPERTY(config)
	bool DisplayEmptyFolders;

	/** Whether to display tooltip in the asset view. */
	UPROPERTY(config)
	bool DisplayAssetTooltip;

	/** Whether to empty engine version as asset thumbnail overlay */
	UPROPERTY(config)
	bool DisplayEngineVersionOverlay;

	/** Whether to display asset validation result as overlay. */
	UPROPERTY(config)
	bool DisplayValidationStatusOverlay;

	/** Whether to empty content type as asset thumbnail overlay */
	UPROPERTY(config)
	bool DisplayContentTypeOverlay;

	/** Whether to show invalid assets in the asset view of the content browser. */
	UPROPERTY(config)
	bool DisplayInvalidAssets;

	/** Whether to display toolbar button. */
	UPROPERTY(config)
	bool DisplayToolbarButton;

	/** Whether search and filter assets recursively. */
	UPROPERTY(config)
	bool SearchAndFilterRecursively;

	/** Whether use straight or spline connect line between nodes in dependency viewer. */
	UPROPERTY(config)
	bool UseStraightLineInDependencyViewer;

	/** Whether to show dependency viewer under asset view or on the right of asset view. */
	UPROPERTY(config)
	bool ShowDependencyViewerUnderAssetView;

	/** The number of objects to keep in the Content Browser Recently Opened filter */
	UPROPERTY(EditAnywhere, config, Category = ContentBrowser, meta = (DisplayName = "Number of Assets to Keep in the Recently Opened Filter", ClampMin = "1", ClampMax = "30"))
	int32 NumObjectsInRecentList;

	/** Whether the Content Browser should open the Sources Panel by default */
	UPROPERTY(EditAnywhere, config, Category = ContentBrowser)
	bool bShowFullCollectionNameInToolTip;

	//////////////////////////
	// Cache Setting
	UPROPERTY(EditAnywhere, config, Category = CacheDB)
	bool bCacheMode;

	UPROPERTY(EditAnywhere, config, Category = CacheDB)
	FLinearColor CacheModeBorderColor;

	/** If caching is enabled then it will save cache on close. */
	UPROPERTY(EditAnywhere, config, Category = CacheDB)
	bool bAutoSaveCacheOnExit;

	UPROPERTY(EditAnywhere, config, Category = CacheDB)
	bool bAutoSaveCacheOnSwitchToCacheMode;

	/** If caching is enabled then it will save cache on close. */
	UPROPERTY(EditAnywhere, config, Category = CacheDB)
	bool bKeepCachedAssetsWhenRootRemoved;

	UPROPERTY(EditAnywhere, config, Category = CacheDB)
	bool bShowCacheStatusBarInLiveMode;

	/** The number of objects to load at once in the Content Browser before displaying a warning about loading many assets */
	UPROPERTY(EditAnywhere, config, Category = CacheDB, meta = (DisplayName = "Cache Database File Path"))
	FFilePath  CacheFilePath;

	//////////////////////////
	// Thumbnail Settings
	UPROPERTY(EditAnywhere, config, Category = ThumbnailPool)
	bool bUseThumbnailPool;

	UPROPERTY(EditAnywhere, config, Category = ThumbnailPool)
	int32 NumThumbnailsInPool;

	//////////////////////////
	// Import Settings
	UPROPERTY(config)
	bool bSkipImportIfAnyDependencyMissing;

	UPROPERTY(config)
	bool bImportIgnoreSoftReferencesError;

	UPROPERTY(config)
	bool bImportOverwriteExistingFiles;

	UPROPERTY(config)
	bool bRollbackImportIfFailed;

	UPROPERTY(config)
	bool bImportSyncAssetsInContentBrowser;

	UPROPERTY(config)
	bool bImportSyncExistingAssets;

	UPROPERTY(config)
	bool bLoadAssetAfterImport;

	UPROPERTY(config)
	bool bAddImportedAssetsToCollection;

	UPROPERTY(config)
	bool bUniqueCollectionNameForEachImportSession;

	UPROPERTY(config)
	FName DefaultImportedUAssetCollectionName;

	UPROPERTY(config)
	bool bImportToPluginFolder;

	UPROPERTY(config)
	bool bWarnBeforeImportToPluginFolder;

	UPROPERTY(config)
	FName ImportToPluginName;

	UPROPERTY(config)
	bool bImportFolderColor;

	UPROPERTY(config)
	bool bOverrideExistingFolderColor;

	//////////////////////////
	// Export Settings
	UPROPERTY(config)
	bool bSkipExportIfAnyDependencyMissing;

	UPROPERTY(config)
	bool bExportIgnoreSoftReferencesError;

	UPROPERTY(config)
	bool bExportOverwriteExistingFiles;
	
	UPROPERTY(config)
	bool bRollbackExportIfFailed;

	UPROPERTY(config)
	bool bOpenFolderAfterExport;

	//////////////////////////
	// Folder Tree Settings
	UPROPERTY(config)		
	bool bIgnoreFoldersStartWithDot;

	UPROPERTY(config)
	bool bIgnoreCommonNonContentFolders;
	TArray<FString> CommonNonContentFolders;

	UPROPERTY(config)
	bool bIgnoreExternalContentFolders;
	TArray<FString> ExternalContentFolders;

	UPROPERTY(config)
	bool bIgnoreMoreFolders;

	UPROPERTY(config)
	TArray<FString> IgnoreFolders;

public:

	/** Reset all setting to factory default */
	void Reset();

	/** Sets whether we are allowed to display the engine folder or not, optional flag for setting override instead */
	void SetDisplayEngineFolder( bool bInDisplayEngineFolder, bool bOverride = false )
	{ 
		bOverride ? OverrideDisplayEngineFolder = bInDisplayEngineFolder : DisplayEngineFolder = bInDisplayEngineFolder;
	}

	/** Gets whether we are allowed to display the engine folder or not, optional flag ignoring the override */
	bool GetDisplayEngineFolder( bool bExcludeOverride = false ) const
	{ 
		return ( ( bExcludeOverride ? false : OverrideDisplayEngineFolder ) || DisplayEngineFolder );
	}

	/** Sets whether we are allowed to display the developers folder or not, optional flag for setting override instead */
	void SetDisplayDevelopersFolder( bool bInDisplayDevelopersFolder, bool bOverride = false )
	{ 
		bOverride ? OverrideDisplayDevelopersFolder = bInDisplayDevelopersFolder : DisplayDevelopersFolder = bInDisplayDevelopersFolder;
	}

	/** Gets whether we are allowed to display the developers folder or not, optional flag ignoring the override */
	bool GetDisplayDevelopersFolder( bool bExcludeOverride = false ) const
	{ 
		return ( ( bExcludeOverride ? false : OverrideDisplayDevelopersFolder ) || DisplayDevelopersFolder );
	}

	/** Sets whether we are allowed to display the L10N folder (contains localized assets) or not */
	void SetDisplayL10NFolder(bool bInDisplayL10NFolder)
	{
		DisplayL10NFolder = bInDisplayL10NFolder;
	}

	/** Gets whether we are allowed to display the L10N folder (contains localized assets) or not */
	bool GetDisplayL10NFolder() const
	{
		return DisplayL10NFolder;
	}

	/** Sets whether we are allowed to display the plugin folders or not, optional flag for setting override instead */
	void SetDisplayPluginFolders( bool bInDisplayPluginFolders, bool bOverride = false )
	{ 
		bOverride ? OverrideDisplayPluginFolders = bInDisplayPluginFolders : DisplayPluginFolders = bInDisplayPluginFolders;
	}

	/** Gets whether we are allowed to display the plugin folders or not, optional flag ignoring the override */
	bool GetDisplayPluginFolders( bool bExcludeOverride = false ) const
	{ 
		return ( ( bExcludeOverride ? false : OverrideDisplayPluginFolders ) || DisplayPluginFolders );
	}

	/** Sets whether we are allowed to display collection folders or not */
	void SetDisplayCollections(bool bInDisplayCollections)
	{
		DisplayCollections = bInDisplayCollections;
	}

	/** Gets whether we are allowed to display the collection folders or not*/
	bool GetDisplayCollections() const
	{
		return DisplayCollections;
	}

	/** Sets whether we are allowed to display favorite folders or not */
	void SetDisplayFavorites(bool bInDisplayFavorites)
	{
		DisplayFavorites = bInDisplayFavorites;
	}

	/** Gets whether we are allowed to display the favorite folders or not*/
	bool GetDisplayFavorites() const
	{
		return DisplayFavorites;
	}

	/** Sets whether we are allowed to display C++ folders or not */
	void SetDisplayCppFolders(bool bDisplay)
	{
		DisplayCppFolders = bDisplay;
	}

	/** Gets whether we are allowed to display the C++ folders or not*/
	bool GetDisplayCppFolders() const
	{
		return DisplayCppFolders;
	}

	/** Sets whether text searches should also search in asset class names */
	void SetIncludeClassNames(bool bInclude)
	{
		IncludeClassNames = bInclude;
	}

	/** Gets whether text searches should also search in asset class names */
	bool GetIncludeClassNames() const
	{
		return IncludeClassNames;
	}

	/** Sets whether text searches should also search asset paths (instead of asset name only) */
	void SetIncludeAssetPaths(bool bInclude)
	{
		IncludeAssetPaths = bInclude;
	}

	/** Gets whether text searches should also search asset paths (instead of asset name only) */
	bool GetIncludeAssetPaths() const
	{
		return IncludeAssetPaths;
	}

	/** Sets whether text searches should also search for collection names */
	void SetIncludeCollectionNames(bool bInclude)
	{
		IncludeCollectionNames = bInclude;
	}

	/** Gets whether text searches should also search for collection names */
	bool GetIncludeCollectionNames() const
	{
		return IncludeCollectionNames;
	}

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(UExtContentBrowserSettings, FSettingChangedEvent, FName /*PropertyName*/);
	static FSettingChangedEvent& OnSettingChanged( ) { return SettingChangedEvent; }

	void ResetCacheSettings();

	void ResetThumbnailPoolSettings();

	/** Reset Import Settings to default */
	void ResetImportSettings();

	/** Reset Import Settings to default */
	void ResetExportSettings();

	void ResetViewSettings();

protected:

	// UObject overrides

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;

private:

	/** Whether to display the engine folder in the assets view of the content browser. */
	UPROPERTY(config)
	bool DisplayEngineFolder;

	/** If true, overrides the DisplayEngine setting */
	bool OverrideDisplayEngineFolder;

	/** Whether to display the developers folder in the path view of the content browser */
	UPROPERTY(config)
	bool DisplayDevelopersFolder;

	UPROPERTY(config)
	bool DisplayL10NFolder;

	/** If true, overrides the DisplayDev setting */
	bool OverrideDisplayDevelopersFolder;

	/** List of plugin folders to display in the content browser. */
	UPROPERTY(config)
	bool DisplayPluginFolders;

	/** Temporary override for the DisplayPluginFolders setting */
	bool OverrideDisplayPluginFolders;

	UPROPERTY(config)
	bool DisplayCollections;

	UPROPERTY(config)
	bool DisplayFavorites;

	UPROPERTY(config)
	bool DisplayCppFolders;

	UPROPERTY(config)
	bool IncludeClassNames;

	UPROPERTY(config)
	bool IncludeAssetPaths;

	UPROPERTY(config)
	bool IncludeCollectionNames;

	// Holds an event delegate that is executed when a setting has changed.
	static FSettingChangedEvent SettingChangedEvent;
};

// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "IExtContentBrowserSingleton.h"
#include "ExtAssetData.h"
#include "ExtContentBrowserSettings.h"
#include "ExtAssetThumbnail.h"

#include "AssetRegistry/AssetData.h"

class SExtContentBrowser;
class FSpawnTabArgs;
class FTabManager;
class FViewport;
class UFactory;

struct FExtContentBrowserPluginInfo;

#if ECB_WIP_MULTI_INSTANCES
#define MAX_EXT_CONTENT_BROWSERS 4
#else
#define MAX_EXT_CONTENT_BROWSERS 1
#endif

/**
 * Plugin related info
 */
struct FExtContentBrowserPluginInfo
{
	FExtContentBrowserPluginInfo();

	FText FriendlyName;
	FText VersionName;
	FText CreatedBy;

	FString ResourcesDir;
};



/**
 * External content browser module singleton implementation class
 */
class FExtContentBrowserSingleton : public IExtContentBrowserSingleton
{
public:
	/** Constructor, Destructor */
	FExtContentBrowserSingleton();
	virtual ~FExtContentBrowserSingleton();

	// IExtContentBrowserSingleton interface
	virtual TSharedRef<class SWidget> CreateContentBrowser( const FName InstanceName, TSharedPtr<SDockTab> ContainingTab) override;
#if ECB_LEGACY
	virtual TSharedRef<class SWidget> CreateAssetPicker(const FAssetPickerConfig& AssetPickerConfig) override;
	virtual TSharedRef<class SWidget> CreatePathPicker(const FPathPickerConfig& PathPickerConfig) override;
	virtual TSharedRef<class SWidget> CreateCollectionPicker(const FCollectionPickerConfig& CollectionPickerConfig) override;
	virtual void CreateOpenAssetDialog(const FOpenAssetDialogConfig& OpenAssetConfig, const FOnAssetsChosenForOpen& OnAssetsChosenForOpen, const FOnAssetDialogCancelled& OnAssetDialogCancelled) override;
	virtual TArray<FAssetData> CreateModalOpenAssetDialog(const FOpenAssetDialogConfig& InConfig) override;
	virtual void CreateSaveAssetDialog(const FSaveAssetDialogConfig& SaveAssetConfig, const FOnObjectPathChosenForSave& OnAssetNameChosenForSave, const FOnAssetDialogCancelled& OnAssetDialogCancelled) override;
	virtual FString CreateModalSaveAssetDialog(const FSaveAssetDialogConfig& SaveAssetConfig) override;
	virtual bool HasPrimaryContentBrowser() const override;
	virtual void FocusPrimaryContentBrowser(bool bFocusSearch) override;
	virtual void CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory) override;
	virtual void SyncBrowserToAssets(const TArray<struct FAssetData>& AssetDataList, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) override;
	virtual void SyncBrowserToAssets(const TArray<UObject*>& AssetList, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) override;
	virtual void SyncBrowserToFolders(const TArray<FString>& FolderList, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) override;
	virtual void SyncBrowserTo(const FExtContentBrowserSelection& ItemSelection, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) override;
#endif
	virtual void GetSelectedAssets(TArray<FExtAssetData>& SelectedAssets) override;
#if ECB_LEGACY
	virtual void GetSelectedFolders(TArray<FString>& SelectedFolders) override;
	virtual void GetSelectedPathViewFolders(TArray<FString>& SelectedFolders) override;
	virtual void CaptureThumbnailFromViewport(FViewport* InViewport, TArray<FAssetData>& SelectedAssets) override;
#endif

#if ECB_WIP_MULTI_INSTANCES
	virtual void SetSelectedPaths(const TArray<FString>& FolderPaths, bool bNeedsRefresh = false) override;
#endif

	/** Gets the content browser singleton as a FContentBrowserSingleton */
	static FExtContentBrowserSingleton& Get();

	/** Gets the content browser singleton as a FContentBrowserSingleton */
	static FExtAssetRegistry& GetAssetRegistry();

	static FExtObjectThumbnailPool& GetThumbnailPool();

	static FString GetPluginResourcesDir();

	static FText GetPluginVersionText();

	void ToggleShowToolbarButton();

	/** Sets the current primary content browser. */
	void SetPrimaryContentBrowser(const TSharedRef<SExtContentBrowser>& NewPrimaryBrowser);

#if ECB_WIP_MULTI_INSTANCES
	/** Notifies the singleton that a browser was closed */
	void ContentBrowserClosed(const TSharedRef<SExtContentBrowser>& ClosedBrowser);

#endif
	/** Single storage location for content browser favorites */
	TArray<FString> FavoriteFolderPaths;

public:
	static FExtAssetRegistry ExtAssetRegistry;
	FExtObjectThumbnailPool ThumbnailPool;

private:

private:

	void RegisterEditorDelegates();
	void UnregisterEditorDelegates();

	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

	void MountSandbox();
	void UnMountSandbox();

	void RegisterMenuAndTabSpawner();

	void RegisterCommands();

	void AddToolbarExtension(FToolBarBuilder& Builder);

	void OpenUAssetBrowserButtonClicked() const;

#if ECB_LEGACY
	/** Util to get or create the content browser that should be used by the various Sync functions */
	TSharedPtr<SExtContentBrowser> FindContentBrowserToSync(bool bAllowLockedBrowsers, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false);

	/** Shared code to open an asset dialog window with a config */
	void SharedCreateAssetDialogWindow(const TSharedRef<class SAssetDialog>& AssetDialog, const FSharedAssetDialogConfig& InConfig, bool bModal) const;

	/** 
	 * Delegate handlers
	 **/
	/** Sets the primary content browser to the next valid browser in the list of all browsers */
	void ChooseNewPrimaryBrowser();

	/** Gives focus to the specified content browser */
	void FocuSExtContentBrowser(const TSharedPtr<SExtContentBrowser>& BrowserToFocus);

	/** Summons a new content browser */
	FName SummonNewBrowser(bool bAllowLockedBrowsers = false);
#endif
	/** Handler for a request to spawn a new content browser tab */
	TSharedRef<SDockTab> SpawnContentBrowserTab(const FSpawnTabArgs& SpawnTabArgs, int32 BrowserIdx );

	/** Handler for a request to spawn a new content browser tab */
	FText GetContentBrowserTabLabel(int32 BrowserIdx);

#if ECB_WIP_LOCK
	/** Returns true if this content browser is locked (can be used even when closed) */
	bool IsLocked(const FName& InstanceName) const;
#endif

	/** Returns a localized name for the tab/menu entry with index */
	static FText GetContentBrowserLabelWithIndex( int32 BrowserIdx );

public:
	/** The tab identifier/instance name for content browser tabs */
	FName ContentBrowserTabIDs[MAX_EXT_CONTENT_BROWSERS];

#if ECB_WIP_MULTI_INSTANCES
private:
	TArray<TWeakPtr<SExtContentBrowser>> AllContentBrowsers;

	TMap<FName, TWeakPtr<FTabManager>> BrowserToLastKnownTabManagerMap;
#endif
	TWeakPtr<SExtContentBrowser> PrimaryContentBrowser;

private:
	TSharedPtr<class FExtender> ToolbarExtender;
	TSharedPtr<const class FExtensionBase> ToolbarExtension;

	TSharedPtr<class FUICommandList> ExtContentBrowserCommands;

	TSharedPtr<class FExtDependencyGraphPanelNodeFactory> ExtDependencyGraphPanelNodeFactory;

	FExtContentBrowserPluginInfo PluginInfo;
	
};

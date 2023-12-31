// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "IContentBrowserSingleton.h"
#include "ExtContentBrowser.h"
#include "ExtAssetData.h"

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "Misc/Attribute.h"
#include "AssetRegistry/AssetData.h"
#include "Developer/AssetTools/Public/AssetTypeCategories.h"
#include "AssetRegistry/ARFilter.h"
#include "ContentBrowserDelegates.h"
#include "Developer/CollectionManager/Public/CollectionManagerTypes.h"
#include "Misc/FilterCollection.h"
#include "Framework/Views/ITypedTableView.h"
#include "AssetThumbnail.h"

class FViewport;
class UFactory;

typedef const FExtAssetData& FExtAssetFilterType;
typedef TFilterCollection<FExtAssetFilterType> FExtAssetFilterCollectionType;


/** A selection of items in the Content Browser */
struct FExtContentBrowserSelection
{
	TArray<FExtAssetData> SelectedAssets;
	TArray<FString> SelectedFolders;

	int32 Num() const
	{
		return SelectedAssets.Num() + SelectedFolders.Num();
	}

	void Reset()
	{
		SelectedAssets.Reset();
		SelectedFolders.Reset();
	}

	void Empty()
	{
		SelectedAssets.Empty();
		SelectedFolders.Empty();
	}
};

#if ECB_LEGACY
/** A struct containing details about how the asset picker should behave */
struct FAssetPickerConfig
{
	/** The selection mode the picker should use */
	ESelectionMode::Type SelectionMode;

	/** An array of pointers to existing delegates which the AssetView will register a function which returns the current selection */
	TArray<FGetCurrentSelectionDelegate*> GetCurrentSelectionDelegates;

	/** An array of pointers to existing delegates which the AssetView will register a function which sync the asset list*/
	TArray<FSyncToAssetsDelegate*> SyncToAssetsDelegates;

	/** A pointer to an existing delegate that, when executed, will set the filter an the asset picker after it is created. */
	TArray<FSetARFilterDelegate*> SetFilterDelegates;

	/** A pointer to an existing delegate that, when executed, will refresh the asset view. */
	TArray<FRefreshAssetViewDelegate*> RefreshAssetViewDelegates;

	/** The asset registry filter to use to cull results */
	FARFilter Filter;

	/** Custom front end filters to be displayed */
	TArray< TSharedRef<class FExtFrontendFilter> > ExtraFrontendFilters;

	/** The names of columns to hide by default in the column view */
	TArray<FString> HiddenColumnNames;

	/** List of custom columns that fill out data with a callback */
	TArray<FAssetViewCustomColumn> CustomColumns;

	/** The contents of the label on the thumbnail */
	EThumbnailLabel::Type ThumbnailLabel;

	/** The default scale for thumbnails. [0-1] range */
	TAttribute< float > ThumbnailScale;

	/** Only display results in these collections */
	TArray<FCollectionNameType> Collections;

	/** The asset that should be initially selected */
	FAssetData InitialAssetSelection;

	/** The delegate that fires when an asset was selected */
	FOnAssetSelected OnAssetSelected;

	/** The delegate that fires when a folder was double clicked */
	FOnPathSelected OnFolderEntered;

	/** The delegate that fires when an asset is double clicked */
	FOnAssetDoubleClicked OnAssetDoubleClicked;

	/** The delegate that fires when an asset has enter pressed while selected */
	FOnAssetEnterPressed OnAssetEnterPressed;

	/** The delegate that fires when any number of assets are activated */
	FOnAssetsActivated OnAssetsActivated;

	/** The delegate that fires when an asset is right clicked and a context menu is requested */
	FOnGetAssetContextMenu OnGetAssetContextMenu;

	/** The delegate that fires when a folder is right clicked and a context menu is requested */
	FOnGetFolderContextMenu OnGetFolderContextMenu;

	/** Called to see if it is valid to get a custom asset tool tip */
	FOnIsAssetValidForCustomToolTip OnIsAssetValidForCustomToolTip;

	/** Fired when an asset item is constructed and a tooltip is requested. If unbound the item will use the default widget */
	FOnGetCustomAssetToolTip OnGetCustomAssetToolTip;

	/** Called to add extra asset data to the asset view, to display virtual assets. These get treated similar to Class assets */
	FOnGetCustomSourceAssets OnGetCustomSourceAssets;

	/** Fired when an asset item is about to show its tool tip */
	FOnVisualizeAssetToolTip OnVisualizeAssetToolTip;

	/** Fired when an asset item's tooltip is closing */
	FOnAssetToolTipClosing OnAssetToolTipClosing;

	/** If more detailed filtering is required than simply Filter, this delegate will get fired for every asset to determine if it should be culled. */
	FOnShouldFilterAsset OnShouldFilterAsset;

	/** This delegate will be called in Details view when a new asset registry searchable tag is encountered, to
	    determine if it should be displayed or not.  If it returns true or isn't bound, the tag will be displayed normally. */
	FOnShouldDisplayAssetTag OnAssetTagWantsToBeDisplayed;

	/** The default view mode */
	EAssetViewType::Type InitialAssetViewType;

	/** The text to show when there are no assets to show */
	TAttribute< FText > AssetShowWarningText;

	/** If set, view settings will be saved and loaded for the asset view using this name in ini files */
	FString SaveSettingsName;

	/** If true, the search box will gain focus when the asset picker is created */
	bool bFocusSearchBoxWhenOpened;

	/** If true, a "None" item will always appear at the top of the list */
	bool bAllowNullSelection;

	/** If true, show the bottom toolbar which shows # of assets selected, view mode buttons, etc... */
	bool bShowBottomToolbar;

	/** If false, auto-hide the search bar above */
	bool bAutohideSearchBar;

	/** Whether to allow dragging of items */
	bool bAllowDragging;

	/** Indicates if the 'Show folders' option should be enabled or disabled */
	bool bCanShowFolders;

	/** Indicates if the context menu is going to load the assets, and if so to preload before the context menu is shown, and warn about the pending load. */
	bool bPreloadAssetsForContextMenu;

	/** Indicates that we would like to build the filter UI with the Asset Picker */
	bool bAddFilterUI;

	/** If true, show path in column view */
	bool bShowPathInColumnView; 
	/** If true, show class in column view */
	bool bShowTypeInColumnView;
	/** If true, sort by path in column view. Only works if initial view type is Column */
	bool bSortByPathInColumnView;

	/** Override the default filter context menu layout */
	EAssetTypeCategories::Type DefaultFilterMenuExpansion;

	FAssetPickerConfig()
		: SelectionMode( ESelectionMode::Multi )
		, ThumbnailLabel( EThumbnailLabel::ClassName )
		, ThumbnailScale(0.1f)
		, InitialAssetViewType(EAssetViewType::Tile)
		, bFocusSearchBoxWhenOpened(true)
		, bAllowNullSelection(false)
		, bShowBottomToolbar(true)
		, bAutohideSearchBar(false)
		, bAllowDragging(true)
		, bCanShowFolders(false)
		, bPreloadAssetsForContextMenu(true)
		, bAddFilterUI(false)
		, bShowPathInColumnView(false)
		, bShowTypeInColumnView(true)
		, bSortByPathInColumnView(false)
		, DefaultFilterMenuExpansion(EAssetTypeCategories::Basic)
	{}
};

/** A struct containing details about how the path picker should behave */
struct FPathPickerConfig
{
	/** The initial path to select. Leave empty to skip initial selection. */
	FString DefaultPath;

	/** The delegate that fires when a path was selected */
	FOnPathSelected OnPathSelected;

	/** The delegate that fires when a path is right clicked and a context menu is requested */
	FContentBrowserMenuExtender_SelectedPaths OnGetPathContextMenuExtender;

	/** The delegate that fires when a folder is right clicked and a context menu is requested */
	FOnGetFolderContextMenu OnGetFolderContextMenu;

	/** A pointer to an existing delegate that, when executed, will set the paths for the path picker after it is created. */
	TArray<FSetPathPickerPathsDelegate*> SetPathsDelegates;

	/** If true, the search box will gain focus when the path picker is created */
	bool bFocusSearchBoxWhenOpened;

	/** If false, the context menu will not open when an item is right clicked */
	bool bAllowContextMenu;

	/** If true, will allow class folders to be shown in the picker */
	bool bAllowClassesFolder;

	/** If true, will add the path specified in DefaultPath to the tree if it doesn't exist already */
	bool bAddDefaultPath;

	FPathPickerConfig()
		: bFocusSearchBoxWhenOpened(true)
		, bAllowContextMenu(true)
		, bAllowClassesFolder(false)
		, bAddDefaultPath(false)
	{}
};

/** A struct containing details about how the collection picker should behave */
struct FCollectionPickerConfig
{
	/** If true, collection buttons will be displayed */
	bool AllowCollectionButtons;

	/** If true, users will be able to access the right-click menu of a collection */
	bool AllowRightClickMenu;

	/** Called when a collection was selected */
	FOnCollectionSelected OnCollectionSelected;

	FCollectionPickerConfig()
		: AllowCollectionButtons(true)
		, AllowRightClickMenu(true)
		, OnCollectionSelected()
	{}
};

namespace EAssetDialogType
{
	enum Type
	{
		Open,
		Save
	};
}

/** A struct containing shared details about how asset dialogs should behave. You should not instanciate this config, but use FOpenAssetDialogConfig or FSaveAssetDialogConfig instead. */
struct FSharedAssetDialogConfig
{
	FText DialogTitleOverride;
	FString DefaultPath;
	TArray<FName> AssetClassNames;
	FVector2D WindowSizeOverride;
	FOnPathSelected OnPathSelected;

	virtual EAssetDialogType::Type GetDialogType() const = 0;

	FSharedAssetDialogConfig()
		: WindowSizeOverride(ForceInitToZero)
	{}

	virtual ~FSharedAssetDialogConfig()
	{}
};

/** A struct containing details about how the open asset dialog should behave. */
struct FOpenAssetDialogConfig : public FSharedAssetDialogConfig
{
	bool bAllowMultipleSelection;

	virtual EAssetDialogType::Type GetDialogType() const override { return EAssetDialogType::Open; }

	FOpenAssetDialogConfig()
		: FSharedAssetDialogConfig()
		, bAllowMultipleSelection(false)
	{}
};

/** An enum to choose the behavior of the save asset dialog when the user chooses an asset that already exists */
namespace ESaveAssetDialogExistingAssetPolicy
{
	enum Type
	{
		/** Display an error and disallow the save */
		Disallow,

		/** Allow the save, but warn that the existing file will be overwritten */
		AllowButWarn
	};
}

/** A struct containing details about how the save asset dialog should behave. */
struct FSaveAssetDialogConfig : public FSharedAssetDialogConfig
{
	FString DefaultAssetName;
	ESaveAssetDialogExistingAssetPolicy::Type ExistingAssetPolicy;

	virtual EAssetDialogType::Type GetDialogType() const override { return EAssetDialogType::Save; }

	FSaveAssetDialogConfig()
		: FSharedAssetDialogConfig()
		, ExistingAssetPolicy(ESaveAssetDialogExistingAssetPolicy::Disallow)
	{}
};

#endif

/**
 * External Content browser module singleton
 */
class IExtContentBrowserSingleton
{
public:
	/** Virtual destructor */
	virtual ~IExtContentBrowserSingleton() {}

	/**
	 * Generates a content browser.  Generally you should not call this function, but instead use CreateAssetPicker().
	 *
	 * @param	InstanceName			Global name of this content browser instance
	 * @param	ContainingTab			The tab the browser is contained within or nullptr
	 * @param	ContentBrowserConfig	Initial defaults for the new content browser, or nullptr to use saved settings
	 *
	 * @return The newly created content browser widget
	 */
	virtual TSharedRef<class SWidget> CreateContentBrowser( const FName InstanceName, TSharedPtr<SDockTab> ContainingTab) = 0;

	/** Generates a list of assets that are selected in the primary content browser */
	virtual void GetSelectedAssets(TArray<FExtAssetData>& SelectedAssets) = 0;

#if ECB_LEGACY
	/**
	 * Generates an asset picker widget locked to the specified FARFilter.
	 *
	 * @param AssetPickerConfig		A struct containing details about how the asset picker should behave				
	 * @return The asset picker widget
	 */
	virtual TSharedRef<class SWidget> CreateAssetPicker(const FAssetPickerConfig& AssetPickerConfig) = 0;

	/**
	 * Generates a path picker widget.
	 *
	 * @param PathPickerConfig		A struct containing details about how the path picker should behave				
	 * @return The path picker widget
	 */
	virtual TSharedRef<class SWidget> CreatePathPicker(const FPathPickerConfig& PathPickerConfig) = 0;

	/**
	 * Generates a collection picker widget.
	 *
	 * @param CollectionPickerConfig		A struct containing details about how the collection picker should behave				
	 * @return The collection picker widget
	 */
	virtual TSharedRef<class SWidget> CreateCollectionPicker(const FCollectionPickerConfig& CollectionPickerConfig) = 0;

	/**
	 * Opens the Open Asset dialog in a non-modal window
	 *
	 * @param OpenAssetConfig				A struct containing details about how the open asset dialog should behave
	 * @param OnAssetsChosenForOpen			A delegate that is fired when assets are chosen and the open button is pressed
	 * @param OnAssetDialogCancelled		A delegate that is fired when the asset dialog is closed or cancelled
	 */
	virtual void CreateOpenAssetDialog(const FOpenAssetDialogConfig& OpenAssetConfig, const FOnAssetsChosenForOpen& OnAssetsChosenForOpen, const FOnAssetDialogCancelled& OnAssetDialogCancelled) = 0;

	/**
	 * Opens the Open Asset dialog in a modal window
	 *
	 * @param OpenAssetConfig				A struct containing details about how the open asset dialog should behave
	 * @return The assets that were chosen to be opened
	 */
	virtual TArray<FAssetData> CreateModalOpenAssetDialog(const FOpenAssetDialogConfig& InConfig) = 0;

	/**
	 * Opens the Save Asset dialog in a non-modal window
	 *
	 * @param SaveAssetConfig				A struct containing details about how the save asset dialog should behave
	 * @param OnAssetNameChosenForSave		A delegate that is fired when an object path is chosen and the save button is pressed
	 * @param OnAssetDialogCancelled		A delegate that is fired when the asset dialog is closed or cancelled
	 */
	virtual void CreateSaveAssetDialog(const FSaveAssetDialogConfig& SaveAssetConfig, const FOnObjectPathChosenForSave& OnAssetNameChosenForSave, const FOnAssetDialogCancelled& OnAssetDialogCancelled) = 0;

	/**
	 * Opens the Save Asset dialog in a modal window
	 *
	 * @param SaveAssetConfig				A struct containing details about how the save asset dialog should behave
	 * @return The object path that was chosen
	 */
	virtual FString CreateModalSaveAssetDialog(const FSaveAssetDialogConfig& SaveAssetConfig) = 0;

	/** Returns true if there is at least one browser open that is eligible to be a primary content browser */
	virtual bool HasPrimaryContentBrowser() const = 0;

	/** Brings the primary content browser to the front or opens one if it does not exist. */
	virtual void FocusPrimaryContentBrowser(bool bFocusSearch) = 0;

	/** Sets up an inline-name for the creation of a new asset in the primary content browser using the specified path and the specified class and/or factory */
	virtual void CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory) = 0;

	/** 
	 * Selects the supplied assets in the primary content browser. 
     *
	 * @param AssetList                     An array of AssetDataList structs to sync
	 * @param bAllowLockedBrowsers 	        When true, even locked browsers may handle the sync. Only set to true if the sync doesn't seem external to the content browser
	 * @param bFocuSExtContentBrowser          When true, brings the ContentBrowser into the foreground.
	 * @param InstanceName					When supplied, will only sync the Content Browser with the matching InstanceName.  bAllowLockedBrowsers is ignored.
	 */ 
	virtual void SyncBrowserToAssets(const TArray<struct FAssetData>& AssetDataList, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) = 0;

	/** 
	 * Selects the supplied assets in the primary content browser. 
     *
	 * @param AssetList                     An array of UObject pointers to sync
	 * @param bAllowLockedBrowsers 	        When true, even locked browsers may handle the sync. Only set to true if the sync doesn't seem external to the content browser
	 * @param bFocuSExtContentBrowser          When true, brings the ContentBrowser into the foreground.
	 * @param InstanceName					When supplied, will only sync the Content Browser with the matching InstanceName.  bAllowLockedBrowsers is ignored.
	 */ 
	virtual void SyncBrowserToAssets(const TArray<UObject*>& AssetList, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) = 0;

	/** 
	 * Selects the supplied assets in the primary content browser. 
     *
	 * @param AssetList                     An array of strings represeting folder asset paths to sync
	 * @param bAllowLockedBrowsers 	        When true, even locked browsers may handle the sync. Only set to true if the sync doesn't seem external to the content browser
	 * @param bFocuSExtContentBrowser          When true, brings the ContentBrowser into the foreground.
	 * @param InstanceName					When supplied, will only sync the Content Browser with the matching InstanceName.  bAllowLockedBrowsers is ignored.
	 * @param bNewSpawnBrowser				When supplied, will spawn a new Content Browser instead of selecting the assets in an existing one.
	 */ 
	virtual void SyncBrowserToFolders(const TArray<FString>& FolderList, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) = 0;

	/** 
	 * Selects the supplied assets in the primary content browser. 
     *
	 * @param ItemSelection 				A struct containing AssetData and Folders to sync
	 * @param bAllowLockedBrowsers 	        When true, even locked browsers may handle the sync. Only set to true if the sync doesn't seem external to the content browser
	 * @param bFocuSExtContentBrowser          When true, brings the ContentBrowser into the foreground.
	 * @param InstanceName					When supplied, will only sync the Content Browser with the matching InstanceName.  bAllowLockedBrowsers is ignored.
	 * @param bNewSpawnBrowser				When supplied, will spawn a new Content Browser instead of selecting the assets in an existing one.
	 */ 
	virtual void SyncBrowserTo(const FExtContentBrowserSelection& ItemSelection, bool bAllowLockedBrowsers = false, bool bFocuSExtContentBrowser = true, const FName& InstanceName = FName(), bool bNewSpawnBrowser = false) = 0;

	/** Generates a list of assets that are selected in the primary content browser */
	virtual void GetSelectedAssets(TArray<FAssetData>& SelectedAssets) = 0;

	/** Generates a list of folders that are selected in the primary content browser */
	virtual void GetSelectedFolders(TArray<FString>& SelectedFolders) = 0;

	/** Returns the folders that are selected in the path view */
	virtual void GetSelectedPathViewFolders(TArray<FString>& SelectedFolders) = 0;

	/**
	 * Capture active viewport to thumbnail and assigns that thumbnail to incoming assets
	 *
	 * @param InViewport - viewport to sample from
	 * @param InAssetsToAssign - assets that should receive the new thumbnail ONLY if they are assets that use GenericThumbnails
	 */
	virtual void CaptureThumbnailFromViewport(FViewport* InViewport, TArray<FAssetData>& SelectedAssets) = 0;

	/**
	 * Sets the content browser to display the selected paths
	 */
	virtual void SetSelectedPaths(const TArray<FString>& FolderPaths, bool bNeedsRefresh = false) = 0;

#endif
};

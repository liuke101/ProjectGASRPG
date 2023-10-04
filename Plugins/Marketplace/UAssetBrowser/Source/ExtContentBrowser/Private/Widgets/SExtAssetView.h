// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "IExtContentBrowserSingleton.h"
#include "ExtContentBrowser.h"
#include "ExtAssetViewTypes.h"
#include "ExtAssetData.h"
#include "HistoryManager.h"
#include "AssetViewSortManager.h"

#include "Misc/Attribute.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetThumbnail.h"
#include "SourcesData.h"
#include "Animation/CurveSequence.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

// Drag and drop
#include "ContentBrowserDelegates.h" // FAssetViewDragAndDropExtender
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/Commands/UICommandList.h"

class FMenuBuilder;
class FWeakWidgetPath;
class FWidgetPath;
class SExtAssetColumnView;
class SExtAssetListView;
class SExtAssetTileView;
class SComboButton;
class UFactory;
class FDragDropEvent; 
struct FPropertyChangedEvent;


/**
 * A widget to display a list of filtered assets
 */
class SExtAssetView : public SCompoundWidget
{
public:
	/** Fires whenever one of the "Search" options changes, useful for modifying search criteria to match */
	DECLARE_DELEGATE(FOnSearchOptionChanged);

	/** Fires whenever user click the show chagne log option */
	DECLARE_DELEGATE(FOnRequestShowChangeLog);

	/** Called when an asset is selected in the asset view */
	DECLARE_DELEGATE_OneParam(FOnAssetSelected, const FExtAssetData& /*AssetData*/);

	/** Called to check if an asset should be filtered out by external code. Return true to exclude the asset from the view. */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShouldFilterAsset, const FExtAssetData& /*AssetData*/);

// 	/** Called to request the menu when right clicking on an asset */
// 	DECLARE_DELEGATE_RetVal_ThreeParams(TSharedPtr<SWidget>, FOnGetFolderContextMenu, const TArray<FString>& /*SelectedPaths*/, FContentBrowserMenuExtender_SelectedPaths /*MenuExtender*/);
// 
// 	/** Called to request the menu when right clicking on an asset */
 	DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<SWidget>, FOnGetExtAssetContextMenu, const TArray<FExtAssetData>& /*SelectedAssets*/);

public:
	SLATE_BEGIN_ARGS(SExtAssetView)

		: _AreRealTimeThumbnailsAllowed(false)
		, _ThumbnailLabel( EThumbnailLabel::ClassName )
		, _AllowThumbnailHintLabel(true)
		, _InitialViewType(EAssetViewType::Tile)
		, _ThumbnailScale(0.1f)
		, _ShowBottomToolbar(true)
		, _CanShowFolders(false)
		, _FilterRecursivelyWithBackendFilter(true)
		, _CanShowCollections(false)
		, _CanShowFavorites(false)
#if ECB_TODO
		, _PreloadAssetsForContextMenu(true)
#endif
		, _SelectionMode( ESelectionMode::Multi )
#if ECB_FEA_ASSET_DRAG_DROP
		, _AllowDragging(true)
#endif
#if ECB_LEGACY
		, _AllowFocusOnSync(true)
#endif
		, _FillEmptySpaceInTileView(true)

		, _ShowMajorAssetTypeColumnsInColumnView(false)
		, _ShowPathInColumnView(true)
		, _ShowTypeInColumnView(true)
		, _SortByPathInColumnView(false)
		{}

		/** Called to check if an asset should be filtered out by external code */
		SLATE_EVENT( FOnShouldFilterAsset, OnShouldFilterAsset )

		/** Called to when an asset is selected */
		SLATE_EVENT( FOnAssetSelected, OnAssetSelected )

		/** Called when the user double clicks, presses enter, or presses space on an asset */
		SLATE_EVENT( FOnAssetsActivated, OnAssetsActivated )

		/** Called when an asset is right clicked */
		SLATE_EVENT( FOnGetExtAssetContextMenu, OnGetAssetContextMenu )
#if ECB_LEGACY
		/** Delegate to invoke when a context menu for a folder is opening. */
		SLATE_EVENT( FOnGetFolderContextMenu, OnGetFolderContextMenu )

		/** The delegate that fires when a path is right clicked and a context menu is requested */
		SLATE_EVENT( FContentBrowserMenuExtender_SelectedPaths, OnGetPathContextMenuExtender )

		/** Invoked when a "Find in Asset Tree" is requested */
		SLATE_EVENT( FOnFindInAssetTreeRequested, OnFindInAssetTreeRequested )

		/** Called when the user has committed a rename of one or more assets */
		SLATE_EVENT( FOnAssetRenameCommitted, OnAssetRenameCommitted )

		/** Delegate to call (if bound) to check if it is valid to get a custom tooltip for this asset item */
		SLATE_EVENT(FOnIsAssetValidForCustomToolTip, OnIsAssetValidForCustomToolTip)

		/** Called to get a custom asset item tool tip (if necessary) */
		SLATE_EVENT( FOnGetCustomAssetToolTip, OnGetCustomAssetToolTip )

		/** Called when an asset item is about to show a tooltip */
		SLATE_EVENT( FOnVisualizeAssetToolTip, OnVisualizeAssetToolTip )

		/** Called when an asset item's tooltip is closing */
		SLATE_EVENT(FOnAssetToolTipClosing, OnAssetToolTipClosing)
#endif
		/** The warning text to display when there are no assets to show */
		SLATE_ATTRIBUTE( FText, AssetShowWarningText )

		/** Attribute to determine if real-time thumbnails should be used */
		SLATE_ATTRIBUTE( bool, AreRealTimeThumbnailsAllowed )

		/** Attribute to determine what text should be highlighted */
		SLATE_ATTRIBUTE( FText, HighlightedText )

		/** What the label on the asset thumbnails should be */
		SLATE_ARGUMENT( EThumbnailLabel::Type, ThumbnailLabel )

		/** Whether to ever show the hint label on thumbnails */
		SLATE_ARGUMENT( bool, AllowThumbnailHintLabel )

		/** The filter collection used to further filter down assets returned from the backend */
		SLATE_ARGUMENT( TSharedPtr<FExtAssetFilterCollectionType>, FrontendFilters )

		/** The initial base sources filter */
		SLATE_ARGUMENT( FSourcesData, InitialSourcesData )

		/** The initial backend filter */
		SLATE_ARGUMENT( FARFilter, InitialBackendFilter )

		/** The asset that should be initially selected */
		SLATE_ARGUMENT( FExtAssetData, InitialAssetSelection )

		/** The initial view type */
		SLATE_ARGUMENT( EAssetViewType::Type, InitialViewType )

		/** The thumbnail scale. [0-1] where 0.5 is no scale */
		SLATE_ATTRIBUTE( float, ThumbnailScale )

		/** Should the toolbar indicating number of selected assets, mode switch buttons, etc... be shown? */
		SLATE_ARGUMENT( bool, ShowBottomToolbar )

		/** Indicates if the 'Show Folders' option should be enabled or disabled */
		SLATE_ARGUMENT( bool, CanShowFolders )

		/** If true, recursive filtering will be caused by applying a backend filter */
		SLATE_ARGUMENT( bool, FilterRecursivelyWithBackendFilter )

		/** Indicates if the 'Show Collections' option should be enabled or disabled */
		SLATE_ARGUMENT( bool, CanShowCollections )

		/** Indicates if the 'Show Favorites' option should be enabled or disabled */
		SLATE_ARGUMENT(bool, CanShowFavorites)
#if ECB_LEGACY
		/** Indicates if the context menu is going to load the assets, and if so to preload before the context menu is shown, and warn about the pending load. */
		SLATE_ARGUMENT( bool, PreloadAssetsForContextMenu )
#endif
		/** The selection mode the asset view should use */
		SLATE_ARGUMENT( ESelectionMode::Type, SelectionMode )

#if ECB_FEA_ASSET_DRAG_DROP
		/** Whether to allow dragging of items */
		SLATE_ARGUMENT( bool, AllowDragging )
#endif
#if ECB_LEGACY
		/** Whether this asset view should allow focus on sync or not */
		SLATE_ARGUMENT( bool, AllowFocusOnSync )
#endif
		/** Whether this asset view should allow the thumbnails to consume empty space after the user scale is applied */
		SLATE_ARGUMENT( bool, FillEmptySpaceInTileView )

		/** Should show major asset type columns in column view if true */
		SLATE_ARGUMENT(bool, ShowMajorAssetTypeColumnsInColumnView)

		/** Should show Path in column view if true */
		SLATE_ARGUMENT(bool, ShowPathInColumnView)

		/** Should show Type in column view if true */
		SLATE_ARGUMENT(bool, ShowTypeInColumnView)

		/** Sort by path in the column view. Only works if the initial view type is Column */
		SLATE_ARGUMENT(bool, SortByPathInColumnView)
#if ECB_LEGACY
		/** Called to check if an asset tag should be display in details view. */
		SLATE_EVENT( FOnShouldDisplayAssetTag, OnAssetTagWantsToBeDisplayed )
#endif
		/** Called when a folder is entered */
		SLATE_EVENT( FOnPathSelected, OnPathSelected )

#if ECB_LEGACY
		/** Called to add extra asset data to the asset view, to display virtual assets. These get treated similar to Class assets */
		SLATE_EVENT( FOnGetCustomSourceAssets, OnGetCustomSourceAssets )
#endif
		/** Columns to hide by default */
		SLATE_ARGUMENT( TArray<FString>, HiddenColumnNames )
			
		/** Custom columns that can be use specific */
		SLATE_ARGUMENT( TArray<FAssetViewCustomColumn>, CustomColumns )

		/** Custom columns that can be use specific */
		SLATE_EVENT( FOnSearchOptionChanged, OnSearchOptionsChanged)

		SLATE_EVENT(FOnRequestShowChangeLog, OnRequestShowChangeLog)

	SLATE_END_ARGS()

	~SExtAssetView();

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	/** Changes the base sources for this view */
	void SetSourcesData(const FSourcesData& InSourcesData);

	/** Returns the sources filter applied to this asset view */
	const FSourcesData& GetSourcesData() const;
#if ECB_TODO
	/** Returns true if a real asset path is selected (i.e \Engine\* or \Game\*) */
	bool IsAssetPathSelected() const;
#endif
	/** Notifies the asset view that the filter-list filter has changed */
	void SetBackendFilter(const FARFilter& InBackendFilter);
#if ECB_TODO
	/** Sets up an inline rename for the specified folder */
	void RenameFolder(const FString& FolderToRename);

	/** Selects the paths containing the specified assets. */
	void SyncToAssets( const TArray<FAssetData>& AssetDataList, const bool bFocusOnSync = true );

	/** Selects the specified paths. */
	void SyncToFolders( const TArray<FString>& FolderList, const bool bFocusOnSync = true );

	/** Selects the paths containing the specified items. */
	void SyncTo( const FExtContentBrowserSelection& ItemSelection, const bool bFocusOnSync = true );

	/** Sets the state of the asset view to the one described by the history data */
	void ApplyHistoryData( const FHistoryData& History );
#endif
	/** Returns all the items currently selected in the view */
	TArray<TSharedPtr<FExtAssetViewItem>> GetSelectedItems() const;

	/** Returns all the asset data objects in items currently selected in the view */
	TArray<FExtAssetData> GetSelectedAssets() const;

	/** Returns num of asset data objects in items currently selected in the view */
	int32 GetNumSelectedAssets() const;

	/** Returns all the folders currently selected in the view */
	TArray<FString> GetSelectedFolders() const;

	/** Requests that the asset view refreshes all it's source items. This is slow and should only be used if the source items change. */
	void RequestSlowFullListRefresh();

	/** Requests that the asset view refreshes only items that are filtered through frontend sources. This should be used when possible. */
	void RequestQuickFrontendListRefresh();
#if ECB_LEGACY
	/** Requests that the asset view adds any recently added items in the next update to the filtered asset items */
	void RequestAddNewAssetsNextFrame();
#endif
	/** Saves any settings to config that should be persistent between editor sessions */
	void SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const;

	/** Loads any settings to config that should be persistent between editor sessions */
	void LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString);
#if ECB_LEGACY
	/** Adjusts the selected asset by the selection delta, which should be +1 or -1) */
	void AdjustActiveSelection(int32 SelectionDelta);
#endif
#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	/** Processes assets that were loaded or changed since the last frame */
	void ProcessRecentlyLoadedOrChangedAssets();
#endif
	// SWidget inherited
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
#if ECB_FEA_ASSET_DRAG_DROP
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
#endif
#if ECB_LEGACY
	virtual FReply OnKeyChar( const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent ) override;
#endif
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
#if ECB_LEGACY
	virtual void OnFocusChanging( const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent ) override;

	/** Opens the selected assets or folders, depending on the selection */
	void OnOpenAssetsOrFolders();

	/** Loads the selected assets and previews them if possible */
	void OnPreviewAssets();
#endif
	/** Clears the selection of all the lists in the view */
	void ClearSelection(bool bForceSilent = false);

	/** Delegate called when an editor setting is changed */
	void HandleSettingChanged(FName PropertyName);
#if ECB_FEA_SEARCH
	/** Set whether the user is currently searching or not */
	void SetUserSearching(bool bInSearching);
#endif

#if ECB_LEGACY
	void OnAssetRegistryPathAdded(const FString& Path);

	/** Called when a folder is removed from the asset registry */
	void OnAssetRegistryPathRemoved(const FString& Path);
#endif

	/** Handles updating the content browser when an asset is added to the asset registry */
	virtual void OnAssetRegistryAssetGathered(const FExtAssetData& AssetData, int32 Left);

	/** Handles updating the content browser when a root content path is added to the asset registry */
	virtual void OnAssetRegistryRootPathAdded(const FString& Path);

	/** Handles updating the content browser when a root content path is removed from the asset registry */
	virtual void OnAssetRegistryRootPathRemoved(const FString& Path);

#if ECB_LEGACY
	/** Handles updating the content browser when a path is populated with an asset for the first time */
	void OnFolderPopulated(const FString& Path);

	/** @return true when we are including class names in search criteria */
	bool IsIncludingClassNames() const;

	/** @return true when we are including the entire asset path in search criteria */
	bool IsIncludingAssetPaths() const;

	/** @return true when we are including collection names in search criteria */
	bool IsIncludingCollectionNames() const;
#endif
private:

	/** Sets the pending selection to the current selection (used when changing views or refreshing the view). */
	void SyncToSelection( const bool bFocusOnSync = true );

	/** @return the thumbnail scale setting path to use when looking up the setting in an ini. */
	FString GetThumbnailScaleSettingPath(const FString& SettingsString) const;

	/** @return the view type setting path to use when looking up the setting in an ini. */
	FString GetCurrentViewTypeSettingPath(const FString& SettingsString) const;

	/** Calculates a new filler scale used to adjust the thumbnails to fill empty space. */
	void CalculateFillScale( const FGeometry& AllottedGeometry );

	/** Calculates the latest color and opacity for the hint on thumbnails */
	void CalculateThumbnailHintColorAndOpacity();

	/** Handles amortizing the backend filters */
	void ProcessQueriedItems( const double TickStartTime );

	/** Creates a new tile view */
	TSharedRef<class SExtAssetTileView> CreateTileView();

	/** Creates a new list view */
	TSharedRef<class SExtAssetListView> CreateListView();

	/** Creates a new column view */
	TSharedRef<class SExtAssetColumnView> CreateColumnView();

#if ECB_LEGACY
	/** Returns true if the specified search token is allowed */
	bool IsValidSearchToken(const FString& Token) const;
#endif
	/** Regenerates the AssetItems list from the AssetRegistry */
	void RefreshSourceItems();

	/** Regenerates the FilteredAssetItems list from the AssetItems list */
	void RefreshFilteredItems();

	/** Regenerates folders if we are displaying them */
	void RefreshFolders();

	/** Sets the asset type that represents the majority of the assets in view */
	void SetMajorityAssetType(FName NewMajorityAssetType);

#if ECB_LEGACY
	/** Handler for when an asset is added to a collection */
	void OnAssetsAddedToCollection( const FCollectionNameType& Collection, const TArray< FName >& ObjectPaths );
#endif
#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	/** Process assets that we were recently informed of & buffered in RecentlyAddedAssets */
	void ProcessRecentlyAddedAssets();
#endif
#if ECB_LEGACY
	/** Handler for when an asset is removed from a collection */
	void OnAssetsRemovedFromCollection( const FCollectionNameType& Collection, const TArray< FName >& ObjectPaths );

	/** Handler for when an asset was deleted or removed from the asset registry */
	void OnAssetRemoved(const FAssetData& AssetData);

	/** Removes the specified asset from view's caches */
	void RemoveAssetByPath( const FName& ObjectPath );

	/** Handler for when a collection is renamed */
	void OnCollectionRenamed( const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection );

	/** Handler for when a collection is updated */
	void OnCollectionUpdated( const FCollectionNameType& Collection );

	/** Handler for when an asset was renamed in the asset registry */
	void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);

	/** Handler for when an asset was loaded */
	void OnAssetLoaded(UObject* Asset);

	/** Handler for when an asset's property has changed */
	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	/** Called when the class hierarchy is updated due to the available modules changing */
	void OnClassHierarchyUpdated();
#endif

	/** Handler for when an asset is updated in the asset registry */
	void OnAssetUpdated(const FExtAssetData& AssetData);

	/** Handler for when any frontend filters have been changed */
	void OnFrontendFiltersChanged();

	/** Returns true if there is any frontend filter active */
	bool IsFrontendFilterActive() const;

	/** Returns true if the specified asset data item passes all applied frontend (non asset registry) filters */
	bool PassesCurrentFrontendFilter(const FExtAssetData& Item) const;

#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	/** Returns true if the specified asset data item passes all applied backend (asset registry) filters */
	bool PassesCurrentBackendFilter(const FExtAssetData& Item) const;

	/** Removes asset data items from the given array that don't pass all applied backend (asset registry) filters */
	void RunAssetsThroughBackendFilter(TArray<FExtAssetData>& InOutAssetDataList) const;
#endif

	/** Returns true if any filter is applying to current asset view */
	bool IsFiltering() const;

	/** Returns true if the current filters deem that the asset view should be filtered recursively (overriding folder view) */
	bool ShouldFilterRecursively() const;

	/** Sorts the contents of the asset view alphabetically */
	void SortList(bool bSyncToSelection = true);

	/** Returns the thumbnails hint color and opacity */
	FLinearColor GetThumbnailHintColorAndOpacity() const;

	/** Returns the foreground color for the view button */
	FSlateColor GetViewButtonForegroundColor() const;

	/** Handler for when the view combo button is clicked */
	TSharedRef<SWidget> GetViewButtonContent();

	/** Toggle whether folders should be shown or not */
	void ToggleShowFolders();

	/** Whether or not it's possible to show folders */
	bool IsToggleShowFoldersAllowed() const;

	/** @return true when we are showing folders */
	bool IsShowingFolders() const;

	/** Toggle whether empty folders should be shown or not */
	void ToggleShowEmptyFolders();

	/** Whether or not it's possible to show empty folders */
	bool IsToggleShowEmptyFoldersAllowed() const;

	/** @return true when we are showing empty folders */
	bool IsShowingEmptyFolders() const;

	/** Toggle whether asset tooltip should be shown or not */
	void ToggleShowAssetTooltip();

	/** @return true when we are showing asset tooltip */
	bool IsShowingAssetTooltip() const;

	void ShowDocumentation();

	void ShowPluginVersionChangeLog();

	/** Toggle whether show engine version overlay */
	void ToggleShowEngineVersionOverlayButton();

	/** @return true when we are showing engine version overlay */
	bool IsShowingEngineVersionOverlayButton() const;

	/** Toggle whether show content type overlay */
	void ToggleShowContentTypeOverlayButton();

	/** @return true when we are showing content type overlay */
	bool IsShowingContentTypeOverlayButton() const;

	/** Toggle whether show validation status overlay */
	void ToggleShowValidationStatusOverlayButton();

	/** @return true when we are showing validation status overlay */
	bool IsShowingValidationStatusOverlayButton() const;

	/** Toggle whether invalid assets should be shown or not */
	void ToggleShowInvalidAssets();

	/** @return true when we are showing invalid assets */
	bool IsShowingInvalidAssets() const;

	/** Toggle whether asset tooltip should be shown or not */
	void ToggleShowToolbarButton();

	/** @return true when we are showing asset tooltip */
	bool IsShowingToolbarButton() const;

	/** Toggle whether engine content should be shown or not */
	void ToggleShowEngineContent();

	/** @return true when we are showing engine content */
	bool IsShowingEngineContent() const;

	/** Toggle whether collections should be shown or not */
	void ToggleShowCollections();

	/** Whether or not it's possible to toggle collections */
	bool IsToggleShowCollectionsAllowed() const;

	/** @return true when we are showing collections */
	bool IsShowingCollections() const;

	/** Toggle whether favorites should be shown or not */
	void ToggleShowFavorites();

	/** Whether or not it's possible to toggle favorites */
	bool IsToggleShowFavoritesAllowed() const;

	/** @return true when we are showing favorites */
	bool IsShowingFavorites() const;

	/** Toggle whether favorites should be shown or not */
	void ToggleSearchAndFilterRecursively();

	/** @return true when we are showing favorites */
	bool IsSearchAndFilterRecursively() const;

#if ECB_LEGACY
	/** Toggle whether class names should be included in search criteria */
	void ToggleIncludeClassNames();

	/** Whether or not it's possible to include class names in search criteria */
	bool IsToggleIncludeClassNamesAllowed() const;

	/** Toggle whether the entire asset path should be included in search criteria */
	void ToggleIncludeAssetPaths();

	/** Whether or not it's possible to include the entire asset path in search criteria */
	bool IsToggleIncludeAssetPathsAllowed() const;

	/** Toggle whether collection names should be included in search criteria */
	void ToggleIncludeCollectionNames();

	/** Whether or not it's possible to include collection names in search criteria */
	bool IsToggleIncludeCollectionNamesAllowed() const;
#endif
	/** Sets the view type and updates lists accordingly */
	void SetCurrentViewType(EAssetViewType::Type NewType);

	/** Sets the view type and forcibly dismisses all currently open context menus */
	void SetCurrentViewTypeFromMenu(EAssetViewType::Type NewType);

	/** Clears the reference to the current view and creates a new one, based on CurrentViewType */
	void CreateCurrentView();

	/** Gets the current view type (list or tile) */
	EAssetViewType::Type GetCurrentViewType() const;

	TSharedRef<SWidget> CreateShadowOverlay( TSharedRef<STableViewBase> Table );

	/** Returns true if ViewType is the current view type */
	bool IsCurrentViewType(EAssetViewType::Type ViewType) const;

	/** Set the keyboard focus to the correct list view that should be active */
	void FocusList() const;

	/** Refreshes the list view to display any changes made to the non-filtered assets */
	void RefreshList();

	/** Sets the sole selection for all lists in the view */
	void SetSelection(const TSharedPtr<FExtAssetViewItem>& Item);

	/** Sets selection for an item in all lists in the view */
	void SetItemSelection(const TSharedPtr<FExtAssetViewItem>& Item, bool bSelected, const ESelectInfo::Type SelectInfo = ESelectInfo::Direct);

	/** Scrolls the selected item into view for all lists in the view */
	void RequestScrollIntoView(const TSharedPtr<FExtAssetViewItem>& Item);

	/** Handler for list view widget creation */
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handler for tile view widget creation */
	TSharedRef<ITableRow> MakeTileViewWidget(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handler for column view widget creation */
	TSharedRef<ITableRow> MakeColumnViewWidget(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handler for when any asset item widget gets destroyed */
	void AssetItemWidgetDestroyed(const TSharedPtr<FExtAssetViewItem>& Item);

	/** Creates new thumbnails that are near the view area and deletes old thumbnails that are no longer relevant. */
	void UpdateThumbnails();
	
	/**  Helper function for UpdateThumbnails. Adds the specified item to the new thumbnail relevancy map and creates any thumbnails for new items. Returns the thumbnail. */
	TSharedPtr<class FExtAssetThumbnail> AddItemToNewThumbnailRelevancyMap(const TSharedPtr<FExtAssetViewAsset>& Item, TMap< TSharedPtr<FExtAssetViewAsset>, TSharedPtr<class FExtAssetThumbnail> >& NewRelevantThumbnails);

	/** Handler for tree view selection changes */
	void AssetSelectionChanged( TSharedPtr<FExtAssetViewItem > AssetItem, ESelectInfo::Type SelectInfo );

	/** Handler for when an item has scrolled into view after having been requested to do so */
	void ItemScrolledIntoView(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedPtr<ITableRow>& Widget);

	/** Handler for context menus */
	TSharedPtr<SWidget> OnGetContextMenuContent();

	/** Handler called when an asset context menu is about to open */
	bool CanOpenContextMenu() const;

	/** Handler for double clicking an item */
	void OnListMouseButtonDoubleClick(TSharedPtr<FExtAssetViewItem> AssetItem);

	/** Handle dragging an asset */
	FReply OnDraggingAssetItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Returns true if tooltips should be allowed right now. Tooltips are typically disabled while right click scrolling. */
	bool ShouldAllowToolTips() const;

	/** Gets the text for the asset count label */
	FText GetAssetCountText() const;

	/** Gets the visibility of the list view */
	EVisibility GetListViewVisibility() const;

	/** Gets the visibility of the tile view */
	EVisibility GetTileViewVisibility() const;

	/** Gets the visibility of the column view */
	EVisibility GetColumnViewVisibility() const;
#if ECB_LEGACY
	/** Toggles thumbnail editing mode */
	void ToggleThumbnailEditMode();
#endif
	/** Gets the current value for the scale slider (0 to 1) */
	float GetThumbnailScale() const;

	/** Sets the current scale value (0 to 1) */
	void SetThumbnailScale( float NewValue );

	/** Is thumbnail scale slider locked? */
	bool IsThumbnailScalingLocked() const;

	/** Gets the scaled item height for the list view */
	float GetListViewItemHeight() const;

	/** Gets the final scaled item height for the tile view */
	float GetTileViewItemHeight() const;

	/** Gets the scaled item height for the tile view before the filler scale is applied */
	float GetTileViewItemBaseHeight() const;

	/** Gets the final scaled item width for the tile view */
	float GetTileViewItemWidth() const;

	/** Gets the scaled item width for the tile view before the filler scale is applied */
	float GetTileViewItemBaseWidth() const;

	/** Gets the sort mode for the supplied ColumnId */
	EColumnSortMode::Type GetColumnSortMode(const FName ColumnId) const;

	/** Gets the sort order for the supplied ColumnId */
	EColumnSortPriority::Type GetColumnSortPriority(const FName ColumnId) const;

	/** Handler for when a column header is clicked */
	void OnSortColumnHeader(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type NewSortMode);

	/** @return The state of the is working progress bar */
	TOptional< float > GetIsWorkingProgressBarState() const;

	/** Is the no assets to show warning visible? */
	EVisibility IsAssetShowWarningTextVisible() const;

	/** Gets the text for displaying no assets to show warning */
	FText GetAssetShowWarningText() const;

#if ECB_FEA_ASSET_DRAG_DROP

	bool OnDropAndDropToAssetView(const FAssetViewDragAndDropExtender::FPayload& InPayLoad);

	TSharedRef<FExtender> OnExtendLevelViewportMenu(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> InActors);

	/** Delegate for when assets or asset paths are dragged onto a folder */
	void OnAssetsOrPathsDragDroppedToContentBrowser(const TArray<FExtAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& DestinationPath);

	/** Delegate to respond to drop of assets or asset paths onto a folder */
	void ExecuteImportAndPlaceSelecteUAssetFiles();

	/** Delegate to respond to drop of assets or asset paths onto a folder */
	void ExecuteDropToContentBrowserToImport(TArray<FExtAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath);

	/** Delegate to respond to drop of assets or asset paths onto a folder */
	void ExecuteDropToContentBrowserToImportFlat(TArray<FExtAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath);
#endif

#if ECB_LEGACY
	/** Whether we have a single source collection selected */
	bool HasSingleCollectionSource() const;

	/** Delegate for when assets or asset paths are dragged onto a folder */
	void OnAssetsOrPathsDragDropped(const TArray<FAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& DestinationPath);

	/** Delegate for when external assets are dragged onto a folder */
	void OnFilesDragDropped(const TArray<FString>& AssetList, const FString& DestinationPath);

	/** Delegate to respond to drop of assets or asset paths onto a folder */
	void ExecuteDropCopy(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath);

	/** Delegate to respond to drop of assets or asset paths onto a folder */
	void ExecuteDropMove(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath);

	/** Delegate to respond to drop of assets or asset paths onto a folder */
	void ExecuteDropAdvancedCopy(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath);

	/** @return The current quick-jump term */
	FText GetQuickJumpTerm() const;

	/** @return Whether the quick-jump term is currently visible? */
	EVisibility IsQuickJumpVisible() const;

	/** @return The color that should be used for the quick-jump term */
	FSlateColor GetQuickJumpColor() const;

	/** Reset the quick-jump to its empty state */
	void ResetQuickJump();

	/**
	 * Called from OnKeyChar and OnKeyDown to handle quick-jump key presses
	 * @param InCharacter		The character that was typed
	 * @param bIsControlDown	Was the control key pressed?
	 * @param bIsAltDown		Was the alt key pressed?
	 * @param bTestOnly			True if we only want to test whether the key press would be handled, but not actually update the quick-jump term
	 * @return FReply::Handled or FReply::Unhandled
	 */
	FReply HandleQuickJumpKeyDown(const TCHAR InCharacter, const bool bIsControlDown, const bool bIsAltDown, const bool bTestOnly);

	/**
	 * Perform a quick-jump to the next available asset in FilteredAssetItems that matches the current term
	 * @param bWasJumping		True if we were performing an ongoing quick-jump last Tick
	 * @return True if the quick-jump found a valid match, false otherwise
	 */
	bool PerformQuickJump(const bool bWasJumping);
#endif

	/** Show or hide major asset type columns */
	void ToggleMajorAssetTypeColumns();

	/** @return true when we are showing major asset type columns */
	bool IsShowingMajorAssetTypeColumns() const;

	/** Generates the column filtering menu */
	void FillToggleColumnsMenu(FMenuBuilder& MenuBuilder);

	/** Resets the column filtering state to make them all visible */
	void ResetColumns();

	/** Export columns to CSV */
	void ExportColumns();

	/** Toggle the column at ColumnIndex */
	void ToggleColumn(const FString ColumnName);
	/** Sets the column visibility by removing/inserting the column*/
	void SetColumnVisibility(const FString ColumnName, const bool bShow);

	/** Whether or not a column can be toggled, has to be valid column and mandatory minimum number of columns = 1*/
	bool CanToggleColumn(const FString ColumnName) const;

	/** Whether or not a column is visible to show it's state in the filtering menu */
	bool IsColumnVisible(const FString ColumnName) const;

	/** Creates the row header context menu allowing for hiding individually clicked columns*/
	TSharedRef<SWidget> CreateRowHeaderMenuContent(const FString ColumnName);
	/** Will compute the max row size from all its children for the specified column id*/
	FVector2D GetMaxRowSizeForColumn(const FName& ColumnId);

#if ECB_LEGACY
public:
	/** Delegate that handles if any folder paths changed as a result of a move, rename, etc. in the asset view*/
	FOnFolderPathChanged OnFolderPathChanged;
#endif
private:

	/** The asset items being displayed in the view and the filtered list */
	TArray<FExtAssetData> QueriedAssetItems;
	TArray<FExtAssetData> AssetItems;
	TArray<TSharedPtr<FExtAssetViewItem>> FilteredAssetItems;

	/** The folder items being displayed in the view */
	TSet<FString> Folders;

#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	/** A set of assets that were loaded or changed since the last frame */
	TSet<FExtAssetData> RecentlyLoadedOrChangedAssets;

	/** A list of assets that were recently reported as added by the asset registry */
	TArray<FExtAssetData> RecentlyAddedAssets;
	TArray<FExtAssetData> FilteredRecentlyAddedAssets;
	double LastProcessAddsTime;
#endif
	/** The list view that is displaying the assets */
	EAssetViewType::Type CurrentViewType;
	TSharedPtr<class SExtAssetListView> ListView;
	TSharedPtr<class SExtAssetTileView> TileView;
	TSharedPtr<class SExtAssetColumnView> ColumnView;
	TSharedPtr<SBorder> ViewContainer;

	TSharedPtr<class SWidget> ThumbnailScaleContainer;

	/** The button that displays view options */
	TSharedPtr<SComboButton> ViewOptionsComboButton;

	/** The current base source filter for the view */
	FSourcesData SourcesData;

	FARFilter BackendFilter;
	TSharedPtr<FExtAssetFilterCollectionType> FrontendFilters;

	/** If true, the source items will be refreshed next frame. Very slow. */
	bool bSlowFullListRefreshRequested;

	/** If true, the frontend items will be refreshed next frame. Much faster. */
	bool bQuickFrontendListRefreshRequested;

	/** The list of items to sync next frame */
	FSelectionData PendingSyncItems;

	/** Should we take focus when the PendingSyncAssets are processed? */
	bool bPendingFocusOnSync;

	/** Was recursive filtering enabled the last time a full slow refresh performed? */
	bool bWereItemsRecursivelyFiltered;

	/** Called to check if an asset should be filtered out by external code */
	FOnShouldFilterAsset OnShouldFilterAsset;

	/** Called when an asset was selected in the list */
	FOnAssetSelected OnAssetSelected;

	/** Called when the user double clicks, presses enter, or presses space on an asset */
	FOnAssetsActivated OnAssetsActivated;

	/** Called when the user right clicks on an asset in the view */
	FOnGetExtAssetContextMenu OnGetAssetContextMenu;

	/** Delegate to invoke when generating the context menu for a folder */
	FOnGetFolderContextMenu OnGetFolderContextMenu;

	/** The delegate that fires when a folder is right clicked and a context menu is requested */
	FContentBrowserMenuExtender_SelectedPaths OnGetPathContextMenuExtender;

#if ECB_LEGACY

	/** Called when a "Find in Asset Tree" is requested */
	FOnFindInAssetTreeRequested OnFindInAssetTreeRequested;

	/** Called to check if an asset tag should be display in details view. */
	FOnShouldDisplayAssetTag OnAssetTagWantsToBeDisplayed;

	/** Called to see if it is valid to get a custom asset tool tip */
	FOnIsAssetValidForCustomToolTip OnIsAssetValidForCustomToolTip;

	/** Called to get a custom asset item tooltip (If necessary) */
	FOnGetCustomAssetToolTip OnGetCustomAssetToolTip;

	/** Called when a custom asset item is about to show a tooltip */
	FOnVisualizeAssetToolTip OnVisualizeAssetToolTip;

	/** Called when a custom asset item's tooltip is closing */
	FOnAssetToolTipClosing OnAssetToolTipClosing;

	/** Called to add extra asset data to the asset view, to display virtual assets. These get treated similar to Class assets */
	FOnGetCustomSourceAssets OnGetCustomSourceAssets;
#endif

	/** Called when a search option changes to notify that results should be rebuilt */
	FOnSearchOptionChanged OnSearchOptionsChanged;

	FOnRequestShowChangeLog OnRequestShowChangeLog;

	/** When true, filtered list items will be sorted next tick. Provided another sort hasn't happened recently or we are renaming an asset */
	bool bPendingSortFilteredItems;
	double CurrentTime;
	double LastSortTime;
	double SortDelaySeconds;

	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<class FExtAssetThumbnailPool> AssetThumbnailPool;

	/** A map of FAssetViewAsset to the thumbnail that represents it. Only items that are currently visible or within half of the FilteredAssetItems array index distance described by NumOffscreenThumbnails are in this list */
	TMap< TSharedPtr<FExtAssetViewAsset>, TSharedPtr<class FExtAssetThumbnail> > RelevantThumbnails;

	/** The set of FAssetItems that currently have widgets displaying them. */
	TArray< TSharedPtr<FExtAssetViewItem> > VisibleItems;

	/** The number of thumbnails to keep for asset items that are not currently visible. Half of the thumbnails will be before the earliest item and half will be after the latest. */
	int32 NumOffscreenThumbnails;

	/** The current size of relevant thumbnails */
	int32 CurrentThumbnailSize;

	/** Flag to defer thumbnail updates until the next frame */
	bool bPendingUpdateThumbnails;

	/** The size of thumbnails */
	int32 ListViewThumbnailResolution;
	int32 ListViewThumbnailSize;
	int32 ListViewThumbnailPadding;
	int32 TileViewThumbnailResolution;
	int32 TileViewThumbnailSize;
	int32 TileViewThumbnailPadding;
	int32 TileViewNameHeight;

	/** The current value for the thumbnail scale from the thumbnail slider */
	TAttribute< float > ThumbnailScaleSliderValue;

	/** The max and min thumbnail scales as a fraction of the rendered size */
	float MinThumbnailScale;
	float MaxThumbnailScale;

	/** Flag indicating if we will be filling the empty space in the tile view. */
	bool bFillEmptySpaceInTileView;

	/** The amount to scale each thumbnail so that the empty space is filled. */
	float FillScale;

	/** When in columns view, this is the name of the asset type which is most commonly found in the recent results */
	FName MajorityAssetType;

	/** The manager responsible for sorting assets in the view */
	FAssetViewSortManager SortManager;

	/** When true, selection change notifications will not be sent */
	bool bBulkSelecting;

	/** Indicates if the 'Show Folders' option should be enabled or disabled */
	bool bCanShowFolders;

	/** If true, recursive filtering will be caused by applying a backend filter */
	bool bFilterRecursivelyWithBackendFilter;

	/** Indicates if the 'Show Collections' option should be enabled or disabled */
	bool bCanShowCollections;

	/** Indicates if the 'Show Favorites' option should be enabled or disabled */
	bool bCanShowFavorites;

#if ECB_LEGACY
	/** Indicates if the context menu is going to load the assets, and if so to preload before the context menu is shown, and warn about the pending load. */
	bool bPreloadAssetsForContextMenu;
#endif

	/** If true, it will show major asset type columns in the asset view */
	bool bShowMajorAssetTypeColumnsInColumnView;

	/** If true, it will show path column in the asset view */
	bool bShowPathInColumnView;

	/** If true, it will show type in the asset view */
	bool bShowTypeInColumnView;

	/** If true, it sorts by path and then name */
	bool bSortByPathInColumnView;

	/** The current selection mode used by the asset view */
	ESelectionMode::Type SelectionMode;

	/** The max number of results to process per tick */
	float MaxSecondsPerFrame;

	/** When delegate amortization began */
	double AmortizeStartTime;

	/** The total time spent amortizing the delegate filter */
	double TotalAmortizeTime;

	/** Whether the asset view is currently working on something and should display a cue to the user */
	bool bIsWorking;

	/** The text to highlight on the assets */
	TAttribute< FText > HighlightedText;

	/** What the label on the thumbnails should be */
	EThumbnailLabel::Type ThumbnailLabel;

	/** Whether to ever show the hint label on thumbnails */
	bool AllowThumbnailHintLabel;

	/** The sequence used to generate the opacity of the thumbnail hint */
	FCurveSequence ThumbnailHintFadeInSequence;

	/** The current thumbnail hint color and opacity*/
	FLinearColor ThumbnailHintColorAndOpacity;

	/** The text to show when there are no assets to show */
	TAttribute< FText > AssetShowWarningText;

	/** Whether to allow dragging of items */
	bool bAllowDragging;

	/** Whether this asset view should allow focus on sync or not */
	bool bAllowFocusOnSync;

	/** Delegate to invoke when folder is entered. */
	FOnPathSelected OnPathSelected;

	/** Flag set if the user is currently searching */
	bool bUserSearching;

	/** Whether or not to notify about newly selected items on on the next asset sync */
	bool bShouldNotifyNextAssetSync;

#if ECB_LEGACY	
	/** A struct to hold data for the deferred creation of assets */
	struct FCreateDeferredAssetData
	{
		/** The name of the asset */
		FString DefaultAssetName;

		/** The path where the asset will be created */
		FString PackagePath;

		/** The class of the asset to be created */
		UClass* AssetClass;

		/** The factory to use */
		UFactory* Factory;
	};

	/** Asset pending deferred creation */
	TSharedPtr<FCreateDeferredAssetData> DeferredAssetToCreate;

	/** A struct to hold data for the deferred creation of a folder */
	struct FCreateDeferredFolderData
	{
		/** The name of the folder to create */
		FString FolderName;

		/** The path of the folder to create */
		FString FolderPath;
	};

	/** Folder pending deferred creation */
	TSharedPtr<FCreateDeferredFolderData> DeferredFolderToCreate;
#endif

#if ECB_WIP_BREADCRUMB
	/** Struct holding the data for the asset quick-jump */
	struct FQuickJumpData
	{
		FQuickJumpData()
			: bIsJumping(false)
			, bHasChangedSinceLastTick(false)
			, bHasValidMatch(false)
			, LastJumpTime(0)
		{
		}

		/** True if we're currently performing an ongoing quick-jump */
		bool bIsJumping;

		/** True if the jump data has changed since the last Tick */
		bool bHasChangedSinceLastTick;

		/** True if the jump term found a valid match */
		bool bHasValidMatch;

		/** Time (taken from Tick) that we last performed a quick-jump */
		double LastJumpTime;

		/** The string we should be be looking for */
		FString JumpTerm;
	};

	/** Data for the asset quick-jump */
	FQuickJumpData QuickJumpData;
#endif	
	/** Column filtering state */
	TArray<FString> DefaultHiddenColumnNames;
	TArray<FString> HiddenColumnNames;
	int32 NumVisibleColumns;
	TArray<FAssetViewCustomColumn> CustomColumns;

public:
	bool ShouldColumnGenerateWidget(const FString ColumnName) const;
};

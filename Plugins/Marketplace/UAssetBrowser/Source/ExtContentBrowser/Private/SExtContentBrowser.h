// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtContentBrowser.h"
#include "IExtContentBrowserSingleton.h"

#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "SAssetSearchBox.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "AssetRegistry/AssetData.h"

#if ECB_WIP_COLLECTION
#include "CollectionManagerTypes.h"
#endif

#if ECB_WIP_HISTORY
#include "HistoryManager.h"
#endif

class FAssetContextMenu;
class FFrontendFilter_Text;
class FSourcesSearch;
class FPathContextMenu;
class FTabManager;
class FUICommandList;
class SExtDependencyViewer;
class SExtAssetView;
class SCollectionView;
class SComboButton;
class SExtFilterList;
class SExtPathView;
class SSplitter;
class UFactory;

/**
 * A widget to display and work with uasset files
 */
class SExtContentBrowser
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SExtContentBrowser)
 		: _ContainingTab()
 		, _InitiallyLocked(false)
	{}

		/** The tab in which the content browser resides */
		SLATE_ARGUMENT( TSharedPtr<SDockTab>, ContainingTab )

		/** If true, this content browser will not sync from external sources. */
		SLATE_ARGUMENT( bool, InitiallyLocked )

	SLATE_END_ARGS()

	~SExtContentBrowser();

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, const FName& InstanceName);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

#if ECB_WIP_SYNC_ASSET

	/**
	 * Changes sources to show the specified assets and selects them in the asset view
	 *
	 *	@param AssetDataList		- A list of assets to sync the view to
	 * 
	 *	@param bAllowImplicitSync	- true to allow the view to sync to parent folders if they are already selected,
	 *								  false to force the view to select the explicit Parent folders of each asset 
	 */
	void SyncToAssets( const TArray<FExtAssetData>& AssetDataList, const bool bAllowImplicitSync = false, const bool bDisableFiltersThatHideAssets = true );

	/**
	 * Changes sources to show the specified folders and selects them in the asset view
	 *
	 *	@param FolderList			- A list of folders to sync the view to
	 * 
	 *	@param bAllowImplicitSync	- true to allow the view to sync to parent folders if they are already selected,
	 *								  false to force the view to select the explicit Parent folders of each asset 
	 */
	void SyncToFolders( const TArray<FString>& FolderList, const bool bAllowImplicitSync = false );

	/**
	 * Changes sources to show the specified items and selects them in the asset view
	 *
	 *	@param AssetDataList		- A list of assets to sync the view to
	 * 
	 *	@param bAllowImplicitSync	- true to allow the view to sync to parent folders if they are already selected,
	 *								  false to force the view to select the explicit Parent folders of each asset 
	 */
	void SyncTo( const FExtContentBrowserSelection& ItemSelection, const bool bAllowImplicitSync = false, const bool bDisableFiltersThatHideAssets = true );

#endif

	/** Sets this content browser as the primary browser. The primary browser is the target for asset syncs and contributes to the global selection set. */
	void SetIsPrimaryContentBrowser(bool NewIsPrimary);

	/** Returns if this browser can be used as the primary browser. */
	bool CanSetAsPrimaryContentBrowser() const;

#if ECB_WIP_MULTI_INSTANCES

	/** Get the unique name of this content browser's in */
	const FName GetInstanceName() const;

	/** Gets the tab manager for the tab containing this browser */
	TSharedPtr<FTabManager> GetTabManager() const;

	/** Handler for when "Open in new Content Browser" is selected */
	void OpenNewContentBrowser();

#endif

#if ECB_LEGACY

	/** Loads all selected assets if unloaded */
	void LoadSelectedObjectsIfNeeded();

#endif

	/** Returns all the assets that are selected in the asset view */
	void GetSelectedAssets(TArray<FExtAssetData>& SelectedAssets);

#if ECB_LEGACY
	/** Returns all the folders that are selected in the asset view */
	void GetSelectedFolders(TArray<FString>& SelectedFolders);

	/** Returns the folders that are selected in the path view */
	TArray<FString> GetSelectedPathViewFolders();
#endif
	/** Saves all persistent settings to config and returns a string identifier */
	void SaveSettings() const;
#if ECB_LEGACY
	/** Sets the content browser to show the specified paths */
	void SetSelectedPaths(const TArray<FString>& FolderPaths, bool bNeedsRefresh = false);
#endif

#if ECB_WIP_LOCK
	/** Returns true if this content browser does not accept syncing from an external source */
	bool IsLocked() const;

#endif
	/** Gives keyboard focus to the asset search box */
	void SetKeyboardFocusOnSearch() const;

	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
#if ECB_LEGACY
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnPreviewMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override;
#endif

private:

#if ECB_WIP_SYNC_ASSET
	/** Called prior to syncing the selection in this Content Browser */
	void PrepareToSync( const TArray<FAssetData>& AssetDataList, const TArray<FString>& FolderPaths, const bool bDisableFiltersThatHideAssets );
#endif

	/** Called to retrieve the text that should be highlighted on assets */
	FText GetHighlightedText() const;

	/** Called to work out whether the import button should be enabled */
	bool IsImportEnabled() const;

	/** Called to retrieve the text that should be in the import tooltip */
	FText GetImportTooltipText() const;

	/** Called when a containing tab is closing, if there is one */
	void OnContainingTabSavingVisualState() const;

	/** Called when a containing tab is closed, if there is one */
	void OnContainingTabClosed(TSharedRef<SDockTab> DockTab);

	/** Called when a containing tab is activated, if there is one */
	void OnContainingTabActivated(TSharedRef<SDockTab> DockTab, ETabActivationCause InActivationCause);

	/** Loads settings from config based on the browser's InstanceName*/
	void LoadSettings(const FName& InstanceName);

	/** Handler for when the sources were changed */
	void SourcesChanged(const TArray<FString>& SelectedPaths, const TArray<FCollectionNameType>& SelectedCollections);

	/** Handler for when a folder has been entered in the path view */
	void FolderEntered(const FString& FolderPath);

	/** Handler for when a path has been selected in the path view */
	void PathSelected(const FString& FolderPath);
#if ECB_WIP_FAVORITE
	/** Handler for when a path has been selected in the favorite path view */
	void FavoritePathSelected(const FString& FolderPath);

	/** Gets the current path if one exists, otherwise returns empty string. */
	FString GetCurrentPath() const;
#endif
	/** Get the asset tree context menu */
	TSharedRef<FExtender> GetPathContextMenuExtender(const TArray<FString>& SelectedPaths) const;
#if ECB_WIP_COLLECTION
	/** Handler for when a collection has been selected in the collection view */
	void CollectionSelected(const FCollectionNameType& SelectedCollection);

	/** Handler for when the sources were changed by the path picker */
	void PathPickerPathSelected(const FString& FolderPath);

	/** Handler for when the sources were changed by the path picker collection view */
	void PathPickerCollectionSelected(const FCollectionNameType& SelectedCollection);

	/** Sets the state of the browser to the one described by the supplied history data */
	void OnApplyHistoryData(const FHistoryData& History);

	/** Updates the supplied history data with current information */
	void OnUpdateHistoryData(FHistoryData& History) const;
#endif

#if ECB_FEA_SEARCH
	/** Called when the editable text needs to be set or cleared */
	void SetSearchBoxText(const FText& InSearchText);

	/** Called by the editable text control when the search text is changed by the user */
	void OnSearchBoxChanged(const FText& InSearchText);

	/** Called by the editable text control when the user commits a text change */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);
#endif

#if ECB_LEGACY
	/** Should the "Save Search" button be enabled? */
	bool IsSaveSearchButtonEnabled() const;

	/** Open the menu to let you save the current search text as a dynamic collection */
	FReply OnSaveSearchButtonClicked();

	/** Called when a crumb in the path breadcrumb trail or menu is clicked */
	void OnPathClicked(const FString& CrumbData);

	/** Called when item in the path delimiter arrow menu is clicked */
	void OnPathMenuItemClicked(FString ClickedPath);

	/** 
	 * Populates the delimiter arrow with a menu of directories under the current directory that can be navigated to
	 * 
	 * @param CrumbData	The current directory path
	 * @return The directory menu
	 */
	TSharedPtr<SWidget> OnGetCrumbDelimiterContent(const FString& CrumbData) const;

	/** Gets the content for the path picker combo button */
	TSharedRef<SWidget> GetPathPickerContent();
#endif
	/** Handle creating a context menu for the "Add New" button */
	TSharedRef<SWidget> MakeAddNewContextMenu(bool bShowGetContent, bool bShowImport);
#if ECB_LEGACY
	/** Called to work out whether the import button should be enabled */
	bool IsAddNewEnabled() const;

	/** Gets the tool tip for the "Add New" button */
	FText GetAddNewToolTipText() const;
#endif
	/** Makes the filters menu */
	TSharedRef<SWidget> MakeAddFilterMenu();

	/** Builds the context menu for the filter list area. */
	TSharedPtr<SWidget> GetFilterContextMenu();

	/** Imports uasset files. */
	void ImportAsset(const TArray<FExtAssetData>& InAssetDatas);

	/** Imports uasset files to sandbox folder. */
	void ImportAssetToSandbox(const TArray<FExtAssetData>& InAssetDatas);

	void ImportAssetFlatMode(const TArray<FExtAssetData>& InAssetDatas);

	/** Imports a new piece of content. */
	FReply HandleImportClicked();

	/** Imports a new piece of content. */
	FReply HandleImportToSandboxClicked();

	FReply HandleFlatImportClicked();

	/** Zip uasset with dependencies. */
	FReply HandleExportAssetClicked();

	/** Called to work out whether the zip button should be enabled */
	bool IsExportAssetEnabled() const;

	/** Zip uasset with dependencies. */
	FReply HandleZipExportClicked();

	bool IsValidateEnabled() const;

	FReply HandleValidateclicked();

#if ECB_FEA_IMPORT_OPTIONS
	/** Returns the foreground color for the import options */
	FSlateColor GetImportOptionsButtonForegroundColor() const;

	/** Handler for when the import options combo button is clicked */
	TSharedRef<SWidget> GetImportOptionsButtonContent();
#endif

	/** Handle add root content folder button clicked. */
	FReply HandleAddContentFolderClicked();

	/** Add root content folder. */
	void AddContentFolder();

	/** Handle add root content folder button clicked. */
	FReply HandleRemoveContentFoldersClicked();

	/** Add root content folder. */
	void RemoveContentFolders();

	/** If can remove content folders */
	bool CanRemoveContentFolders() const;

#if ECB_LEGACY
	/** Opens the add content dialog. */
	void OnAddContentRequested();
#endif
	/** Handler for when the selection set in the asset view has changed. */
	void OnAssetSelectionChanged(const FExtAssetData& SelectedAsset);
#if ECB_LEGACY
	/** Handler for when the user double clicks, presses enter, or presses space on an asset */
	void OnAssetsActivated(const TArray<FAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod);
#endif
	/** Handler for when an asset context menu has been requested. */
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FExtAssetData>& SelectedAssets);

#if ECB_WIP_LOCK
	/** Handler for clicking the lock button */
	FReply ToggleLockClicked();

	/** Gets the brush used on the lock button */
	const FSlateBrush* GetToggleLockImage() const;
#endif

	/** Gets the visibility by last frame width */
	EVisibility GetVisibilityByLastFrameWidth(float MinWidthToShow) const;

	/** Gets the visibility state of the asset tree */
	EVisibility GetSourcesViewVisibility() const;

	/** Gets the brush used to on the sources toggle button */
	const FSlateBrush* GetSourcesToggleImage() const;

	/** Handler for clicking the tree expand/collapse button */
	FReply SourcesViewExpandClicked();

	/** Gets the visibility of the path expander button */
	EVisibility GetPathExpanderVisibility() const;

#if ECB_FEA_REF_VIEWER
	/** Gets the visibility state of the asset tree */
	EVisibility GetReferencesViewerVisibility() const;

	/** Gets the brush used to on the sources toggle button */
	const FSlateBrush* GetDependencyViewerToggleImage() const;

	/** Handler for clicking the tree expand/collapse button */
	FReply ReferenceViewerExpandClicked();
#endif

#if ECB_WIP_HISTORY
	/** Handler for clicking the history back button */
	FReply BackClicked();

	/** Handler for clicking the history forward button */
	FReply ForwardClicked();

	/** Gets the tool tip text for the history back button */
	FText GetHistoryBackTooltip() const;

	/** Gets the tool tip text for the history forward button */
	FText GetHistoryForwardTooltip() const;

	/** True if the user may use the history back button */
	bool IsBackEnabled() const;

	/** True if the user may use the history forward button */
	bool IsForwardEnabled() const;
#endif

#if ECB_LEGACY
	/** Handler for SaveAll in folder */
	void HandleSaveAllCurrentFolderCommand() const;

	/** Handler for opening assets or folders */
	void HandleOpenAssetsOrFoldersCommandExecute();

	/** Handler for previewing assets */
	void HandlePreviewAssetsCommandExecute();

	/** Handler for clicking the directory up button */
	void HandleDirectoryUpCommandExecute();

	/** True if the user may use the directory up button */
	bool CanExecuteDirectoryUp() const;

	/** Gets the tool tip text for the directory up button */
	FText GetDirectoryUpTooltip() const;

	/** Gets the Visibility for the directory up buttons tooltip (hidden if empty) */
	EVisibility GetDirectoryUpToolTipVisibility() const;
#endif

	void HandleImportSelectedCommandExecute();

	void HandleToggleDependencyViewerCommandExecute();

#if ECB_WIP_BREADCRUMB
	/** Updates the breadcrumb trail to the current path */
	void UpdatePath();
#endif
	/** Handler for when a filter in the filter list has changed */
	void OnFilterChanged();
#if ECB_LEGACY
	/** Gets the text for the path label */
	FText GetPathText() const;

	/** Returns true if currently filtering by a source */
	bool IsFilteredBySource() const;
#endif

#if ECB_WIP_SYNC_ASSET
	/** Handler for when the context menu or asset view requests to find assets in the asset tree */
	void OnFindInAssetTreeRequested(const TArray<FAssetData>& AssetsToFind);
#endif

#if ECB_LEGACY
	/** Handler for when the user has committed a rename of an asset */
	void OnAssetRenameCommitted(const TArray<FAssetData>& Assets);

	/** Handler for when the asset context menu requests to rename an asset */
	void OnRenameRequested(const FAssetData& AssetData);

	/** Handler for when the asset context menu requests to rename a folder */
	void OnRenameFolderRequested(const FString& FolderToRename);

	/** Handler for when the path context menu has successfully deleted a folder */
	void OnOpenedFolderDeleted();

	/** Handler for when the asset context menu requests to refresh the asset view */
	void OnAssetViewRefreshRequested();

	/** Handles an on collection destroyed event */
	void HandleCollectionRemoved(const FCollectionNameType& Collection);

	/** Handles an on collection renamed event */
	void HandleCollectionRenamed(const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection);

	/** Handles an on collection updated event */
	void HandleCollectionUpdated(const FCollectionNameType& Collection);

	/** Handles a path removed event */
	void HandlePathRemoved(const FString& Path);
#endif

#if ECB_FEA_SEARCH
	/** Gets all suggestions for the asset search box */
	void OnAssetSearchSuggestionFilter(const FText& SearchText, TArray<FAssetSearchBoxSuggestion>& PossibleSuggestions, FText& SuggestionHighlightText) const;

	/** Combines the chosen suggestion with the active search text */
	FText OnAssetSearchSuggestionChosen(const FText& SearchText, const FString& Suggestion) const;

	/** Gets the dynamic hint text for the "Search Assets" search text box */
	FText GetSearchAssetsHintText() const;
#endif
	/** Delegate called when generating the context menu for a folder */
	TSharedPtr<SWidget> GetFolderContextMenu(const TArray<FString>& SelectedPaths, FContentBrowserMenuExtender_SelectedPaths InMenuExtender, FOnCreateNewFolder OnCreateNewFolder, bool bPathView);

#if ECB_WIP_DELEGATES
	/** Delegate called to get the current selection state */
	void GetSelectionState(TArray<FExtAssetData>& SelectedAssets, TArray<FString>& SelectedPaths);
#endif

	/** Handler for when "Clear Folder Selection" is selected */
	void ClearFolderSelection();

	/** Bind our UI commands */
	void BindCommands();

	/** Gets the visibility of the collection view */
	EVisibility GetCollectionViewVisibility() const;

	/** Gets the visibility of the favorites view */
	EVisibility GetFavoriteFolderVisibility() const;

#if ECB_FEA_SEARCH	
	/** The visibility of the search bar for the base path view*/
	EVisibility GetAlternateSearchBarVisibility() const;

	/** Called when Asset View Options "Search" options change */
	void HandleAssetViewSearchOptionsChanged();
#endif

	void HandleRequestShowChangeLog();

#if ECB_WIP_FAVORITE
	/** Toggles the favorite status of an array of folders*/
	void ToggleFolderFavorite(const TArray<FString>& FolderPaths);
#endif

#if ECB_DEBUG
	FReply HandlePrintCacheClicked();
	FReply HandleClearCacheClicked();
	FReply HandlePrintAssetDataClicked();
#endif

	void ShowPluginVersionChangeLog();

	/** Delegate called when an editor setting is changed */
	void HandleSettingChanged(FName PropertyName);

	void UpdateAssetReferenceSplitterOrientation();

	FSlateColor GetContentBrowserBorderBackgroundColor() const;
	FSlateColor GetToolbarBackgroundColor() const;
	FSlateColor GetSourceViewBackgroundColor() const;
	FSlateColor GetAssetViewBackgroundColor() const;


private:

	/** The tab that contains this browser */
	TWeakPtr<SDockTab> ContainingTab;

#if ECB_WIP_HISTORY
	/** The manager that keeps track of history data for this browser */
	FHistoryManager HistoryManager;
#endif
	/** A helper class to manage asset context menu options */
	TSharedPtr<class FAssetContextMenu> AssetContextMenu;

	/** The context menu manager for the path view */
	TSharedPtr<class FPathContextMenu> PathContextMenu;

	/** The asset tree widget */
	TSharedPtr<SExtPathView> PathViewPtr;

#if ECB_WIP_FAVORITE
	/** The favorites tree widget */
	TSharedPtr<class SFavoritePathView> FavoritePathViewPtr;
#endif

#if ECB_WIP_COLLECTION
	/** The collection widget */
	TSharedPtr<SCollectionView> CollectionViewPtr;
#endif
	/** The asset view widget */
	TSharedPtr<SExtAssetView> AssetViewPtr;

#if ECB_WIP_BREADCRUMB
	/** The breadcrumb trail representing the current path */
	TSharedPtr< SBreadcrumbTrail<FString> > PathBreadcrumbTrail;
#endif

#if ECB_FEA_SEARCH
	/** The text box used to search for assets */
	TSharedPtr<SAssetSearchBox> SearchBoxPtr;

	/** The text filter to use on the assets */
	TSharedPtr< FFrontendFilter_Text > TextFilter;

	/** When viewing a dynamic collection, the active search query will be stashed in this variable so that it can be restored again later */
	TOptional<FText> StashedSearchBoxText;
#endif
	/** The filter list */
	TSharedPtr<SExtFilterList> FilterListPtr;

	/** The filter list */
	TSharedPtr<SExtDependencyViewer> ReferenceViewerPtr;

#if ECB_WIP_PATHPICKER
	/** The path picker */
	TSharedPtr<SComboButton> PathPickerButton;
#endif

#if ECB_WIP_CACHEDB
	void HandleCacheDBFilePathPicked(const FString& PickedPath);
	FReply HandleLoadCacheDBclicked();
	FReply HandleSaveCacheDBClicked();
	FReply HandleSaveAndSwitchToCacheModeClicked();
	FReply HandlePurgeCacheDBClicked();
	//FReply HandleSwitchAndLoadCacheDBClicked();
	/** Callback for picking a path in the source directory picker. */
	void HandleSwitchAndLoadCacheDBClicked(const FString& PickedPath);
	FText GetSaveAndSwitchToCacheModeText() const;
	FText GetCacheStatusText() const;
	EVisibility GetCacheStatusBarVisibility() const;
#endif

#if ECB_FEA_IMPORT_OPTIONS
	/** The button that displays import options */
	TSharedPtr<SComboButton> ImportOptionsComboButton;
#endif

	/** The expanded state of the asset tree */
	bool bSourcesViewExpanded;

	/** The expanded state of the asset tree */
	bool bReferenceViewerExpanded;

	/** True if this browser is the primary content browser */
	bool bIsPrimaryBrowser;

	/** True if this content browser can be set to the primary browser. */
	bool bCanSetAsPrimaryBrowser;

	/** Unique name for this Content Browser. */
	FName InstanceName;

	/** True if source should not be changed from an outside source */
	bool bIsLocked;

	/** True if can remove selected content folders */
	bool bCanRemoveContentFolders;

	/** The list of FrontendFilters currently applied to the asset view */
	TSharedPtr<FExtAssetFilterCollectionType> FrontendFilters;

	/** Commands handled by this widget */
	TSharedPtr< FUICommandList > Commands;

	/** The splitter between the path & asset view */
	TSharedPtr<SSplitter> PathAssetSplitterPtr;

	/** The splitter between the path & collection view */
	TSharedPtr<SSplitter> PathCollectionSplitterPtr;

	/** The splitter between the path & favorite view */
	TSharedPtr<SSplitter> PathFavoriteSplitterPtr;

	/** The splitter between the asset & dependency view */
	TSharedPtr<SSplitter> AssetReferenceSplitterPtr;

	/** True if we should always show collections in this Content Browser, ignoring the view settings */
	bool bAlwaysShowCollections;

public: 

	/** The section of EditorPerProjectUserSettings in which to save content browser settings */
	static const FString SettingsIniSection;

private:
	TSharedRef<SWidget> CreateChangeLogWidget();
	TSharedPtr<class IDocumentationPage > AboutPage;
	bool bShowChangelog = false;

	float WidthLastFrame;
};

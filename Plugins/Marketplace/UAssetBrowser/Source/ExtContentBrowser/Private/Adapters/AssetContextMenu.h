// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserDelegates.h"
#include "SourcesData.h"

class UToolMenu;
struct FExtAssetData;
class SExtAssetView;
class FUICommandList;
class SWindow;

/**
 * Provide context menu support for asset view and asset items
 */
class FAssetContextMenu : public TSharedFromThis<FAssetContextMenu>
{
public:
	/** Constructor */
	FAssetContextMenu(const TWeakPtr<SExtAssetView>& InAssetView);

	/** Binds the commands used by the asset view context menu to the content browser command list */
	void BindCommands(TSharedPtr< FUICommandList >& Commands);

	/** Makes the context menu widget */
	TSharedRef<SWidget> MakeContextMenu(const TArray<FExtAssetData>& SelectedAssets, const FSourcesData& InSourcesData, TSharedPtr< FUICommandList > InCommandList);

	/** Updates the list of currently selected assets to those passed in */
	void SetSelectedAssets(const TArray<FExtAssetData>& InSelectedAssets);

	/** Delegate for when the context menu requests a rename */
	//void SetOnFindInAssetTreeRequested(const FOnFindInAssetTreeRequested& InOnFindInAssetTreeRequested);

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE_OneParam(FOnRenameRequested, const FAssetData& /*AssetToRename*/);
	void SetOnRenameRequested(const FOnRenameRequested& InOnRenameRequested);

	/** Delegate for when the context menu requests a rename of a folder */
	DECLARE_DELEGATE_OneParam(FOnRenameFolderRequested, const FString& /*FolderToRename*/);
	void SetOnRenameFolderRequested(const FOnRenameFolderRequested& InOnRenameFolderRequested);

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE_OneParam(FOnDuplicateRequested, const TWeakObjectPtr<UObject>& /*OriginalObject*/);
	void SetOnDuplicateRequested(const FOnDuplicateRequested& InOnDuplicateRequested);

	/** Delegate for when the context menu requests an asset view refresh */
	DECLARE_DELEGATE(FOnAssetViewRefreshRequested);
	void SetOnAssetViewRefreshRequested(const FOnAssetViewRefreshRequested& InOnAssetViewRefreshRequested);

	/** Handler to check to see if a rename command is allowed */
	bool CanExecuteRename() const;

	/** Handler to check to see if a delete command is allowed */
	bool CanExecuteDelete() const;

	/** Handler for Delete */
	void ExecuteDelete();

	/** Handler to check to see if a reload command is allowed */
	bool CanExecuteReload() const;

	/** Handler for Reload */
	void ExecuteReload();

	/** Handler to check to see if "Save Asset" can be executed */
	bool CanExecuteSaveAsset() const;

	/** Handler for when "Save Asset" is selected */
	void ExecuteSaveAsset();

private:
	struct FSourceAssetsState
	{
		TSet<FName> SelectedAssets;
		TSet<FName> CurrentAssets;
	};

	struct FLocalizedAssetsState
	{
		FCulturePtr Culture;
		TSet<FName> NewAssets;
		TSet<FName> CurrentAssets;
	};

private:
	/** Helper to load selected assets and sort them by UClass */
	void GetSelectedAssetsByClass(TMap<UClass*, TArray<UObject*> >& OutSelectedAssetsByClass) const;

	/** Helper to collect resolved filepaths for all selected assets */
	void GetSelectedAssetSourceFilePaths(TArray<FString>& OutFilePaths, TArray<FString>& OutUniqueSourceFileLabels, int32 &OutValidSelectedAssetCount) const;

	/** Handler to check to see if a imported asset actions should be visible in the menu */
	bool AreImportedAssetActionsVisible() const;

	/** Handler to check to see if imported asset actions are allowed */
	bool CanExecuteImportedAssetActions(const TArray<FString> ResolvedFilePaths) const;

	/** Handler for FindInExplorer */
	void ExecuteFindSourceInExplorer(const TArray<FString> ResolvedFilePaths);

	/** Handler for OpenInExternalEditor */
	void ExecuteOpenInExternalEditor(const TArray<FString> ResolvedFilePaths);

private:

	/** Registers all unregistered menus in the hierarchy for a class */
	static void RegisterMenuHierarchy(UClass* InClass);

	/** Adds options to menu */
	void AddMenuOptions(UToolMenu* InMenu);

	/** Adds asset type-specific menu options to a menu builder. Returns true if any options were added. */
	bool AddAssetTypeMenuOptions(UToolMenu* Menu, bool bHasObjectsSelected);

	/** Adds asset type-specific menu options to a menu builder. Returns true if any options were added. */
	bool AddImportedAssetMenuOptions(UToolMenu* Menu);
	
	/** Adds common menu options to a menu builder. Returns true if any options were added. */
	bool AddCommonMenuOptions(UToolMenu* Menu);

	/** Adds explore menu options to a menu builder. */
	void AddExploreMenuOptions(UToolMenu* Menu);

	/** Adds Asset Actions sub-menu to a menu builder. */
	void MakeAssetActionsSubMenu(UToolMenu* Menu);

	/** Handler to check to see if "Asset Actions" are allowed */
	bool CanExecuteAssetActions() const;

	/** Find the given assets in the Content Browser */
	void ExecuteFindInAssetTree(TArray<FName> InAssets);

	/** Open the given assets in their respective editors */
	void ExecuteOpenEditorsForAssets(TArray<FName> InAssets);

	/** Adds asset reference menu options to a menu builder. Returns true if any options were added. */
	bool AddReferenceMenuOptions(UToolMenu* Menu);

	/** Adds copy file path menu options to a menu builder. Returns true if any options were added. */
	bool AddCopyMenuOptions(UToolMenu* Menu);

	/** Adds menu options related to working with collections */
	bool AddCollectionMenuOptions(FMenuBuilder& MenuBuilder);

	/** Handler for when sync to asset tree is selected */
	void ExecuteSyncToAssetTree();

	/** Handler for when find in explorer is selected */
	void ExecuteFindInExplorer();

	/** Handler for when validate asset is selected */
	void ExecuteValidateAsset();
	void ExecuteRevalidateAsset();

	/** Handler for when import asset is selected */
	void ExecuteImportAsset();

	/** Handler for when flat import asset is selected */
	void ExecuteFlatImportAsset();

	/** Handler for when direct copy asset is selected */
	void ExecuteDirectCopyAsset();

	void ExecuteReparseAsset();

	/** Handler for when sync in content browser is selected */
	void ExecuteSyncToContentBrowser();

	/** Handler for when create using asset is selected */
	void ExecuteCreateBlueprintUsing();

	/** Handler for when "Find in World" is selected */
	void ExecuteFindAssetInWorld();

	/** Handler for when "Property Matrix..." is selected */
	void ExecutePropertyMatrix();

	/** Handler for when "Show MetaData" is selected */
	void ExecuteShowAssetMetaData();

	/** Handler for when "Edit Asset" is selected */
	void ExecuteEditAsset();

	/** Handler for when "Diff Selected" is selected */
	void ExecuteDiffSelected() const;

	/** Handler for when "Migrate Asset" is selected */
	void ExecuteMigrateAsset();

	/** Handler for GoToAssetCode */
	void ExecuteGoToCodeForAsset(UClass* SelectedClass);

	/** Handler for GoToAssetDocs */
	void ExecuteGoToDocsForAsset(UClass* SelectedClass);

	/** Handler for GoToAssetDocs */
	void ExecuteGoToDocsForAsset(UClass* SelectedClass, const FString ExcerptSection);

	/** Handler for CopyReference */
	void ExecuteCopyReference();

	/** Handler for CopyFilePath */
	void ExecuteCopyFilePath();

	/** Handler to copy the given text to the clipboard */
	void ExecuteCopyTextToClipboard(FString InText);

	/** Handler for showing the cached list of localized texts stored in the package header */
	void ExecuteShowLocalizationCache(const FString InPackageFilename);

	/** Handler for Dump */
	void ExecuteDumpExport();

	/** Handler to check to see if dump export command is allowed */
	bool CanExecuteDumpExport() const;

	/** Handler for Export */
	void ExecuteExport();

	/** Handler for Bulk Export */
	void ExecuteBulkExport();

	/** Handler for when "Remove from collection" is selected */
	void ExecuteRemoveFromCollection();

	/** Handler for when source control is disabled */
	void ExecuteEnableSourceControl();

	/** Handler to remove a single ChunkID assignment from a selection of assets */
	void ExecuteRemoveChunkID(int32 ChunkID);

	/** Handler to export the selected asset(s) to experimental text format */
	void ExportSelectedAssetsToText();

	/** Handler to check if we can export the selected asset(s) to experimental text format */
	bool CanExportSelectedAssetsToText() const;

	/** Handler to check to see if a sync to asset tree command is allowed */
	bool CanExecuteSyncToAssetTree() const;

	/** Handler to check to see if a sync to content browser command is allowed */
	bool CanExecuteSyncToContentBrowser() const;

	/** Handler to check to see if a find in explorer command is allowed */
	bool CanCopyFilePath() const;

	/** Handler to check to see if a find in explorer command is allowed */
	bool CanExecuteFindInExplorer() const;

	/** Handler to check to see if validate asset command is allowed */
	bool CanExecuteValidateAsset() const;
	bool CanExecuteRevalidateAsset() const;

	/** Handler to check to see if import asset command is allowed */
	bool CanExecuteImportAsset() const;

	/** Handler to check to see if import asset command is allowed */
	bool CanExecuteFlatImportAsset() const;

	/** Handler to check to see if direct copy asset command is allowed */
	bool CanExecuteDirectCopyAsset() const;

	bool CanExecuteReparseAsset() const;

	/** Handler to check if we can create blueprint using selected asset */
	bool CanExecuteCreateBlueprintUsing() const;

	/** Handler to check to see if a find in world command is allowed */
	bool CanExecuteFindAssetInWorld() const;

	/** Handler to check to see if a properties command is allowed */
	bool CanExecuteProperties() const;

	/** Handler to check to see if a property matrix command is allowed */
	bool CanExecutePropertyMatrix(FText& OutErrorMessage) const;
	bool CanExecutePropertyMatrix() const;

	/** Handler to check to see if a "Show MetaData" command is allowed */
	bool CanExecuteShowAssetMetaData() const;

	/** Handler to check to see if a "Remove from collection" command is allowed */
	bool CanExecuteRemoveFromCollection() const;

	/** Handler to check to see if "Diff Selected" can be executed */
	bool CanExecuteDiffSelected() const;

	/** Handler to check to see if "Capture Thumbnail" can be executed */
	bool CanExecuteCaptureThumbnail() const;

	/** Handler to check to see if "Clear Thumbnail" can be executed */
	bool CanExecuteClearThumbnail() const;

	/** Handler to check to see if "Clear Thumbnail" should be visible */
	bool CanClearCustomThumbnails() const;

	/** Helper function to gather the package names of all selected assets */
	void GetSelectedPackageNames(TArray<FString>& OutPackageNames) const;

	/** Helper function to gather the packages containing all selected assets */
	void GetSelectedPackages(TArray<UPackage*>& OutPackages) const;

	/** Generates tooltip for the Property Matrix menu option */
	FText GetExecutePropertyMatrixTooltip() const;

private:

	/** Registers the base context menu for assets */
	void RegisterContextMenu(const FName MenuName);

	/** Generates a list of selected assets in the content browser */
	void GetSelectedAssets(TArray<UObject*>& Assets, bool SkipRedirectors) const;

	TArray<FExtAssetData> SelectedAssets;
	FSourcesData SourcesData;

	/** The asset view this context menu is a part of */
	TWeakPtr<SExtAssetView> AssetView;

	//FOnFindInAssetTreeRequested OnFindInAssetTreeRequested;
	FOnRenameRequested OnRenameRequested;
	FOnRenameFolderRequested OnRenameFolderRequested;
	FOnDuplicateRequested OnDuplicateRequested;
	FOnAssetViewRefreshRequested OnAssetViewRefreshRequested;

	/** Cached CanExecute vars */
	bool bAtLeastOneNonRedirectorSelected;
	bool bAtLeastOneClassSelected;
	bool bCanExecuteSCCMerge;
	bool bCanExecuteSCCCheckOut;
	bool bCanExecuteSCCOpenForAdd;
	bool bCanExecuteSCCCheckIn;
	bool bCanExecuteSCCHistory;
	bool bCanExecuteSCCRevert;
	bool bCanExecuteSCCSync;
	/** */
	int32 ChunkIDSelected;
};

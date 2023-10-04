// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
//#include "Editor/ContentBrowser/Private/NewAssetOrClassContextMenu.h"

class FExtender;
class FMenuBuilder;
class SWidget;
class SWindow;

class FPathContextMenu : public TSharedFromThis<FPathContextMenu>
{
public:
	/** Constructor */
	FPathContextMenu(const TWeakPtr<SWidget>& InParentContent);

	/** Delegate for when the context menu requests a rename of a folder */
	DECLARE_DELEGATE_OneParam(FOnRenameFolderRequested, const FString& /*FolderToRename*/);
	void SetOnRenameFolderRequested(const FOnRenameFolderRequested& InOnRenameFolderRequested);

	/** Delegate for when the context menu has successfully deleted a folder */
	DECLARE_DELEGATE(FOnFolderDeleted)
	void SetOnFolderDeleted(const FOnFolderDeleted& InOnFolderDeleted);

	/** Delegate for when the context menu has successfully toggled the favorite status of a folder */
	DECLARE_DELEGATE_OneParam(FOnFolderFavoriteToggled, const TArray<FString>& /*FoldersToToggle*/)
	void SetOnFolderFavoriteToggled(const FOnFolderFavoriteToggled& InOnFolderFavoriteToggled);

	/** Gets the currently selected paths */
	const TArray<FString>& GetSelectedPaths() const;

	/** Sets the currently selected paths */
	void SetSelectedPaths(const TArray<FString>& InSelectedPaths);

	/** Makes the asset tree context menu extender */
	TSharedRef<FExtender> MakePathViewContextMenuExtender(const TArray<FString>& InSelectedPaths);

	/** Makes the asset tree context menu widget */
	void MakePathViewContextMenu(FMenuBuilder& MenuBuilder);

	/** Handler to check to see if creating a new asset is allowed */
	bool CanCreateAsset() const;

	/** Makes the new asset submenu */
	void MakeNewAssetSubMenu(FMenuBuilder& MenuBuilder);

	/** Makes the set color submenu */
	void MakeSetColorSubMenu(FMenuBuilder& MenuBuilder);

	/** Handler for when "Migrate Folder" is selected */
	void ExecuteMigrateFolder();

	/** Handler for when "Explore" is selected */
	void ExecuteExplore();

	/** Handler for when "Apply Project Folder Colors" is selected */
	void ExecuteApplyProjectFolderColors();

	/** Handler for when "ValidateAssets" is selected */
	void ExecuteValidateAssetsInFolder();

	/** Handler to check to see if a rescan content folder command is allowed */
	bool CanExecuteRescanFolder() const;

	/** Handler to check to see if a import content folder command is allowed */
	bool CanExecuteImportFolder() const;

	/** Handler to check to see if it's a project folder */
	bool CanExecuteApplyProjectFolderColors() const;

	/** Handler for Rescan Content Folder */
	void ExecuteRescanFolder();

	/** Handler for Import Content Folder */
	void ExecuteImportFolder();

	/** Handler to check to see if a remove content folder command is allowed */
	bool CanExecuteRootDirsActions() const;

	/** Handler for Reload Content Folder */
	void ExecuteReloadRootFolder();

	/** Handler for Remove Content Folder */
	void ExecuteRemoveRootFolder();

	/** Handler for Add Content Folder */
	void ExecuteAddRootFolder();

	/** Handler for export Content Folder list*/
	void ExecuteExportRootFolderList();

	void LoadRootFolderList(bool bReplaceCurrent);

	/** Handler for load and merge Content Folder list*/
	void ExecuteLoadAndMergeRootFolderList();
		
	/** Handler for load and replace Content Folder list*/
	void ExecuteLoadAndReplaceRootFolderList();

	/** Handler to check to see if a rename command is allowed */
	bool CanExecuteRename() const;

	/** Handler for Rename */
	void ExecuteRename();

	/** Handler to check to see if a delete command is allowed */
	bool CanExecuteDelete() const;

	/** Handler for Delete */
	void ExecuteDelete();

	/** Handler for when reset color is selected */
	void ExecuteResetColor();

	/** Handler for when new or set color is selected */
	void ExecutePickColor();

	/** Handler for favoriting */
	void ExecuteFavorite();

	/** Handler for when "Save" is selected */
	void ExecuteSaveFolder();

	/** Handler for when "Resave" is selected */
	void ExecuteResaveFolder();

	/** Handler for when "Fix up Redirectors in Folder" is selected */
	void ExecuteFixUpRedirectorsInFolder();

	/** Handler for when "Delete" is selected and the delete was confirmed */
	FReply ExecuteDeleteFolderConfirmed();

private:
	/** Initializes some variable used to in "CanExecute" checks that won't change at runtime or are too expensive to check every frame. */
	void CacheCanExecuteVars();

	/** Returns a list of names of packages in all selected paths in the sources view */
	void GetPackageNamesInSelectedPaths(TArray<FString>& OutPackageNames) const;

	/** Gets the first selected path, if it exists */
	FString GetFirstSelectedPath() const;

	/** Checks to see if any of the selected paths use custom colors */
	bool SelectedHasCustomColors() const;

	/** Callback when the color picker dialog has been closed */
	void NewColorComplete(const TSharedRef<SWindow>& Window);

	/** Callback when the color is picked from the set color submenu */
	FReply OnColorClicked( const FLinearColor InColor );

	/** Resets the colors of the selected paths */
	void ResetColors();

private:
	TArray<FString> SelectedPaths;
	TWeakPtr<SWidget> ParentContent;
	FOnRenameFolderRequested OnRenameFolderRequested;
	FOnFolderDeleted OnFolderDeleted;
	FOnFolderFavoriteToggled OnFolderFavoriteToggled;

	bool bCanExecuteRootDirsActions;
	bool bHasSelectedPath;

	bool bRescanFolderAndAssetsInSelecteFolder = false;
};

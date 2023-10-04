// Copyright 2017-2021 marynate. All Rights Reserved.

#include "SExtPathView.h"
#include "ExtContentBrowser.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtContentBrowserUtils.h"
#include "ExtPackageUtils.h"
#include "ExtPathViewTypes.h"
#include "ExtSourcesViewWidgets.h"
#include "DragDropHandler.h"
#include "Adapters/SourcesData.h"

#include "Layout/WidgetPath.h"
#include "Application/SlateApplicationBase.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "EditorStyleSet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "DragAndDrop/AssetDragDropOp.h"

#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"

#if ECB_WIP_HISTORY
#include "HistoryManager.h"
#endif

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

SExtPathView::~SExtPathView()
{
#if ECB_LEGACY
	// Unsubscribe from content path events
	FPackageName::OnContentPathMounted().RemoveAll( this );
	FPackageName::OnContentPathDismounted().RemoveAll( this );

	// Unsubscribe from class events
	if ( bAllowClassesFolder )
	{
		TSharedRef<FNativeClassHierarchy> NativeClassHierarchy = FContentBrowserSingleton::Get().GetNativeClassHierarchy();
		NativeClassHierarchy->OnClassHierarchyUpdated().RemoveAll( this );
	}

	// Unsubscribe from folder population events
	{
		TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();
		EmptyFolderVisibilityManager->OnFolderPopulated().RemoveAll(this);
	}

	// Load the asset registry module to stop listening for updates
	FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
	if(AssetRegistryModule)
	{
		AssetRegistryModule->Get().OnPathAdded().RemoveAll(this);
		AssetRegistryModule->Get().OnPathRemoved().RemoveAll(this);
		AssetRegistryModule->Get().OnFilesLoaded().RemoveAll(this);
	}
#endif

	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathAdded().RemoveAll(this);
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathRemoved().RemoveAll(this);
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathUpdated().RemoveAll(this);
	FExtContentBrowserSingleton::GetAssetRegistry().OnFolderStartGathering().RemoveAll(this);
	FExtContentBrowserSingleton::GetAssetRegistry().OnFolderFinishGathering().RemoveAll(this);

	SearchBoxFolderFilter->OnChanged().RemoveAll( this );
}

void SExtPathView::Construct( const FArguments& InArgs )
{
	OnPathSelected = InArgs._OnPathSelected;
	bAllowContextMenu = InArgs._AllowContextMenu;
	OnGetFolderContextMenu = InArgs._OnGetFolderContextMenu;
	OnGetPathContextMenuExtender = InArgs._OnGetPathContextMenuExtender;
	bAllowClassesFolder = InArgs._AllowClassesFolder;
	PreventTreeItemChangedDelegateCount = 0;
	TreeTitle = LOCTEXT("AssetTreeTitle", "Asset Tree");
	PluginVersionText = FExtContentBrowserSingleton::GetPluginVersionText();
	if ( InArgs._FocusSearchBoxWhenOpened )
	{
		RegisterActiveTimer( 0.f, FWidgetActiveTimerDelegate::CreateSP( this, &SExtPathView::SetFocusPostConstruct ) );
	}

	// Listen for when view settings are changed
	UExtContentBrowserSettings::OnSettingChanged().AddSP(this, &SExtPathView::HandleSettingChanged);

	//Setup the SearchBox filter
	SearchBoxFolderFilter = MakeShareable( new FolderTextFilter( FolderTextFilter::FItemToStringArray::CreateSP( this, &SExtPathView::PopulateFolderSearchStrings ) ) );
	SearchBoxFolderFilter->OnChanged().AddSP( this, &SExtPathView::FilterUpdated );

	// Listen to find out when new game content paths are mounted or dismounted, so that we can refresh our root set of paths
	FPackageName::OnContentPathMounted().AddSP( this, &SExtPathView::OnContentPathMountedOrDismounted );
	FPackageName::OnContentPathDismounted().AddSP( this, &SExtPathView::OnContentPathMountedOrDismounted );
#if ECB_LEGACY
	// Listen to find out when the available classes are changed, so that we can refresh our paths
	if ( bAllowClassesFolder )
	{
		TSharedRef<FNativeClassHierarchy> NativeClassHierarchy = FContentBrowserSingleton::Get().GetNativeClassHierarchy();
		NativeClassHierarchy->OnClassHierarchyUpdated().AddSP( this, &SExtPathView::OnClassHierarchyUpdated );
	}

	// Listen to find out when previously empty paths are populated with content
	{
		TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();
		EmptyFolderVisibilityManager->OnFolderPopulated().AddSP(this, &SExtPathView::OnFolderPopulated);
	}
#endif
	if (!TreeViewPtr.IsValid())
	{
		SAssignNew(TreeViewPtr, STreeView< TSharedPtr<FTreeItem> >)
			.TreeItemsSource(&TreeRootItems)
			.OnGenerateRow(this, &SExtPathView::GenerateTreeRow)
			.OnItemScrolledIntoView(this, &SExtPathView::TreeItemScrolledIntoView)
			.ItemHeight(18)
			.SelectionMode(InArgs._SelectionMode)
			.OnSelectionChanged(this, &SExtPathView::TreeSelectionChanged)
			.OnExpansionChanged(this, &SExtPathView::TreeExpansionChanged)
			.OnGetChildren(this, &SExtPathView::GetChildrenForTree)
			.OnSetExpansionRecursive(this, &SExtPathView::SetTreeItemExpansionRecursive)
			.OnContextMenuOpening(this, &SExtPathView::MakePathViewContextMenu)
			.ClearSelectionOnClick(false)
			.HighlightParentNodesForSelection(true);
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		// Search >>
#if ECB_FOLD
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 1, 0, 3)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				InArgs._SearchContent.Widget
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(SearchBoxPtr, SSearchBox)
				.Visibility(InArgs._SearchBarVisibility)
				.HintText( LOCTEXT( "AssetTreeSearchBoxHint", "Search Folders" ) )
				.OnTextChanged( this, &SExtPathView::OnAssetTreeSearchBoxChanged )
				.OnTextCommitted( this, &SExtPathView::OnAssetTreeSearchBoxCommitted )
			]
		]
#endif	// Search <<

		// Tree Section >>
#if ECB_FOLD

#if 0
		// Tree title
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(1.f)
			.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.1f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(4, 0, 0, 0)
				[
					SNew(STextBlock)
					.Font( FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont") )
					//.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
					.Text(this, &SExtPathView::GetTreeTitle)
					//.Visibility(InArgs._ShowTreeTitle ? EVisibility::Visible : EVisibility::Collapsed)
					.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(1.f)
			.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
		]
#endif

		// Separator
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(SSeparator)
			.Visibility( ( InArgs._ShowSeparator) ? EVisibility::Visible : EVisibility::Collapsed )
		]
			
		// Tree
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			TreeViewPtr.ToSharedRef()
		]

		// Tree title
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2, 0, 0)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(1.f)
			.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.1f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(2)
			.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.Padding(4, 0, 0, 0)
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.4f))
					.Text(this, &SExtPathView::GetTreeTitle)
					.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(1.f)
			.Visibility(this, &SExtPathView::GetTreeTitleVisibility)
		]
#endif	// Tree Section <<

#if ECB_TODO
		// Version
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2, 2, 0, 1)
		.HAlign(HAlign_Left)
		[
			SNew (SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text(this, &SExtPathView::GetPluginVersionText)
				.ColorAndOpacity(FLinearColor::Gray)
			]
		]
#endif
	];

#if ECB_LEGACY
	// Load the asset registry module to listen for updates
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnPathAdded().AddSP( this, &SExtPathView::OnAssetRegistryPathAdded );
	AssetRegistryModule.Get().OnPathRemoved().AddSP( this, &SExtPathView::OnAssetRegistryPathRemoved );
	AssetRegistryModule.Get().OnFilesLoaded().AddSP( this, &SExtPathView::OnAssetRegistrySearchCompleted );
#endif
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathAdded().AddSP(this, &SExtPathView::OnAssetRegistryRootPathAdded);
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathRemoved().AddSP(this, &SExtPathView::OnAssetRegistryRootPathRemoved);
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathUpdated().AddSP(this, &SExtPathView::OnAssetRegistryRootPathUpdated);
	FExtContentBrowserSingleton::GetAssetRegistry().OnFolderStartGathering().AddSP(this, &SExtPathView::OnAssetRegistryFolderStartGathering);
	FExtContentBrowserSingleton::GetAssetRegistry().OnFolderFinishGathering().AddSP(this, &SExtPathView::OnAssetRegistryFolderFinishGathering);

	// Add all paths currently gathered from the asset registry
	Populate();

	// Always expand the game root initially
	static const FString GameRootName = TEXT("Game");
	for ( auto RootIt = TreeRootItems.CreateConstIterator(); RootIt; ++RootIt )
	{
		if ( (*RootIt)->FolderName == GameRootName )
		{
			TreeViewPtr->SetItemExpansion(*RootIt, true);
		}
	}
}

void SExtPathView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
#if ECB_FEA_ASYNC_FOLDER_DISCOVERY
	if (FExtContentBrowserSingleton::GetAssetRegistry().GetAndTrimFolderGatherResult())
	{
		Populate();
	}
#endif
}

void SExtPathView::SetSelectedPaths(const TArray<FString>& Paths)
{
	if ( !ensure(TreeViewPtr.IsValid()) )
	{
		return;
	}

	if ( !SearchBoxPtr->GetText().IsEmpty() )
	{
		// Clear the search box so the selected paths will be visible
		SearchBoxPtr->SetText( FText::GetEmpty() );
	}

	// Prevent the selection changed delegate since the invoking code requested it
	FScopedPreventTreeItemChangedDelegate DelegatePrevention( SharedThis(this) );

	// If the selection was changed before all pending initial paths were found, stop attempting to select them
	PendingInitialPaths.Empty();

	// Clear the selection to start, then add the selected paths as they are found
	TreeViewPtr->ClearSelection();

	TArray<FString> RootContentPaths;
	FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPaths(RootContentPaths);

	for (int32 PathIdx = 0; PathIdx < Paths.Num(); ++PathIdx)
	{
		const FString& Path = Paths[PathIdx];

		TArray<FString> PathItemList;
		GetPathItemList(Path, RootContentPaths, PathItemList, /*bIncludeRootPath*/true);

		if ( PathItemList.Num() )
		{
			// There is at least one element in the path
			TArray<TSharedPtr<FTreeItem>> TreeItems;

			// Find the first item in the root items list
			for ( int32 RootItemIdx = 0; RootItemIdx < TreeRootItems.Num(); ++RootItemIdx )
			{
				if ( TreeRootItems[RootItemIdx]->FolderPath == PathItemList[0] )
				{
					// Found the first item in the path
					TreeItems.Add(TreeRootItems[RootItemIdx]);
					break;
				}
			}

			// If found in the root items list, try to find the childmost item matching the path
			if ( TreeItems.Num() > 0 )
			{
				for ( int32 PathItemIdx = 1; PathItemIdx < PathItemList.Num(); ++PathItemIdx )
				{
					const FString& PathItemName = PathItemList[PathItemIdx];
					const TSharedPtr<FTreeItem> ChildItem = TreeItems.Last()->GetChild(PathItemName);

					if ( ChildItem.IsValid() )
					{
						// Update tree items list
						TreeItems.Add(ChildItem);
					}
					else
					{
						// Could not find the child item
						break;
					}
				}

				// Expand all the tree folders up to but not including the last one.
				for (int32 ItemIdx = 0; ItemIdx < TreeItems.Num() - 1; ++ItemIdx)
				{
					TreeViewPtr->SetItemExpansion(TreeItems[ItemIdx], true);
				}

				// Set the selection to the closest found folder and scroll it into view
				TreeViewPtr->SetItemSelection(TreeItems.Last(), true);
				TreeViewPtr->RequestScrollIntoView(TreeItems.Last());
			}
			else
			{
				// Could not even find the root path... skip
			}
		}
		else
		{
			// No path items... skip
		}
	}
}

void SExtPathView::ClearSelection()
{
	// Prevent the selection changed delegate since the invoking code requested it
	FScopedPreventTreeItemChangedDelegate DelegatePrevention( SharedThis(this) );

	// If the selection was changed before all pending initial paths were found, stop attempting to select them
	PendingInitialPaths.Empty();

	// Clear the selection to start, then add the selected paths as they are found
	TreeViewPtr->ClearSelection();
}

int32 SExtPathView::GetNumPathsSelected() const
{
	return TreeViewPtr->GetNumItemsSelected();
}

FString SExtPathView::GetSelectedPath() const
{
	TArray<TSharedPtr<FTreeItem>> Items = TreeViewPtr->GetSelectedItems();
	if (Items.Num() > 0)
	{
		FString& FolderPath = Items[0]->FolderPath;
		return FolderPath;
	}

	return FString();
}

TArray<FString> SExtPathView::GetSelectedPaths() const
{
	TArray<FString> RetArray;

	TArray<TSharedPtr<FTreeItem>> Items = TreeViewPtr->GetSelectedItems();
	for (int32 ItemIdx = 0; ItemIdx < Items.Num(); ++ItemIdx)
	{
		FString& FolderPath = Items[ItemIdx]->FolderPath;
		RetArray.Add(FolderPath);
	}
	return RetArray;
}

TSharedPtr<FTreeItem> SExtPathView::AddPath(const FString& InPath, const FString* RootPathPtr/* = nullptr*/, bool bUserNamed)
{
	if ( !ensure(TreeViewPtr.IsValid()) )
	{
		// No tree view for some reason
		return TSharedPtr<FTreeItem>();
	}

	FString Path = InPath;

	FString RootPath;
	if (RootPathPtr != nullptr)
	{
		RootPath = *RootPathPtr;
	}

	TArray<FString> RootContentPaths;
	FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPaths(RootContentPaths);

	TArray<FString> PathItemList;
	GetPathItemList(Path, /*{ RootPath }*/RootContentPaths, PathItemList, /*bIncludeRootPath*/ true);

	if ( PathItemList.Num() )
	{
		FString ContentRootPath = PathItemList[0];

		// There is at least one element in the path
		TSharedPtr<FTreeItem> CurrentItem;

		// Find the first item in the root items list
		for ( int32 RootItemIdx = 0; RootItemIdx < TreeRootItems.Num(); ++RootItemIdx )
		{
			if ( TreeRootItems[RootItemIdx]->FolderPath == ContentRootPath)
			{
				// Found the first item in the path
				CurrentItem = TreeRootItems[RootItemIdx];
				break;
			}
		}

		// Roots may or may not exist, add the root here if it doesn't
		if ( !CurrentItem.IsValid() )
		{
			CurrentItem = AddRootItem(ContentRootPath);
		}

		// Found or added the root item?
		if ( CurrentItem.IsValid() )
		{
			// Now add children as necessary
			const bool bDisplayEmpty = GetDefault<UExtContentBrowserSettings>()->DisplayEmptyFolders;
			const bool bDisplayDev = GetDefault<UExtContentBrowserSettings>()->GetDisplayDevelopersFolder();
			const bool bDisplayL10N = GetDefault<UExtContentBrowserSettings>()->GetDisplayL10NFolder();
			for ( int32 PathItemIdx = 1; PathItemIdx < PathItemList.Num(); ++PathItemIdx )
			{
				const FString& PathItemName = PathItemList[PathItemIdx];
				TSharedPtr<FTreeItem> ChildItem = CurrentItem->GetChild(PathItemName);

				// If it does not exist, Create the child item
				if ( !ChildItem.IsValid() )
				{
					const FString FolderName = PathItemName;
					const FString FolderPath = CurrentItem->FolderPath + "/" + PathItemName;

					ChildItem = MakeShareable( new FTreeItem(FText::FromString(FolderName), FolderName, FolderPath, RootPath, CurrentItem, bUserNamed) );
					CurrentItem->Children.Add(ChildItem);
					CurrentItem->RequestSortChildren();
					TreeViewPtr->RequestTreeRefresh();

					// If we have pending initial paths, and this path added the path, we should select it now
					if ( PendingInitialPaths.Num() > 0 && PendingInitialPaths.Contains(FolderPath) )
					{
						RecursiveExpandParents(ChildItem);
						TreeViewPtr->SetItemSelection(ChildItem, true);
						TreeViewPtr->RequestScrollIntoView(ChildItem);
					}
				}
				else
				{
					//If the child item does exist, ensure its folder path is correct (may differ when renaming parent folder)
					ChildItem->FolderPath = CurrentItem->FolderPath + "/" + PathItemName;
				}

				CurrentItem = ChildItem;
			}

			if ( bUserNamed && CurrentItem->Parent.IsValid() )
			{
				// If we were creating a new item, select it, scroll it into view, expand the parent
				RecursiveExpandParents(CurrentItem);
				TreeViewPtr->RequestScrollIntoView(CurrentItem);
				TreeViewPtr->SetSelection(CurrentItem);
			}
			else
			{
				CurrentItem->bNamingFolder = false;
			}

			// Root: Is Loading
			if (!CurrentItem->Parent.IsValid())
			{
				const bool bIsLoading = ExtContentBrowserUtils::IsFolderBackgroundGathering(CurrentItem->FolderPath);
				CurrentItem->bLoading = bIsLoading;
			}
			
			// Update Root's loading status
			if (CurrentItem->Parent.IsValid())
			{
				TSharedPtr<FTreeItem> RootOfCurrentItem = FindItemRecursive(RootPath);
				if (RootOfCurrentItem.IsValid())
				{
					const bool bIsLoading = ExtContentBrowserUtils::IsFolderBackgroundGathering(RootPath);
					RootOfCurrentItem->bLoading = bIsLoading;

					if (bIsLoading)
					{
						FName GatheringFolder = ExtContentBrowserUtils::GetCurrentGatheringFolder();
						if (GatheringFolder != NAME_None)
						{
							FString LoadingFolder = ExtContentBrowserUtils::GetCurrentGatheringFolder().ToString();
							if (LoadingFolder.StartsWith(RootPath))
							{
								LoadingFolder.RemoveFromStart(RootPath);
								LoadingFolder.RemoveFromStart(TEXT("/"));
								RootOfCurrentItem->LoadingStatus = LoadingFolder;
							}
						}
					}
				}
			}
		}

		return CurrentItem;
	}

	return TSharedPtr<FTreeItem>();
}

bool SExtPathView::RemovePath(const FString& Path)
{
	if ( !ensure(TreeViewPtr.IsValid()) )
	{
		// No tree view for some reason
		return false;
	}

	if ( Path.IsEmpty() )
	{
		// There were no elements in the path, cannot remove nothing
		return false;
	}

	// Find the folder in the tree
	TSharedPtr<FTreeItem> ItemToRemove = FindItemRecursive(Path);

	if ( ItemToRemove.IsValid() )
	{
		// Found the folder to remove. Remove it.
		if ( ItemToRemove->Parent.IsValid() )
		{
			// Remove the folder from its parent's list
			ItemToRemove->Parent.Pin()->Children.Remove(ItemToRemove);
		}
		else
		{
			// This is a root item. Remove the folder from the root items list.
			TreeRootItems.Remove(ItemToRemove);
		}

		// Refresh the tree
		TreeViewPtr->RequestTreeRefresh();

		return true;
	}
	else
	{
		// Did not find the folder to remove
		return false;
	}
}

void SExtPathView::RenameFolder(const FString& FolderToRename)
{
	TArray<TSharedPtr<FTreeItem>> Items = TreeViewPtr->GetSelectedItems();
	for (int32 ItemIdx = 0; ItemIdx < Items.Num(); ++ItemIdx)
	{
		TSharedPtr<FTreeItem>& Item = Items[ItemIdx];
		if (Item.IsValid())
		{
			if (Item->FolderPath == FolderToRename)
			{
				Item->bNamingFolder = true;
				TreeViewPtr->SetSelection(Item);
				TreeViewPtr->RequestScrollIntoView(Item);
				break;
			}
		}
	}
}

void SExtPathView::SyncToAssets( const TArray<FExtAssetData>& AssetDataList, const bool bAllowImplicitSync )
{
	SyncToInternal(AssetDataList, TArray<FString>(), bAllowImplicitSync);
}

void SExtPathView::SyncToFolders( const TArray<FString>& FolderList, const bool bAllowImplicitSync )
{
	SyncToInternal(TArray<FExtAssetData>(), FolderList, bAllowImplicitSync);
}

void SExtPathView::SyncTo( const FExtContentBrowserSelection& ItemSelection, const bool bAllowImplicitSync )
{
	SyncToInternal(ItemSelection.SelectedAssets, ItemSelection.SelectedFolders, bAllowImplicitSync);
}

void SExtPathView::SyncToInternal( const TArray<FExtAssetData>& AssetDataList, const TArray<FString>& FolderPaths, const bool bAllowImplicitSync )
{
	TArray<TSharedPtr<FTreeItem>> SyncTreeItems;

	// Clear the filter
	SearchBoxPtr->SetText(FText::GetEmpty());

	TSet<FString> PackagePaths = TSet<FString>(FolderPaths);
	for (const FExtAssetData& AssetData : AssetDataList)
	{
		FString PackagePath = AssetData.GetFolderPath();
		PackagePaths.Add(PackagePath);
	}

	for (const FString& PackagePath : PackagePaths)
	{
		if ( !PackagePath.IsEmpty() )
		{
			TSharedPtr<FTreeItem> Item = FindItemRecursive(PackagePath);
			if ( Item.IsValid() )
			{
				SyncTreeItems.Add(Item);
			}
		}
	}

	if ( SyncTreeItems.Num() > 0 )
	{
		if (bAllowImplicitSync)
		{
			// Prune the current selection so that we don't unnecessarily change the path which might disorientate the user.
			// If a parent tree item is currently selected we don't need to clear it and select the child
			auto SelectedTreeItems = TreeViewPtr->GetSelectedItems();

			for (int32 Index = 0; Index < SelectedTreeItems.Num(); ++Index)
			{
				// For each item already selected in the tree
				auto AlreadySelectedTreeItem = SelectedTreeItems[Index];
				if (!AlreadySelectedTreeItem.IsValid())
				{
					continue;
				}

				// Check to see if any of the items to sync are already synced
				for (int32 ToSyncIndex = SyncTreeItems.Num()-1; ToSyncIndex >= 0; --ToSyncIndex)
				{
					auto ToSyncItem = SyncTreeItems[ToSyncIndex];
					if (ToSyncItem == AlreadySelectedTreeItem || ToSyncItem->IsChildOf(*AlreadySelectedTreeItem.Get()))
					{
						// A parent is already selected
						SyncTreeItems.Pop();
					}
					else if (ToSyncIndex == 0)
					{
						// AlreadySelectedTreeItem is not required for SyncTreeItems, so deselect it
						TreeViewPtr->SetItemSelection(AlreadySelectedTreeItem, false);
					}
				}
			}
		}
		else
		{
			// Explicit sync so just clear the selection
			TreeViewPtr->ClearSelection();
		}

		// SyncTreeItems should now only contain items which aren't already shown explicitly or implicitly (as a child)
		for ( auto ItemIt = SyncTreeItems.CreateConstIterator(); ItemIt; ++ItemIt )
		{
			RecursiveExpandParents(*ItemIt);
			TreeViewPtr->SetItemSelection(*ItemIt, true);
		}

		// > 0 as some may have been popped off in the code above
		if (SyncTreeItems.Num() > 0)
		{
			// Scroll the first item into view if applicable
			TreeViewPtr->RequestScrollIntoView(SyncTreeItems[0]);
		}
	}
}

TSharedPtr<FTreeItem> SExtPathView::FindItemRecursive(const FString& Path) const
{
	for (auto TreeItemIt = TreeRootItems.CreateConstIterator(); TreeItemIt; ++TreeItemIt)
	{
		if ( (*TreeItemIt)->FolderPath == Path)
		{
			// This root item is the path
			return *TreeItemIt;
		}

		// Try to find the item under this root
		TSharedPtr<FTreeItem> Item = (*TreeItemIt)->FindItemRecursive(Path);
		if ( Item.IsValid() )
		{
			// The item was found under this root
			return Item;
		}
	}

	return TSharedPtr<FTreeItem>();
}

void SExtPathView::ApplyHistoryData ( const FHistoryData& History )
{
#if ECB_WIP_HISTORY
	// Prevent the selection changed delegate because it would add more history when we are just setting a state
	FScopedPreventTreeItemChangedDelegate DelegatePrevention( SharedThis(this) );

	// Update paths
	TArray<FString> SelectedPaths;
	for (const FName& HistoryPath : History.SourcesData.PackagePaths)
	{
		SelectedPaths.Add(HistoryPath.ToString());
	}
	SetSelectedPaths(SelectedPaths);
#endif
}

void SExtPathView::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	FString SelectedPathsString;
	TArray< TSharedPtr<FTreeItem> > PathItems = TreeViewPtr->GetSelectedItems();
	for ( auto PathIt = PathItems.CreateConstIterator(); PathIt; ++PathIt )
	{
		if ( SelectedPathsString.Len() > 0 )
		{
			SelectedPathsString += TEXT(",");
		}

		SelectedPathsString += (*PathIt)->FolderPath;
	}

	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".SelectedPaths")), *SelectedPathsString, IniFilename);
}

void SExtPathView::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	// Selected Paths
	FString SelectedPathsString;
	TArray<FString> NewSelectedPaths;
	if ( GConfig->GetString(*IniSection, *(SettingsString + TEXT(".SelectedPaths")), SelectedPathsString, IniFilename) )
	{
		SelectedPathsString.ParseIntoArray(NewSelectedPaths, TEXT(","), /*bCullEmpty*/true);
	}

	if ( NewSelectedPaths.Num() > 0 )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		const bool bDiscoveringAssets = AssetRegistryModule.Get().IsLoadingAssets();

		if ( bDiscoveringAssets )
		{
			// Keep track if we changed at least one source so we know to fire the bulk selection changed delegate later
			bool bSelectedAtLeastOnePath = false;

			{
				// Prevent the selection changed delegate since we are selecting one path at a time. A bulk event will be fired later if needed.
				FScopedPreventTreeItemChangedDelegate DelegatePrevention( SharedThis(this) );				

				// Clear any previously selected paths
				TreeViewPtr->ClearSelection();

				// If the selected paths is empty, the path was "All assets"
				// This should handle that case properly
				for (int32 PathIdx = 0; PathIdx < NewSelectedPaths.Num(); ++PathIdx)
				{
					const FString& Path = NewSelectedPaths[PathIdx];
					if ( ExplicitlyAddPathToSelection(Path) )
					{
						bSelectedAtLeastOnePath = true;
					}
					else
					{
						// If we could not initially select these paths, but are still discovering assets, add them to a pending list to select them later
						PendingInitialPaths.Add(Path);
					}
				}
			}

			if ( bSelectedAtLeastOnePath )
			{
				// Send the first selected item with the notification
				const TArray<TSharedPtr<FTreeItem>> SelectedItems = TreeViewPtr->GetSelectedItems();
				check(SelectedItems.Num() > 0);

				// Signal a single selection changed event to let any listeners know that paths have changed
				TreeSelectionChanged( SelectedItems[0], ESelectInfo::Direct );
			}
		}
		else
		{
			// If all assets are already discovered, just select paths the best we can
			SetSelectedPaths(NewSelectedPaths);

			// Send the first selected item with the notification
			const TArray<TSharedPtr<FTreeItem>> SelectedItems = TreeViewPtr->GetSelectedItems();
			if (SelectedItems.Num() > 0)
			{
				// Signal a single selection changed event to let any listeners know that paths have changed
				TreeSelectionChanged( SelectedItems[0], ESelectInfo::Direct );
			}
		}
	}
}

EActiveTimerReturnType SExtPathView::SetFocusPostConstruct( double InCurrentTime, float InDeltaTime )
{
	FWidgetPath WidgetToFocusPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked( SearchBoxPtr.ToSharedRef(), WidgetToFocusPath );
	FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EFocusCause::SetDirectly );

	return EActiveTimerReturnType::Stop;
}

EActiveTimerReturnType SExtPathView::TriggerRepopulate(double InCurrentTime, float InDeltaTime)
{
	Populate();
	return EActiveTimerReturnType::Stop;
}

TSharedPtr<SWidget> SExtPathView::MakePathViewContextMenu()
{
	if (!bAllowContextMenu || !OnGetFolderContextMenu.IsBound())
	{
		return nullptr;
	}

	const TArray<FString> SelectedPaths = GetSelectedPaths();
	if (SelectedPaths.Num() == 0)
	{
		return nullptr;
	}
	return OnGetFolderContextMenu.Execute(SelectedPaths, OnGetPathContextMenuExtender, NULL);
}

void SExtPathView::OnCreateNewFolder(const FString& FolderName, const FString& FolderPath)
{
	AddPath(FolderPath / FolderName);
}

bool SExtPathView::ExplicitlyAddPathToSelection(const FString& Path)
{
	if ( !ensure(TreeViewPtr.IsValid()) )
	{
		return false;
	}

	TArray<FString> PathItemList;
	Path.ParseIntoArray(PathItemList, TEXT("/"), /*InCullEmpty=*/true);

	if ( PathItemList.Num() )
	{
		// There is at least one element in the path
		TSharedPtr<FTreeItem> RootItem;

		// Find the first item in the root items list
		for ( int32 RootItemIdx = 0; RootItemIdx < TreeRootItems.Num(); ++RootItemIdx )
		{
			if ( TreeRootItems[RootItemIdx]->FolderPath == PathItemList[0] )
			{
				// Found the first item in the path
				RootItem = TreeRootItems[RootItemIdx];
				break;
			}
		}

		// If found in the root items list, try to find the item matching the path
		if ( RootItem.IsValid() )
		{
			TSharedPtr<FTreeItem> FoundItem = RootItem->FindItemRecursive(Path);

			if ( FoundItem.IsValid() )
			{
				// Set the selection to the closest found folder and scroll it into view
				RecursiveExpandParents(FoundItem);
				TreeViewPtr->SetItemSelection(FoundItem, true);
				TreeViewPtr->RequestScrollIntoView(FoundItem);

				return true;
			}
		}
	}

	return false;
}

bool SExtPathView::ShouldAllowTreeItemChangedDelegate() const
{
	return PreventTreeItemChangedDelegateCount == 0;
}

void SExtPathView::RecursiveExpandParents(const TSharedPtr<FTreeItem>& Item)
{
	if ( Item->Parent.IsValid() )
	{
		RecursiveExpandParents(Item->Parent.Pin());
		TreeViewPtr->SetItemExpansion(Item->Parent.Pin(), true);
	}
}

TSharedPtr<struct FTreeItem> SExtPathView::AddRootItem( const FString& InFolderName )
{
	// Make sure the item is not already in the list
	for ( int32 RootItemIdx = 0; RootItemIdx < TreeRootItems.Num(); ++RootItemIdx )
	{
		if ( TreeRootItems[RootItemIdx]->FolderName == InFolderName )
		{
			// The root to add was already in the list return it here
			return TreeRootItems[RootItemIdx];
		}
	}

	TSharedPtr<struct FTreeItem> NewItem = nullptr;

	const bool bIsValidRootFolder = true;
	if (bIsValidRootFolder)
	{
		const FText DisplayName = ExtContentBrowserUtils::GetRootDirDisplayName(InFolderName);
		NewItem = MakeShareable( new FTreeItem(DisplayName, /*FolderName*/ InFolderName, /*InFolderPath*/ InFolderName, /*InRootFolderPath*/InFolderName, TSharedPtr<FTreeItem>()));
		TreeRootItems.Add( NewItem );
		TreeViewPtr->RequestTreeRefresh();
	}

	return NewItem;
}

TSharedRef<ITableRow> SExtPathView::GenerateTreeRow( TSharedPtr<FTreeItem> TreeItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	check(TreeItem.IsValid());

	return
		SNew( STableRow< TSharedPtr<FTreeItem> >, OwnerTable )
		.OnDragDetected( this, &SExtPathView::OnFolderDragDetected )
		[
			SNew(SExtAssetTreeItem)
			.TreeItem(TreeItem)
			.OnAssetsOrPathsDragDropped(this, &SExtPathView::TreeAssetsOrPathsDropped)
			.OnFilesDragDropped(this, &SExtPathView::TreeFilesDropped)
			.IsItemExpanded(this, &SExtPathView::IsTreeItemExpanded, TreeItem)
			.HighlightText(this, &SExtPathView::GetHighlightText)
			.IsSelected(this, &SExtPathView::IsTreeItemSelected, TreeItem)
		];
}

void SExtPathView::TreeItemScrolledIntoView( TSharedPtr<FTreeItem> TreeItem, const TSharedPtr<ITableRow>& Widget )
{
	if ( TreeItem->bNamingFolder && Widget.IsValid() && Widget->GetContent().IsValid() )
	{
		TreeItem->OnRenamedRequestEvent.Broadcast();
	}
}

void SExtPathView::GetChildrenForTree( TSharedPtr< FTreeItem > TreeItem, TArray< TSharedPtr<FTreeItem> >& OutChildren )
{
	TreeItem->SortChildrenIfNeeded();
	OutChildren = TreeItem->Children;
}

void SExtPathView::SetTreeItemExpansionRecursive( TSharedPtr< FTreeItem > TreeItem, bool bInExpansionState )
{
	TreeViewPtr->SetItemExpansion(TreeItem, bInExpansionState);

	// Recursively go through the children.
	for(auto It = TreeItem->Children.CreateIterator(); It; ++It)
	{
		SetTreeItemExpansionRecursive( *It, bInExpansionState );
	}
}

void SExtPathView::TreeSelectionChanged( TSharedPtr< FTreeItem > TreeItem, ESelectInfo::Type /*SelectInfo*/ )
{
	if ( ShouldAllowTreeItemChangedDelegate() )
	{
		const TArray<TSharedPtr<FTreeItem>> SelectedItems = TreeViewPtr->GetSelectedItems();

		LastSelectedPaths.Empty();
		for (int32 ItemIdx = 0; ItemIdx < SelectedItems.Num(); ++ItemIdx)
		{
			const TSharedPtr<FTreeItem> Item = SelectedItems[ItemIdx];
			if ( !ensure(Item.IsValid()) )
			{
				// All items must exist
				continue;
			}

			// Keep track of the last paths that we broadcasted for selection reasons when filtering
			LastSelectedPaths.Add(Item->FolderPath);
		}

		if (OnPathSelected.IsBound() )
		{
			if ( TreeItem.IsValid() )
			{
				OnPathSelected.Execute(TreeItem->FolderPath);
			}
			else
			{
				OnPathSelected.Execute(TEXT(""));
			}
		}
	}

	if (TreeItem.IsValid())
	{
		// Prioritize the asset registry scan for the selected path
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().PrioritizeSearchPath(TreeItem->FolderPath / TEXT(""));
	}
}

void SExtPathView::TreeExpansionChanged( TSharedPtr< FTreeItem > TreeItem, bool bIsExpanded )
{
	if ( ShouldAllowTreeItemChangedDelegate() )
	{
		TSet<TSharedPtr<FTreeItem>> ExpandedItemSet;
		TreeViewPtr->GetExpandedItems(ExpandedItemSet);
		const TArray<TSharedPtr<FTreeItem>> ExpandedItems = ExpandedItemSet.Array();

		LastExpandedPaths.Empty();
		for (int32 ItemIdx = 0; ItemIdx < ExpandedItems.Num(); ++ItemIdx)
		{
			const TSharedPtr<FTreeItem> Item = ExpandedItems[ItemIdx];
			if ( !ensure(Item.IsValid()) )
			{
				// All items must exist
				continue;
			}

			// Keep track of the last paths that we broadcasted for expansion reasons when filtering
			LastExpandedPaths.Add(Item->FolderPath);
		}
	}
}

void SExtPathView::OnAssetTreeSearchBoxChanged( const FText& InSearchText )
{
	SearchBoxFolderFilter->SetRawFilterText( InSearchText );
	SearchBoxPtr->SetError( SearchBoxFolderFilter->GetFilterErrorText() );
}

void SExtPathView::OnAssetTreeSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	if (InCommitType == ETextCommit::OnCleared)
	{
		// Clear the search box and the filters
		SearchBoxPtr->SetText(FText::GetEmpty());
		OnAssetTreeSearchBoxChanged(FText::GetEmpty());
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
	}
}

void SExtPathView::FilterUpdated()
{
	Populate();
}

FText SExtPathView::GetHighlightText() const
{
	return SearchBoxFolderFilter->GetRawFilterText();
}

void SExtPathView::Populate()
{
	ECB_LOG(Display, TEXT("[SExtPathView] Populate"));

	// Don't allow the selection changed delegate to be fired here
	FScopedPreventTreeItemChangedDelegate DelegatePrevention( SharedThis(this) );

	// Clear all root items and clear selection
	TreeRootItems.Empty();
	TreeViewPtr->ClearSelection();

	const bool bFilteringByText = !SearchBoxFolderFilter->GetRawFilterText().IsEmpty();

	TArray<FString> RootContentPaths;
	FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPaths(RootContentPaths);

	// Add default root folders to the asset tree if there's no filtering
	if ( !bFilteringByText ) 
	{	
		for( TArray<FString>::TConstIterator RootPathIt( RootContentPaths ); RootPathIt; ++RootPathIt )
		{
			// Strip off any trailing forward slashes
			FString CleanRootPathName = *RootPathIt;
			while( CleanRootPathName.EndsWith( TEXT( "/" ) ) )
			{
				CleanRootPathName = CleanRootPathName.Mid( 0, CleanRootPathName.Len() - 1 );
			}

			AddPath(CleanRootPathName, &CleanRootPathName);
		}
	}
	
	for (const FString& RootContentPath : RootContentPaths)
	{
		if (FExtContentBrowserSingleton::GetAssetRegistry().IsFolderBackgroundGathering(*RootContentPath))
		{
			//continue;
		}

		// Get all paths
		TSet<FName> SubPaths;
		FExtContentBrowserSingleton::GetAssetRegistry().GetCachedSubPaths/*GetOrCacheSubPaths*//*safer*/(*RootContentPath, SubPaths, /*bRecursively = */ true);

		// Add all paths
		for (const FName& SubPath : SubPaths)
		{
			const FString Path = SubPath.ToString();

			// by sending the whole path we deliberately include any children of successful hits in the filtered list. 
			if (SearchBoxFolderFilter->PassesFilter(Path))
			{
				TSharedPtr<FTreeItem> Item = AddPath(Path, &RootContentPath);
				if (Item.IsValid())
				{
					const bool bSelectedItem = LastSelectedPaths.Contains(Item->FolderPath);
					const bool bExpandedItem = LastExpandedPaths.Contains(Item->FolderPath);

					if (bFilteringByText || bSelectedItem)
					{
						RecursiveExpandParents(Item);
					}

					if (bSelectedItem)
					{
						// Tree items that match the last broadcasted paths should be re-selected them after they are added
						if (!TreeViewPtr->IsItemSelected(Item))
						{
							TreeViewPtr->SetItemSelection(Item, true);
						}
						TreeViewPtr->RequestScrollIntoView(Item);
					}

					if (bExpandedItem)
					{
						// Tree items that were previously expanded should be re-expanded when repopulating
						if (!TreeViewPtr->IsItemExpanded(Item))
						{
							TreeViewPtr->SetItemExpansion(Item, true);
							//RecursiveExpandParents(Item);
						}
					}
				}
			}
		}
	}

	SortRootItems();
}

void SExtPathView::SortRootItems()
{
	// First sort the root items by their display name, but also making sure that content to appears before classes
	TreeRootItems.Sort([](const TSharedPtr<FTreeItem>& One, const TSharedPtr<FTreeItem>& Two) -> bool
	{
		static const FString ClassesPrefix = TEXT("Classes_");

		FString OneModuleName = One->FolderName;
		const bool bOneIsClass = OneModuleName.StartsWith(ClassesPrefix);
		if(bOneIsClass)
		{
			OneModuleName = OneModuleName.Mid(ClassesPrefix.Len());
		}

		FString TwoModuleName = Two->FolderName;
		const bool bTwoIsClass = TwoModuleName.StartsWith(ClassesPrefix);
		if(bTwoIsClass)
		{
			TwoModuleName = TwoModuleName.Mid(ClassesPrefix.Len());
		}

		// We want to sort content before classes if both items belong to the same module
		if(OneModuleName == TwoModuleName)
		{
			if(!bOneIsClass && bTwoIsClass)
			{
				return true;
			}
			return false;
		}

		return One->DisplayName.ToString() < Two->DisplayName.ToString();
	});

	// We have some manual sorting requirements that game must come before engine, and engine before everything else - we do that here after sorting everything by name
	// The array below is in the inverse order as we iterate through and move each match to the beginning of the root items array
	static const FString InverseSortOrder[] = {
		TEXT("Classes_Engine"),
		TEXT("Engine"),
		TEXT("Classes_Game"),
		TEXT("Game"),
	};
	for(const FString& SortItem : InverseSortOrder)
	{
		const int32 FoundItemIndex = TreeRootItems.IndexOfByPredicate([&SortItem](const TSharedPtr<FTreeItem>& TreeItem) -> bool
		{
			return TreeItem->FolderName == SortItem;
		});
		if(FoundItemIndex != INDEX_NONE)
		{
			TSharedPtr<FTreeItem> ItemToMove = TreeRootItems[FoundItemIndex];
			TreeRootItems.RemoveAt(FoundItemIndex);
			TreeRootItems.Insert(ItemToMove, 0);
		}
	}

	TreeViewPtr->RequestTreeRefresh();
}

void SExtPathView::PopulateFolderSearchStrings( const FString& FolderName, OUT TArray< FString >& OutSearchStrings ) const
{
	OutSearchStrings.Add( FolderName );
}

FReply SExtPathView::OnFolderDragDetected(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton ) )
	{
		TArray<TSharedPtr<FTreeItem>> SelectedItems = TreeViewPtr->GetSelectedItems();
		if (SelectedItems.Num())
		{
			TArray<FString> PathNames;

			for ( auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt )
			{
				PathNames.Add((*ItemIt)->FolderPath);
			}

			return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(PathNames));
		}
	}

	return FReply::Unhandled();
}


bool SExtPathView::FolderAlreadyExists(const TSharedPtr< FTreeItem >& TreeItem, TSharedPtr< FTreeItem >& ExistingItem)
{
	ExistingItem.Reset();

	if ( TreeItem.IsValid() )
	{
		if ( TreeItem->Parent.IsValid() )
		{
			// This item has a parent, try to find it in its parent's children
			TSharedPtr<FTreeItem> ParentItem = TreeItem->Parent.Pin();

			for ( auto ChildIt = ParentItem->Children.CreateConstIterator(); ChildIt; ++ChildIt )
			{
				const TSharedPtr<FTreeItem>& Child = *ChildIt;
				if ( Child != TreeItem && Child->FolderName == TreeItem->FolderName )
				{
					// The item is in its parent already
					ExistingItem = Child;
					break;
				}
			}
		}
		else
		{
			// This item is part of the root set
			for ( auto RootIt = TreeRootItems.CreateConstIterator(); RootIt; ++RootIt )
			{
				const TSharedPtr<FTreeItem>& Root = *RootIt;
				if ( Root != TreeItem && Root->FolderName == TreeItem->FolderName )
				{
					// The item is part of the root set already
					ExistingItem = Root;
					break;
				}
			}
		}
	}

	return ExistingItem.IsValid();
}

void SExtPathView::RemoveFolderItem(const TSharedPtr< FTreeItem >& TreeItem)
{
	if ( TreeItem.IsValid() )
	{
		if ( TreeItem->Parent.IsValid() )
		{
			// Remove this item from it's parent's list
			TreeItem->Parent.Pin()->Children.Remove(TreeItem);
		}
		else
		{
			// This was a root node, remove from the root list
			TreeRootItems.Remove(TreeItem);
		}

		TreeViewPtr->RequestTreeRefresh();
	}
}

void SExtPathView::TreeAssetsOrPathsDropped(const TArray<FAssetData>& AssetList, const TArray<FString>& AssetPaths, const TSharedPtr<FTreeItem>& TreeItem)
{
	DragDropHandler::HandleDropOnAssetFolder(
		SharedThis(this), 
		AssetList, 
		AssetPaths, 
		TreeItem->FolderPath, 
		TreeItem->DisplayName, 
		DragDropHandler::FExecuteCopyOrMove::CreateSP(this, &SExtPathView::ExecuteTreeDropCopy),
		DragDropHandler::FExecuteCopyOrMove::CreateSP(this, &SExtPathView::ExecuteTreeDropMove),
		DragDropHandler::FExecuteCopyOrMove::CreateSP(this, &SExtPathView::ExecuteTreeDropAdvancedCopy)
		);
}

void SExtPathView::TreeFilesDropped(const TArray<FString>& FileNames, const TSharedPtr<FTreeItem>& TreeItem)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().ImportAssets( FileNames, TreeItem->FolderPath );
}

bool SExtPathView::IsTreeItemExpanded(TSharedPtr<FTreeItem> TreeItem) const
{
	return TreeViewPtr->IsItemExpanded(TreeItem);
}

bool SExtPathView::IsTreeItemSelected(TSharedPtr<FTreeItem> TreeItem) const
{
	return TreeViewPtr->IsItemSelected(TreeItem);
}

void SExtPathView::ExecuteTreeDropCopy(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
	if (AssetList.Num() > 0)
	{
		TArray<UObject*> DroppedObjects;
		ExtContentBrowserUtils::GetObjectsInAssetData(AssetList, DroppedObjects);

		ExtContentBrowserUtils::CopyAssets(DroppedObjects, DestinationPath);
	}

	if (AssetPaths.Num() > 0 && ExtContentBrowserUtils::CopyFolders(AssetPaths, DestinationPath))
	{
		TSharedPtr<FTreeItem> RootItem = FindItemRecursive(DestinationPath);
		if (RootItem.IsValid())
		{
			TreeViewPtr->SetItemExpansion(RootItem, true);

			// Select all the new folders
			TreeViewPtr->ClearSelection();
			for (const FString& AssetPath : AssetPaths)
			{
				const FString SubFolderName = FPackageName::GetLongPackageAssetName(AssetPath);
				const FString NewPath = DestinationPath + TEXT("/") + SubFolderName;

				TSharedPtr<FTreeItem> Item = FindItemRecursive(NewPath);
				if (Item.IsValid())
				{
					TreeViewPtr->SetItemSelection(Item, true);
					TreeViewPtr->RequestScrollIntoView(Item);
				}
			}
		}
	}
}

void SExtPathView::ExecuteTreeDropMove(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
#if ECB_LEGACY
	if (AssetList.Num() > 0)
	{
		TArray<UObject*> DroppedObjects;
		ContentBrowserUtils::GetObjectsInAssetData(AssetList, DroppedObjects);

		ContentBrowserUtils::MoveAssets(DroppedObjects, DestinationPath);
	}

	// Prepare to fixup any asset paths that are favorites
	TArray<FMovedContentFolder> MovedFolders;
	for (const FString& OldPath : AssetPaths)
	{
		const FString SubFolderName = FPackageName::GetLongPackageAssetName(OldPath);
		const FString NewPath = DestinationPath + TEXT("/") + SubFolderName;
		MovedFolders.Add(FMovedContentFolder(OldPath, NewPath));
	}

	if (AssetPaths.Num() > 0 && ContentBrowserUtils::MoveFolders(AssetPaths, DestinationPath))
	{
		TSharedPtr<FTreeItem> RootItem = FindItemRecursive(DestinationPath);
		if (RootItem.IsValid())
		{
			TreeViewPtr->SetItemExpansion(RootItem, true);

			// Select all the new folders
			TreeViewPtr->ClearSelection();
			for (const FString& AssetPath : AssetPaths)
			{
				const FString SubFolderName = FPackageName::GetLongPackageAssetName(AssetPath);
				const FString NewPath = DestinationPath + TEXT("/") + SubFolderName;
				TSharedPtr<FTreeItem> Item = FindItemRecursive(NewPath);
				if (Item.IsValid())
				{
					TreeViewPtr->SetItemSelection(Item, true);
					TreeViewPtr->RequestScrollIntoView(Item);
				}
			}
		}

		OnFolderPathChanged.ExecuteIfBound(MovedFolders);
	}
#endif
}

void SExtPathView::GetPathItemList(const FString& InPath, const TArray<FString>& InRootPaths, TArray<FString>& OutPathItemList, bool bIncludeRootPath) const
{
	bool bFoundRootPath = false;
	FString RootPath;
	FString RelativePath;

	for (int32 Index = 0; Index < InRootPaths.Num(); ++Index)
	{
		RootPath = InRootPaths[Index];
		if (InPath.Find(*RootPath) == 0)
		{
			bFoundRootPath = true;
			RelativePath = InPath.Mid(FCString::Strlen(*RootPath));
			break;
		}
	}

	RelativePath.ParseIntoArray(OutPathItemList, TEXT("/"), /*InCullEmpty=*/true);

	if (bFoundRootPath && !RootPath.IsEmpty() && bIncludeRootPath)
	{
		OutPathItemList.Insert(RootPath, 0);
	}
}

void SExtPathView::ExecuteTreeDropAdvancedCopy(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
	ExtContentBrowserUtils::BeginAdvancedCopyPackages(AssetList, AssetPaths, DestinationPath);
}

void SExtPathView::OnAssetRegistryPathAdded(const FString& Path)
{
	// by sending the whole path we deliberately include any children
	// of successful hits in the filtered list. 
	if ( SearchBoxFolderFilter->PassesFilter( Path ) )
	{
		AddPath(Path);
	}
}

void SExtPathView::OnAssetRegistryPathRemoved(const FString& Path)
{
	// by sending the whole path we deliberately include any children
	// of successful hits in the filtered list. 
	if ( SearchBoxFolderFilter->PassesFilter( Path ) )
	{
		RemovePath(Path);
	}
}

void SExtPathView::OnAssetRegistryRootPathAdded(const FString& Path)
{
	Populate();
}

void SExtPathView::OnAssetRegistryRootPathRemoved(const FString& Path)
{
	Populate();
	ClearSelection();
}

void SExtPathView::OnAssetRegistryRootPathUpdated()
{
	Populate();
}

void SExtPathView::OnAssetRegistryFolderStartGathering(const TArray<FString>& Paths)
{
	//Populate();
	//ClearSelection();
}

void SExtPathView::OnAssetRegistryFolderFinishGathering(const FString& InPath, const FString& InRootPath)
{
	//Populate();
	if (SearchBoxFolderFilter->PassesFilter(InPath))
	{
		AddPath(InPath, &InRootPath);

		TArray<FString> SelectedPath = GetSelectedPaths();
		if (SelectedPath.Contains(InPath) && OnPathSelected.IsBound())
		{
			// Refresh
			OnPathSelected.Execute(InPath);
		}
	}
}

void SExtPathView::OnAssetRegistrySearchCompleted()
{
	// If there were any more initial paths, they no longer exist so clear them now.
	PendingInitialPaths.Empty();
}

void SExtPathView::OnFolderPopulated(const FString& Path)
{
	OnAssetRegistryPathAdded(Path);
}

void SExtPathView::OnContentPathMountedOrDismounted( const FString& AssetPath, const FString& FilesystemPath )
{
	/**
	 * Hotfix
	 * For some reason this widget sometime outlive the slate application shutdown
	 * Validating that Slate application base is initialized will at least avoid the possible crash
	 */
	if ( FSlateApplicationBase::IsInitialized() )
	{
		// A new content path has appeared, so we should refresh out root set of paths
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SExtPathView::TriggerRepopulate));
	}
}

void SExtPathView::OnClassHierarchyUpdated()
{
	// The class hierarchy has changed in some way, so we need to refresh our set of paths
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SExtPathView::TriggerRepopulate));
}

void SExtPathView::HandleSettingChanged(FName PropertyName)
{
#if ECB_TODO
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UExtContentBrowserSettings, DisplayEmptyFolders)) ||
		(PropertyName == "DisplayDevelopersFolder") ||
		(PropertyName == "DisplayEngineFolder") ||
		(PropertyName == "DisplayPluginFolders") ||
		(PropertyName == "DisplayL10NFolder") ||
		(PropertyName == NAME_None))	// @todo: Needed if PostEditChange was called manually, for now
	{
		TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();

		// If the dev or engine folder is no longer visible but we're inside it...
		const bool bDisplayEmpty = GetDefault<UExtContentBrowserSettings>()->DisplayEmptyFolders;
		const bool bDisplayDev = GetDefault<UExtContentBrowserSettings>()->GetDisplayDevelopersFolder();
		const bool bDisplayEngine = GetDefault<UExtContentBrowserSettings>()->GetDisplayEngineFolder();
		const bool bDisplayPlugins = GetDefault<UExtContentBrowserSettings>()->GetDisplayPluginFolders();
		const bool bDisplayL10N = GetDefault<UExtContentBrowserSettings>()->GetDisplayL10NFolder();
		if (!bDisplayEmpty || !bDisplayDev || !bDisplayEngine || !bDisplayPlugins || !bDisplayL10N)
		{
			const FString OldSelectedPath = GetSelectedPath();
			const ContentBrowserUtils::ECBFolderCategory OldFolderCategory = ContentBrowserUtils::GetFolderCategory(OldSelectedPath);

			if ((!bDisplayEmpty && !EmptyFolderVisibilityManager->ShouldShowPath(OldSelectedPath)) || 
				(!bDisplayDev && OldFolderCategory == ContentBrowserUtils::ECBFolderCategory::DeveloperContent) || 
				(!bDisplayEngine && (OldFolderCategory == ContentBrowserUtils::ECBFolderCategory::EngineContent || OldFolderCategory == ContentBrowserUtils::ECBFolderCategory::EngineClasses)) || 
				(!bDisplayPlugins && (OldFolderCategory == ContentBrowserUtils::ECBFolderCategory::PluginContent || OldFolderCategory == ContentBrowserUtils::ECBFolderCategory::PluginClasses)) ||
				(!bDisplayL10N && ContentBrowserUtils::IsLocalizationFolder(OldSelectedPath)))
			{
				// Set the folder back to the root, and refresh the contents
				TSharedPtr<FTreeItem> GameRoot = FindItemRecursive(TEXT("/Game"));
				if ( GameRoot.IsValid() )
				{
					TreeViewPtr->SetSelection(GameRoot);
				}
				else
				{
					TreeViewPtr->ClearSelection();
				}
			}
		}

		// Update our path view so that it can include/exclude the dev folder
		Populate();

		// If the dev or engine folder has become visible and we're inside it...
		if (bDisplayDev || bDisplayEngine || bDisplayPlugins || bDisplayL10N)
		{
			const FString NewSelectedPath = GetSelectedPath();
			const ContentBrowserUtils::ECBFolderCategory NewFolderCategory = ContentBrowserUtils::GetFolderCategory(NewSelectedPath);

			if ((bDisplayDev && NewFolderCategory == ContentBrowserUtils::ECBFolderCategory::DeveloperContent) || 
				(bDisplayEngine && (NewFolderCategory == ContentBrowserUtils::ECBFolderCategory::EngineContent || NewFolderCategory == ContentBrowserUtils::ECBFolderCategory::EngineClasses)) || 
				(bDisplayPlugins && (NewFolderCategory == ContentBrowserUtils::ECBFolderCategory::PluginContent || NewFolderCategory == ContentBrowserUtils::ECBFolderCategory::PluginClasses)) ||
				(bDisplayL10N && ContentBrowserUtils::IsLocalizationFolder(NewSelectedPath)))
			{
				// Refresh the contents
				OnPathSelected.ExecuteIfBound(NewSelectedPath);
			}
		}
	}
#endif
}




void SFavoritePathView::Construct(const FArguments& InArgs)
{
	SAssignNew(TreeViewPtr, STreeView< TSharedPtr<FTreeItem> >)
		.TreeItemsSource(&TreeRootItems)
		.OnGetChildren(this, &SFavoritePathView::GetChildrenForTree)
		.OnGenerateRow(this, &SFavoritePathView::GenerateTreeRow)
		.OnItemScrolledIntoView(this, &SFavoritePathView::TreeItemScrolledIntoView)
		.ItemHeight(18)
		.SelectionMode(InArgs._SelectionMode)
		.OnSelectionChanged(this, &SFavoritePathView::TreeSelectionChanged)
		.OnContextMenuOpening(this, &SFavoritePathView::MakePathViewContextMenu)
		.ClearSelectionOnClick(false);

	SExtPathView::Construct(InArgs);
}

void SFavoritePathView::Populate()
{
	// Don't allow the selection changed delegate to be fired here
	FScopedPreventTreeItemChangedDelegate DelegatePrevention(SharedThis(this));

	// Clear all root items and clear selection
	TreeRootItems.Empty();
	TreeViewPtr->ClearSelection();
	const TArray<FString> FavoritePaths = ExtContentBrowserUtils::GetFavoriteFolders();
	// we have a text filter, expand all parents of matching folders
	for (const FString& Path : FavoritePaths)
	{
		// by sending the whole path we deliberately include any children
		// of successful hits in the filtered list. 
		if (SearchBoxFolderFilter->PassesFilter(Path))
		{
			TSharedPtr<FTreeItem> Item = AddPath(Path);
			if (Item.IsValid())
			{
				const bool bSelectedItem = LastSelectedPaths.Contains(Item->FolderPath);

				if (bSelectedItem)
				{
					// Tree items that match the last broadcasted paths should be re-selected them after they are added
					TreeViewPtr->SetItemSelection(Item, true);
					TreeViewPtr->RequestScrollIntoView(Item);
				}
			}
		}
	}

	SortRootItems();
}



void SFavoritePathView::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	SExtPathView::SaveSettings(IniFilename, IniSection, SettingsString);

	FString FavoritePathsString;
	const TArray<FString> FavoritePaths = ExtContentBrowserUtils::GetFavoriteFolders();
	for (const FString& PathIt : FavoritePaths)
	{
		if (FavoritePathsString.Len() > 0)
		{
			FavoritePathsString += TEXT(",");
		}

		FavoritePathsString += PathIt;
	}

	GConfig->SetString(*IniSection, TEXT("FavoritePaths"), *FavoritePathsString, IniFilename);
}

void SFavoritePathView::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	SExtPathView::LoadSettings(IniFilename, IniSection, SettingsString);

	// Selected Paths
	FString SelectedPathsString;
	TArray<FString> NewFavoritePaths;
	if (GConfig->GetString(*IniSection, TEXT("FavoritePaths"), SelectedPathsString, IniFilename))
	{
		SelectedPathsString.ParseIntoArray(NewFavoritePaths, TEXT(","), /*bCullEmpty*/true);
	}

	if (NewFavoritePaths.Num() > 0)
	{
		// Keep track if we changed at least one source so we know to fire the bulk selection changed delegate later
		bool bAddedAtLeastOnePath = false;
		{
			// If the selected paths is empty, the path was "All assets"
			// This should handle that case properly
			for (int32 PathIdx = 0; PathIdx < NewFavoritePaths.Num(); ++PathIdx)
			{
				const FString& Path = NewFavoritePaths[PathIdx];
				ExtContentBrowserUtils::AddFavoriteFolder(Path, false);
				bAddedAtLeastOnePath = true;
			}
		}

		if (bAddedAtLeastOnePath)
		{
			Populate();
		}
	}
}

void SFavoritePathView::OnAssetTreeSearchBoxChanged(const FText& InSearchText)
{
	SExtPathView::OnAssetTreeSearchBoxChanged(InSearchText);
	OnFavoriteSearchChanged.ExecuteIfBound(InSearchText);
}

void SFavoritePathView::OnAssetTreeSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type InCommitType)
{
	SExtPathView::OnAssetTreeSearchBoxCommitted(InSearchText, InCommitType);
	OnFavoriteSearchCommitted.ExecuteIfBound(InSearchText, InCommitType);
}

void SFavoritePathView::SetSelectedPaths(const TArray<FString>& Paths)
{
	if (!ensure(TreeViewPtr.IsValid()))
	{
		return;
	}

	if (!SearchBoxPtr->GetText().IsEmpty())
	{
		// Clear the search box so the selected paths will be visible
		SearchBoxPtr->SetText(FText::GetEmpty());
	}

	// Prevent the selection changed delegate since the invoking code requested it
	FScopedPreventTreeItemChangedDelegate DelegatePrevention(SharedThis(this));

	// If the selection was changed before all pending initial paths were found, stop attempting to select them
	PendingInitialPaths.Empty();

	// Clear the selection to start, then add the selected paths as they are found
	TreeViewPtr->ClearSelection();

	for (int32 PathIdx = 0; PathIdx < Paths.Num(); ++PathIdx)
	{
		const FString& Path = Paths[PathIdx];

		TArray<FString> PathItemList;
		Path.ParseIntoArray(PathItemList, TEXT("/"), /*InCullEmpty=*/true);

		if (PathItemList.Num())
		{
			// There is at least one element in the path
			TArray<TSharedPtr<FTreeItem>> TreeItems;

			// Find the first item in the root items list
			for (int32 RootItemIdx = 0; RootItemIdx < TreeRootItems.Num(); ++RootItemIdx)
			{
				if (TreeRootItems[RootItemIdx]->FolderName == PathItemList[0])
				{
					// Found the first item in the path
					TreeItems.Add(TreeRootItems[RootItemIdx]);
					break;
				}
			}

			// If found in the root items list, try to find the childmost item matching the path
			if (TreeItems.Num() > 0)
			{
				for (int32 PathItemIdx = 1; PathItemIdx < PathItemList.Num(); ++PathItemIdx)
				{
					const FString& PathItemName = PathItemList[PathItemIdx];
					const TSharedPtr<FTreeItem> ChildItem = TreeItems.Last()->GetChild(PathItemName);

					if (ChildItem.IsValid())
					{
						// Update tree items list
						TreeItems.Add(ChildItem);
					}
					else
					{
						// Could not find the child item
						break;
					}
				}

				// Set the selection to the closest found folder and scroll it into view
				TreeViewPtr->SetItemSelection(TreeItems.Last(), true);
				TreeViewPtr->RequestScrollIntoView(TreeItems.Last());
			}
			else
			{
				// Could not even find the root path... skip
			}
		}
		else
		{
			// No path items... skip
		}
	}
}

TSharedPtr<FTreeItem> SFavoritePathView::AddPath(const FString& InPath, const FString* RootPathPtr/* = nullptr*/, bool bUserNamed /*= false*/)
{
	if (!ensure(TreeViewPtr.IsValid()))
	{
		// No tree view for some reason
		return TSharedPtr<FTreeItem>();
	}

	FString Path = InPath;

	FString RootPath;
	if (RootPathPtr != nullptr)
	{
		RootPath = *RootPathPtr;
		if (Path.Find(*RootPath) == 0)
		{
			Path = Path.Mid(FCString::Strlen(*RootPath));
		}
	}

	TArray<FString> PathItemList;
	Path.ParseIntoArray(PathItemList, TEXT("/"), /*InCullEmpty=*/true);

	if (PathItemList.Num())
	{
		// There is at least one element in the path
		const FString FolderName = PathItemList.Last();
		const FString FolderPath = Path;

		// Make sure the item is not already in the list
		for (int32 RootItemIdx = 0; RootItemIdx < TreeRootItems.Num(); ++RootItemIdx)
		{
			if (TreeRootItems[RootItemIdx]->FolderPath == FolderPath)
			{
				// The root to add was already in the list return it here
				return TreeRootItems[RootItemIdx];
			}
		}

		TSharedPtr<struct FTreeItem> NewItem = nullptr;

		// If this isn't an engine folder or we want to show them, add
		const bool bDisplayEngine = GetDefault<UExtContentBrowserSettings>()->GetDisplayEngineFolder();
		const bool bDisplayPlugins = GetDefault<UExtContentBrowserSettings>()->GetDisplayPluginFolders();
		const bool bDisplayCpp = GetDefault<UExtContentBrowserSettings>()->GetDisplayCppFolders();
		const bool bDisplayLoc = GetDefault<UExtContentBrowserSettings>()->GetDisplayL10NFolder();
		// Filter out classes folders if we're not showing them.
		if (!bDisplayCpp && ExtContentBrowserUtils::IsClassesFolder(FolderName))
		{
			return nullptr;
		}

		// Filter out plugin folders
		bool bIsPlugin = false;
		EPluginLoadedFrom PluginSource = EPluginLoadedFrom::Engine; // init to avoid warning
		if (!bDisplayEngine || !bDisplayPlugins)
		{
			bIsPlugin = ExtContentBrowserUtils::IsPluginFolder(FolderName, &PluginSource);
		}

		if ((bDisplayEngine || !ExtContentBrowserUtils::IsEngineFolder(FolderName)) &&
			((bDisplayEngine && bDisplayPlugins) || !(bIsPlugin && PluginSource == EPluginLoadedFrom::Engine)) &&
			(bDisplayPlugins || !(bIsPlugin && PluginSource == EPluginLoadedFrom::Project)) &&
			(bDisplayLoc || !ExtContentBrowserUtils::IsLocalizationFolder(FolderName)))
		{
			const FText DisplayName = ExtContentBrowserUtils::GetRootDirDisplayName(FolderName);
			NewItem = MakeShareable(new FTreeItem(FText::FromString(FolderName), FolderName, FolderPath, RootPath, TSharedPtr<FTreeItem>()));
			TreeRootItems.Add(NewItem);
			TreeViewPtr->RequestTreeRefresh();
			TreeViewPtr->SetSelection(NewItem);
		}

		return NewItem;
	}
	return TSharedPtr<FTreeItem>();
}

TSharedRef<ITableRow> SFavoritePathView::GenerateTreeRow(TSharedPtr<FTreeItem> TreeItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(TreeItem.IsValid());

	return
		SNew( STableRow< TSharedPtr<FTreeItem> >, OwnerTable )
		.OnDragDetected( this, &SFavoritePathView::OnFolderDragDetected )
		[
			SNew(SExtAssetTreeItem)
			.TreeItem(TreeItem)
			.OnAssetsOrPathsDragDropped(this, &SFavoritePathView::TreeAssetsOrPathsDropped)
			.OnFilesDragDropped(this, &SFavoritePathView::TreeFilesDropped)
			.IsItemExpanded(false)
			.HighlightText(this, &SFavoritePathView::GetHighlightText)
			.IsSelected(this, &SFavoritePathView::IsTreeItemSelected, TreeItem)
			.FontOverride(FAppStyle::GetFontStyle("ContentBrowser.SourceTreeItemFont"))
		];
}


void SFavoritePathView::OnAssetRegistryPathAdded(const FString& Path)
{
}

void SFavoritePathView::OnAssetRegistryPathRemoved(const FString& Path)
{
	ExtContentBrowserUtils::RemoveFavoriteFolder(Path);
	Populate();

}

void SFavoritePathView::ExecuteTreeDropMove(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
#if ECB_LEGACY
	if (AssetList.Num() > 0)
	{
		TArray<UObject*> DroppedObjects;
		ContentBrowserUtils::GetObjectsInAssetData(AssetList, DroppedObjects);
		ContentBrowserUtils::MoveAssets(DroppedObjects, DestinationPath);
	}

	// Prepare to fixup any asset paths that are favorites
	TArray<FMovedContentFolder> MovedFolders;
	for (const FString& OldPath : AssetPaths)
	{
		const FString SubFolderName = FPackageName::GetLongPackageAssetName(OldPath);
		const FString NewPath = DestinationPath + TEXT("/") + SubFolderName;
		MovedFolders.Add(FMovedContentFolder(OldPath, NewPath));
	}

	FixupFavoritesFromExternalChange(MovedFolders);
	ContentBrowserUtils::MoveFolders(AssetPaths, DestinationPath);
#endif
}


FText SExtPathView::GetTreeTitle() const
{
	const UExtContentBrowserSettings* ExtContentBrowserSettings = GetDefault<UExtContentBrowserSettings>();
	const bool bCacheMode = ExtContentBrowserSettings->bCacheMode;
	if (bCacheMode)
	{
		const FString& CacheFilePath = ExtContentBrowserSettings->CacheFilePath.FilePath;
		const FString FileName = FPaths::GetBaseFilename(CacheFilePath);
		return FText::Format(LOCTEXT("CacheModeTreeTitle", "{0}"), FText::FromString(FileName));
	}
	else
	{
		return TreeTitle;
	}
}

EVisibility SExtPathView::GetTreeTitleVisibility() const
{
	
	const UExtContentBrowserSettings* ExtContentBrowserSettings = GetDefault<UExtContentBrowserSettings>();
	const bool bCacheMode = ExtContentBrowserSettings->bCacheMode;
	if (bCacheMode)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

#undef LOCTEXT_NAMESPACE

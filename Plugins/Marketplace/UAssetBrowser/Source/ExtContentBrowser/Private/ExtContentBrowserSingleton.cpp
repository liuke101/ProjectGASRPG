// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtContentBrowserSingleton.h"
#include "ExtContentBrowserStyle.h"
#include "ExtContentBrowserCommands.h"
#include "SExtDependencyNode.h"

#include "Textures/SlateIcon.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Widgets/SWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/WorkspaceItem.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "Editor.h"
#include "SExtContentBrowser.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "IDocumentation.h"
#include "Interfaces/IMainFrameModule.h"
#include "LevelEditor.h"
#include "Interfaces/IPluginManager.h"

#if ECB_LEGACY
#include "TutorialMetaData.h"
#include "SAssetDialog.h"
#include "SAssetPicker.h"
#endif

#if ECB_WIP_PATHPICKER
#include "SPathPicker.h"
#include "SCollectionPicker.h"
#endif

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

/////////////////////////////////////////////////////////////
// FExtContentBrowserSingleton implementation
//

FExtAssetRegistry FExtContentBrowserSingleton::ExtAssetRegistry;

FExtContentBrowserSingleton::FExtContentBrowserSingleton()
// 	, SettingsStringID(0)
{
	UExtContentBrowserSettings* ExtContentBrowserSetting = GetMutableDefault<UExtContentBrowserSettings>();

#if ECB_WIP_CACHEDB
	ExtAssetRegistry.SwitchCacheMode();
#else
	ExtAssetRegistry.LoadRootContentPaths();
#endif

#if ECB_WIP_OBJECT_THUMB_POOL
	if (ExtContentBrowserSetting->bUseThumbnailPool && ExtContentBrowserSetting->NumThumbnailsInPool > 0)
	{
		ThumbnailPool.Reserve(ExtContentBrowserSetting->NumThumbnailsInPool); // todo: move to setting
	}
#endif

	// Style
	FExtContentBrowserStyle::Initialize();
	FExtContentBrowserStyle::ReloadTextures();

	RegisterMenuAndTabSpawner();
	
	RegisterCommands();

	{
		ToolbarExtender = MakeShareable(new FExtender);
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		ExtContentBrowserSetting->DisplayToolbarButton = false;
		ToggleShowToolbarButton();
	}

	// Register Pins and nodes
	{
		ExtDependencyGraphPanelNodeFactory = MakeShareable(new FExtDependencyGraphPanelNodeFactory());
		FEdGraphUtilities::RegisterVisualNodeFactory(ExtDependencyGraphPanelNodeFactory);
#if ECB_TODO
		ExtDependencyGraphPanelPinFactory = MakeShareable(new FExtDependencyGraphPanelPinFactory());
		FEdGraphUtilities::RegisterVisualPinFactory(ExtDependencyGraphPanelPinFactory);
#endif
	}

	RegisterEditorDelegates();

	// Mount Sandbox
	MountSandbox();
}

FExtContentBrowserSingleton::~FExtContentBrowserSingleton()
{
#if ECB_LEGACY
	FEditorDelegates::LoadSelectedAssetsIfNeeded.RemoveAll(this);
#endif

	if ( FSlateApplication::IsInitialized() )
	{
		for ( int32 BrowserIdx = 0; BrowserIdx < UE_ARRAY_COUNT(ContentBrowserTabIDs); BrowserIdx++ )
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( ContentBrowserTabIDs[BrowserIdx] );
		}
	}

	ExtAssetRegistry.Shutdown();

	FExtContentBrowserStyle::Shutdown();

	FExtContentBrowserCommands::Unregister();

	// Un-Register Pins and nodes
	{
		if (ExtDependencyGraphPanelNodeFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualNodeFactory(ExtDependencyGraphPanelNodeFactory);
		}
#if ECB_TODO
		if (ExtDependencyGraphPanelPinFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualPinFactory(ExtDependencyGraphPanelPinFactory);
		}
#endif
	}

	UnregisterEditorDelegates();

	UnMountSandbox();
}

#if ECB_LEGACY

TSharedRef<SWidget> FContentBrowserSingleton::CreateAssetPicker(const FAssetPickerConfig& AssetPickerConfig)
{
	return SNew( SAssetPicker )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.AssetPickerConfig(AssetPickerConfig);
}

TSharedRef<SWidget> FContentBrowserSingleton::CreatePathPicker(const FPathPickerConfig& PathPickerConfig)
{
	return SNew( SPathPicker )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.PathPickerConfig(PathPickerConfig);
}

TSharedRef<class SWidget> FContentBrowserSingleton::CreateCollectionPicker(const FCollectionPickerConfig& CollectionPickerConfig)
{
	return SNew( SCollectionPicker )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.CollectionPickerConfig(CollectionPickerConfig);
}

void FContentBrowserSingleton::CreateOpenAssetDialog(const FOpenAssetDialogConfig& InConfig,
													 const FOnAssetsChosenForOpen& InOnAssetsChosenForOpen,
													 const FOnAssetDialogCancelled& InOnAssetDialogCancelled)
{
	const bool bModal = false;
	TSharedRef<SAssetDialog> AssetDialog = SNew(SAssetDialog, InConfig);
	AssetDialog->SetOnAssetsChosenForOpen(InOnAssetsChosenForOpen);
	AssetDialog->SetOnAssetDialogCancelled(InOnAssetDialogCancelled);
	SharedCreateAssetDialogWindow(AssetDialog, InConfig, bModal);
}

TArray<FAssetData> FContentBrowserSingleton::CreateModalOpenAssetDialog(const FOpenAssetDialogConfig& InConfig)
{
	struct FModalResults
	{
		void OnAssetsChosenForOpen(const TArray<FAssetData>& SelectedAssets)
		{
			SavedResults = SelectedAssets;
		}

		TArray<FAssetData> SavedResults;
	};

	FModalResults ModalWindowResults;
	FOnAssetsChosenForOpen OnAssetsChosenForOpenDelegate = FOnAssetsChosenForOpen::CreateRaw(&ModalWindowResults, &FModalResults::OnAssetsChosenForOpen);

	const bool bModal = true;
	TSharedRef<SAssetDialog> AssetDialog = SNew(SAssetDialog, InConfig);
	AssetDialog->SetOnAssetsChosenForOpen(OnAssetsChosenForOpenDelegate);
	SharedCreateAssetDialogWindow(AssetDialog, InConfig, bModal);

	return ModalWindowResults.SavedResults;
}

void FContentBrowserSingleton::CreateSaveAssetDialog(const FSaveAssetDialogConfig& InConfig,
													 const FOnObjectPathChosenForSave& InOnObjectPathChosenForSave,
													 const FOnAssetDialogCancelled& InOnAssetDialogCancelled)
{
	const bool bModal = false;
	TSharedRef<SAssetDialog> AssetDialog = SNew(SAssetDialog, InConfig);
	AssetDialog->SetOnObjectPathChosenForSave(InOnObjectPathChosenForSave);
	AssetDialog->SetOnAssetDialogCancelled(InOnAssetDialogCancelled);
	SharedCreateAssetDialogWindow(AssetDialog, InConfig, bModal);
}

FString FContentBrowserSingleton::CreateModalSaveAssetDialog(const FSaveAssetDialogConfig& InConfig)
{
	struct FModalResults
	{
		void OnObjectPathChosenForSave(const FString& ObjectPath)
		{
			SavedResult = ObjectPath;
		}

		FString SavedResult;
	};

	FModalResults ModalWindowResults;
	FOnObjectPathChosenForSave OnObjectPathChosenForSaveDelegate = FOnObjectPathChosenForSave::CreateRaw(&ModalWindowResults, &FModalResults::OnObjectPathChosenForSave);

	const bool bModal = true;
	TSharedRef<SAssetDialog> AssetDialog = SNew(SAssetDialog, InConfig);
	AssetDialog->SetOnObjectPathChosenForSave(OnObjectPathChosenForSaveDelegate);
	SharedCreateAssetDialogWindow(AssetDialog, InConfig, bModal);

	return ModalWindowResults.SavedResult;
}

bool FContentBrowserSingleton::HasPrimaryContentBrowser() const
{
	if ( PrimaryContentBrowser.IsValid() )
	{
		// There is a primary content browser
		return true;
	}
	else
	{
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if ( AllContentBrowsers[BrowserIdx].IsValid() )
			{
				// There is at least one valid content browser
				return true;
			}
		}

		// There were no valid content browsers
		return false;
	}
}

void FContentBrowserSingleton::FocusPrimaryContentBrowser(bool bFocusSearch)
{
	// See if the primary content browser is still valid
	if ( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}

	if ( PrimaryContentBrowser.IsValid() )
	{
		FocuSExtContentBrowser( PrimaryContentBrowser.Pin() );
	}
	else
	{
		// If we couldn't find a primary content browser, open one
		SummonNewBrowser();
	}

	// Do we also want to focus on the search box of the content browser?
	if ( bFocusSearch && PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->SetKeyboardFocusOnSearch();
	}
}

void FContentBrowserSingleton::CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory)
{
	FocusPrimaryContentBrowser(false);

	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->CreateNewAsset(DefaultAssetName, PackagePath, AssetClass, Factory);
	}
}

TSharedPtr<SExtContentBrowser> FContentBrowserSingleton::FindContentBrowserToSync(bool bAllowLockedBrowsers, const FName& InstanceName, bool bNewSpawnBrowser)
{
	TSharedPtr<SExtContentBrowser> ContentBrowserToSync;

	if (InstanceName.IsValid() && !InstanceName.IsNone())
	{
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if (AllContentBrowsers[BrowserIdx].IsValid() && AllContentBrowsers[BrowserIdx].Pin()->GetInstanceName() == InstanceName)
			{
				return AllContentBrowsers[BrowserIdx].Pin();
			}	
		}

		return ContentBrowserToSync;
	}

	if ( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}

	if ( PrimaryContentBrowser.IsValid() && (bAllowLockedBrowsers || !PrimaryContentBrowser.Pin()->IsLocked()) )
	{
		// If wanting to spawn a new browser window, don't set the BrowserToSync in order to summon a new browser
		if (!bNewSpawnBrowser)
		{
			// If the primary content browser is not locked, sync it
			ContentBrowserToSync = PrimaryContentBrowser.Pin();
		}
	}
	else
	{
		// If there is no primary or it is locked, find the first non-locked valid browser
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if ( AllContentBrowsers[BrowserIdx].IsValid() && (bAllowLockedBrowsers || !AllContentBrowsers[BrowserIdx].Pin()->IsLocked()) )
			{
				ContentBrowserToSync = AllContentBrowsers[BrowserIdx].Pin();
				break;
			}
		}
	}

	if ( !ContentBrowserToSync.IsValid() )
	{
		// There are no valid, unlocked browsers, attempt to summon a new one.
		const FName NewBrowserName = SummonNewBrowser(bAllowLockedBrowsers);

		// Now try to find a non-locked valid browser again, now that a new one may exist
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if ((AllContentBrowsers[BrowserIdx].IsValid() && (NewBrowserName == NAME_None && (bAllowLockedBrowsers || !AllContentBrowsers[BrowserIdx].Pin()->IsLocked()))) || (AllContentBrowsers[BrowserIdx].Pin()->GetInstanceName() == NewBrowserName))
			{
				ContentBrowserToSync = AllContentBrowsers[BrowserIdx].Pin();
				break;
			}
		}
	}

	if ( !ContentBrowserToSync.IsValid() )
	{
		UE_LOG( LogContentBrowser, Log, TEXT( "Unable to sync content browser, all browsers appear to be locked" ) );
	}

	return ContentBrowserToSync;
}

void FContentBrowserSingleton::SyncBrowserToAssets(const TArray<FAssetData>& AssetDataList, bool bAllowLockedBrowsers, bool bFocuSExtContentBrowser, const FName& InstanceName, bool bNewSpawnBrowser)
{
	TSharedPtr<SExtContentBrowser> ContentBrowserToSync = FindContentBrowserToSync(bAllowLockedBrowsers, InstanceName, bNewSpawnBrowser);

	if ( ContentBrowserToSync.IsValid() )
	{
		// Finally, focus and sync the browser that was found
		if (bFocuSExtContentBrowser)
		{
			FocuSExtContentBrowser(ContentBrowserToSync);
		}
		ContentBrowserToSync->SyncToAssets(AssetDataList);
	}
}

void FContentBrowserSingleton::SyncBrowserToAssets(const TArray<UObject*>& AssetList, bool bAllowLockedBrowsers, bool bFocuSExtContentBrowser, const FName& InstanceName, bool bNewSpawnBrowser)
{
	// Convert UObject* array to FAssetData array
	TArray<FAssetData> AssetDataList;
	for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
	{
		if ( AssetList[AssetIdx] )
		{
			AssetDataList.Add( FAssetData(AssetList[AssetIdx]) );
		}
	}

	SyncBrowserToAssets(AssetDataList, bAllowLockedBrowsers, bFocuSExtContentBrowser, InstanceName, bNewSpawnBrowser);
}

void FContentBrowserSingleton::SyncBrowserToFolders(const TArray<FString>& FolderList, bool bAllowLockedBrowsers, bool bFocuSExtContentBrowser, const FName& InstanceName, bool bNewSpawnBrowser)
{
	TSharedPtr<SExtContentBrowser> ContentBrowserToSync = FindContentBrowserToSync(bAllowLockedBrowsers, InstanceName, bNewSpawnBrowser);

	if ( ContentBrowserToSync.IsValid() )
	{
		// Finally, focus and sync the browser that was found
		if (bFocuSExtContentBrowser)
		{
			FocuSExtContentBrowser(ContentBrowserToSync);
		}
		ContentBrowserToSync->SyncToFolders(FolderList);
	}
}

void FContentBrowserSingleton::SyncBrowserTo(const FExtContentBrowserSelection& ItemSelection, bool bAllowLockedBrowsers, bool bFocuSExtContentBrowser, const FName& InstanceName, bool bNewSpawnBrowser)
{
	TSharedPtr<SExtContentBrowser> ContentBrowserToSync = FindContentBrowserToSync(bAllowLockedBrowsers, InstanceName, bNewSpawnBrowser);

	if ( ContentBrowserToSync.IsValid() )
	{
		// Finally, focus and sync the browser that was found
		if (bFocuSExtContentBrowser)
		{
			FocuSExtContentBrowser(ContentBrowserToSync);
		}
		ContentBrowserToSync->SyncTo(ItemSelection);
	}
}

#endif

void FExtContentBrowserSingleton::GetSelectedAssets(TArray<FExtAssetData>& SelectedAssets)
{
	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->GetSelectedAssets(SelectedAssets);
	}
}

#if ECB_LEGACY

void FContentBrowserSingleton::GetSelectedFolders(TArray<FString>& SelectedFolders)
{
	if (PrimaryContentBrowser.IsValid())
	{
		PrimaryContentBrowser.Pin()->GetSelectedFolders(SelectedFolders);
	}
}

void FContentBrowserSingleton::GetSelectedPathViewFolders(TArray<FString>& SelectedFolders)
{
	if (PrimaryContentBrowser.IsValid())
	{
		SelectedFolders = PrimaryContentBrowser.Pin()->GetSelectedPathViewFolders();
	}
}

void FContentBrowserSingleton::CaptureThumbnailFromViewport(FViewport* InViewport, TArray<FAssetData>& SelectedAssets)
{
	ContentBrowserUtils::CaptureThumbnailFromViewport(InViewport, SelectedAssets);
}

#endif

FExtContentBrowserSingleton& FExtContentBrowserSingleton::Get()
{
	static const FName ModuleName = "ExtContentBrowser";
	FExtContentBrowserModule& Module = FModuleManager::GetModuleChecked<FExtContentBrowserModule>(ModuleName);
	return static_cast<FExtContentBrowserSingleton&>(Module.Get());
}

FExtAssetRegistry& FExtContentBrowserSingleton::GetAssetRegistry()
{
	return ExtAssetRegistry;
}

FExtObjectThumbnailPool& FExtContentBrowserSingleton::GetThumbnailPool()
{
	return Get().ThumbnailPool;
}

FString FExtContentBrowserSingleton::GetPluginResourcesDir()
{
	return Get().PluginInfo.ResourcesDir;
}

FText FExtContentBrowserSingleton::GetPluginVersionText()
{
	return Get().PluginInfo.VersionName;
}

void FExtContentBrowserSingleton::SetPrimaryContentBrowser(const TSharedRef<SExtContentBrowser>& NewPrimaryBrowser)
{
	if ( PrimaryContentBrowser.IsValid() && PrimaryContentBrowser.Pin().ToSharedRef() == NewPrimaryBrowser )
	{
		// This is already the primary content browser
		return;
	}

	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->SetIsPrimaryContentBrowser(false);
	}

	PrimaryContentBrowser = NewPrimaryBrowser;
	NewPrimaryBrowser->SetIsPrimaryContentBrowser(true);
}

#if ECB_WIP_MULTI_INSTANCES
void FContentBrowserSingleton::ContentBrowserClosed(const TSharedRef<SExtContentBrowser>& ClosedBrowser)
{
	// Find the browser in the all browsers list
	for (int32 BrowserIdx = AllContentBrowsers.Num() - 1; BrowserIdx >= 0; --BrowserIdx)
	{
		if ( !AllContentBrowsers[BrowserIdx].IsValid() || AllContentBrowsers[BrowserIdx].Pin() == ClosedBrowser )
		{
			AllContentBrowsers.RemoveAt(BrowserIdx);
		}
	}

	if ( !PrimaryContentBrowser.IsValid() || ClosedBrowser == PrimaryContentBrowser.Pin() )
	{
		ChooseNewPrimaryBrowser();
	}

	BrowserToLastKnownTabManagerMap.Add(ClosedBrowser->GetInstanceName(), ClosedBrowser->GetTabManager());
}

void FContentBrowserSingleton::SharedCreateAssetDialogWindow(const TSharedRef<SAssetDialog>& AssetDialog, const FSharedAssetDialogConfig& InConfig, bool bModal) const
{
	const FVector2D DefaultWindowSize(1152.0f, 648.0f);
	const FVector2D WindowSize = InConfig.WindowSizeOverride.IsZero() ? DefaultWindowSize : InConfig.WindowSizeOverride;
	const FText WindowTitle = InConfig.DialogTitleOverride.IsEmpty() ? LOCTEXT("GenericAssetDialogWindowHeader", "Asset Dialog") : InConfig.DialogTitleOverride;

	TSharedRef<SWindow> DialogWindow =
		SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(WindowSize);

	DialogWindow->SetContent(AssetDialog);

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if (MainFrameParentWindow.IsValid())
	{
		if (bModal)
		{
			FSlateApplication::Get().AddModalWindow(DialogWindow, MainFrameParentWindow.ToSharedRef());
		}
		else if (FGlobalTabmanager::Get()->GetRootWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(DialogWindow, MainFrameParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(DialogWindow);
		}
	}
	else
	{
		if (ensureMsgf(!bModal, TEXT("Could not create asset dialog because modal windows must have a parent and this was called at a time where the mainframe window does not exist.")))
		{
			FSlateApplication::Get().AddWindow(DialogWindow);
		}
	}
}

#if ECB_WIP_MULTI_INSTANCES
void FContentBrowserSingleton::ChooseNewPrimaryBrowser()
{
	// Find the first valid browser and trim any invalid browsers along the way
	for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
	{
		if ( AllContentBrowsers[BrowserIdx].IsValid() )
		{
			if (AllContentBrowsers[BrowserIdx].Pin()->CanSetAsPrimaryContentBrowser())
			{
				SetPrimaryContentBrowser(AllContentBrowsers[BrowserIdx].Pin().ToSharedRef());
				break;
			}
		}
		else
		{
			// Trim any invalid content browsers
			AllContentBrowsers.RemoveAt(BrowserIdx);
			BrowserIdx--;
		}
	}
}
#endif

void FContentBrowserSingleton::FocuSExtContentBrowser(const TSharedPtr<SExtContentBrowser>& BrowserToFocus)
{
	if ( BrowserToFocus.IsValid() )
	{
		TSharedRef<SExtContentBrowser> Browser = BrowserToFocus.ToSharedRef();
		TSharedPtr<FTabManager> TabManager = Browser->GetTabManager();
		if ( TabManager.IsValid() )
		{
			TabManager->InvokeTab(Browser->GetInstanceName());
		}
	}
}

FName FContentBrowserSingleton::SummonNewBrowser(bool bAllowLockedBrowsers)
{
	TSet<FName> OpenBrowserIDs;

	// Find all currently open browsers to help find the first open slot
	for (int32 BrowserIdx = AllContentBrowsers.Num() - 1; BrowserIdx >= 0; --BrowserIdx)
	{
		const TWeakPtr<SExtContentBrowser>& Browser = AllContentBrowsers[BrowserIdx];
		if ( Browser.IsValid() )
		{
			OpenBrowserIDs.Add(Browser.Pin()->GetInstanceName());
		}
	}
	
	FName NewTabName;
	for ( int32 BrowserIdx = 0; BrowserIdx < UE_ARRAY_COUNT(ContentBrowserTabIDs); BrowserIdx++ )
	{
		FName TestTabID = ContentBrowserTabIDs[BrowserIdx];
		if ( !OpenBrowserIDs.Contains(TestTabID) && (bAllowLockedBrowsers || !IsLocked(TestTabID)) )
		{
			// Found the first index that is not currently open
			NewTabName = TestTabID;
			break;
		}
	}

	if ( NewTabName != NAME_None )
	{
		const TWeakPtr<FTabManager>& TabManagerToInvoke = BrowserToLastKnownTabManagerMap.FindRef(NewTabName);
		if ( TabManagerToInvoke.IsValid() )
		{
			TabManagerToInvoke.Pin()->InvokeTab(NewTabName);
		}
		else
		{
			FGlobalTabmanager::Get()->InvokeTab(NewTabName);
		}
	}
	else
	{
		// No available slots... don't summon anything
	}

	return NewTabName;
}
#endif
TSharedRef<SWidget> FExtContentBrowserSingleton::CreateContentBrowser( const FName InstanceName, TSharedPtr<SDockTab> ContainingTab)
{
	TSharedRef<SExtContentBrowser> NewBrowser =
		SNew(SExtContentBrowser, InstanceName)
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.ContainingTab( ContainingTab );
#if ECB_WIP_MULTI_INSTANCES
	AllContentBrowsers.Add( NewBrowser );

	if( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}
#else
	SetPrimaryContentBrowser(NewBrowser);
#endif
	return NewBrowser;
}

void FExtContentBrowserSingleton::RegisterEditorDelegates()
{
	FEditorDelegates::BeginPIE.AddRaw(this, &FExtContentBrowserSingleton::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FExtContentBrowserSingleton::OnEndPIE);
}

void FExtContentBrowserSingleton::UnregisterEditorDelegates()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
}

// Todo: alert sandbox usage
void FExtContentBrowserSingleton::OnBeginPIE(const bool bIsSimulating)
{
}

void FExtContentBrowserSingleton::OnEndPIE(const bool bIsSimulating)
{
}

void FExtContentBrowserSingleton::MountSandbox()
{
#if ECB_WIP_IMPORT_SANDBOX
	FPackageName::RegisterMountPoint(FExtAssetData::GetSandboxPackagePath() + TEXT("/"), FExtAssetData::GetSandboxDir());
#endif
}

void FExtContentBrowserSingleton::UnMountSandbox()
{
#if ECB_WIP_IMPORT_SANDBOX
	FPackageName::UnRegisterMountPoint(FExtAssetData::GetSandboxPackagePath() + TEXT("/"), FExtAssetData::GetSandboxDir());
#endif
}

void FExtContentBrowserSingleton::RegisterMenuAndTabSpawner()
{
	struct MenuGroupHelper
	{
		static TSharedRef<FWorkspaceItem> FindOrCreateGroup(const FText& InGroupName)
		{
			TSharedRef<FWorkspaceItem> MenuRoot = WorkspaceMenu::GetMenuStructure().GetStructureRoot();
			const TArray<TSharedRef<FWorkspaceItem>>& ChildItems = MenuRoot->GetChildItems();
			for (const TSharedRef<FWorkspaceItem>& ChildItem : ChildItems)
			{
				if (ChildItem->GetDisplayName().EqualTo(InGroupName))
				{
					return ChildItem;
				}
			}
			return MenuRoot->AddGroup(InGroupName);;
		}
	};

	// Register the tab spawner for all content browsers
	const FText NatesToolGroupName = LOCTEXT("NatesTools", "Nate's Tools");
	TSharedRef<FWorkspaceItem> NatesToolsMenuGroup = MenuGroupHelper::FindOrCreateGroup(NatesToolGroupName);

	//const FSlateIcon AssetBrowserIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.TabIcon");
	const FSlateIcon AssetBrowserIcon(FExtContentBrowserStyle::GetStyleSetName(), "UAssetBrowser.Icon16x");

	for (int32 BrowserIdx = 0; BrowserIdx < UE_ARRAY_COUNT(ContentBrowserTabIDs); BrowserIdx++)
	{
		const FName TabID = FName(*FString::Printf(TEXT("UAssetBrowserTab%d"), BrowserIdx + 1));
		ContentBrowserTabIDs[BrowserIdx] = TabID;

		const FText DefaultDisplayName = GetContentBrowserLabelWithIndex(BrowserIdx);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabID, FOnSpawnTab::CreateRaw(this, &FExtContentBrowserSingleton::SpawnContentBrowserTab, BrowserIdx))
#if ECB_WIP_MULTI_INSTANCES
			.SetDisplayName(DefaultDisplayName)
#else
			.SetDisplayName(LOCTEXT("UAssetBrowserTabTitle", "UAsset Browser"))
#endif
			.SetTooltipText(LOCTEXT("UAssetBrowserMenuTooltipText", "Open a UAsset Browser tab."))
			.SetIcon(AssetBrowserIcon)
			.SetGroup(NatesToolsMenuGroup)
			.SetMenuType(ETabSpawnerMenuType::Enabled);

	}
}

void FExtContentBrowserSingleton::RegisterCommands()
{
	FExtContentBrowserCommands::Register();

	ExtContentBrowserCommands = MakeShareable(new FUICommandList);

	ExtContentBrowserCommands->MapAction(FExtContentBrowserCommands::Get().OpenUAssetBrowser, FExecuteAction::CreateRaw(this, &FExtContentBrowserSingleton::OpenUAssetBrowserButtonClicked), FCanExecuteAction());
}

void FExtContentBrowserSingleton::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FExtContentBrowserCommands::Get().OpenUAssetBrowser);
}

void FExtContentBrowserSingleton::OpenUAssetBrowserButtonClicked() const
{
	const FName ExtContentBrowserTabName("UAssetBrowserTab1");
	//FGlobalTabmanager::Get()->InvokeTab(ExtContentBrowserTabName);
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	LevelEditorModule.GetLevelEditorTabManager()->TryInvokeTab(ExtContentBrowserTabName);
}

TSharedRef<SDockTab> FExtContentBrowserSingleton::SpawnContentBrowserTab( const FSpawnTabArgs& SpawnTabArgs, int32 BrowserIdx )
{	
	TAttribute<FText> Label = TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateRaw( this, &FExtContentBrowserSingleton::GetContentBrowserTabLabel, BrowserIdx ) );

	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(Label);
		//.ToolTip( IDocumentation::Get()->CreateToolTip( Label, nullptr, "Shared/ContentBrowser", "Tab" ) );

	TSharedRef<SWidget> NewBrowser = CreateContentBrowser( SpawnTabArgs.GetTabId().TabType, NewTab);

#if ECB_WIP_MULTI_INSTANCES
	if ( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}
#endif

#if ECB_LEGACY
	// Add wrapper for tutorial highlighting
	TSharedRef<SBox> Wrapper =
		SNew(SBox)
		.AddMetaData<FTutorialMetaData>(FTutorialMetaData(TEXT("ContentBrowser"), TEXT("ContentBrowserTab1")))
		[
			NewBrowser
		];

	NewTab->SetContent( Wrapper );
#endif

	NewTab->SetContent(NewBrowser);
	return NewTab;
}

#if ECB_WIP_LOCK
bool FContentBrowserSingleton::IsLocked(const FName& InstanceName) const
{
	// First try all the open browsers, as their locked state might be newer than the configs
	for (int32 AllBrowsersIdx = AllContentBrowsers.Num() - 1; AllBrowsersIdx >= 0; --AllBrowsersIdx)
	{
		const TWeakPtr<SExtContentBrowser>& Browser = AllContentBrowsers[AllBrowsersIdx];
		if ( Browser.IsValid() && Browser.Pin()->GetInstanceName() == InstanceName )
		{
			return Browser.Pin()->IsLocked();
		}
	}

	// Fallback to getting the locked state from the config instead
	bool bIsLocked = false;
	GConfig->GetBool(*SExtContentBrowser::SettingsIniSection, *(InstanceName.ToString() + TEXT(".Locked")), bIsLocked, GEditorPerProjectIni);
	return bIsLocked;
}
#endif

FText FExtContentBrowserSingleton::GetContentBrowserLabelWithIndex( int32 BrowserIdx )
{
	return FText::Format(LOCTEXT("UAssetBrowserTabNameWithIndex", "UAsset Browser {0}"), FText::AsNumber(BrowserIdx + 1));
}

void FExtContentBrowserSingleton::ToggleShowToolbarButton()
{
	const bool bShowToolbarButton = !GetDefault<UExtContentBrowserSettings>()->DisplayToolbarButton;
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayToolbarButton = bShowToolbarButton;

	if (bShowToolbarButton)
	{
		if (!ToolbarExtension.IsValid())
		{
//			ToolbarExtender = MakeShareable(new FExtender);
			ToolbarExtension = ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, ExtContentBrowserCommands, FToolBarExtensionDelegate::CreateRaw(this, &FExtContentBrowserSingleton::AddToolbarExtension));

// 			FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
// 			LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		}
	}
	else
	{
		if (ToolbarExtension.IsValid())
		{
			ToolbarExtender->RemoveExtension(ToolbarExtension.ToSharedRef());
// 			FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
// 			LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolbarExtender);
		}
		ToolbarExtension = nullptr;
//		ToolbarExtender = nullptr;
	}
}

FText FExtContentBrowserSingleton::GetContentBrowserTabLabel(int32 BrowserIdx)
{
#if ECB_WIP_MULTI_INSTANCES
	int32 NumOpenContentBrowsers = 0;
	for (int32 AllBrowsersIdx = AllContentBrowsers.Num() - 1; AllBrowsersIdx >= 0; --AllBrowsersIdx)
	{
		const TWeakPtr<SExtContentBrowser>& Browser = AllContentBrowsers[AllBrowsersIdx];
		if ( Browser.IsValid() )
		{
			NumOpenContentBrowsers++;
		}
		else
		{
			AllContentBrowsers.RemoveAt(AllBrowsersIdx);
		}
	}

	if ( NumOpenContentBrowsers > 1 )
	{
		return GetContentBrowserLabelWithIndex( BrowserIdx );
	}
	else
#endif
	{
		return LOCTEXT("UAssetBrowserTabName", "UAsset Browser");
	}
}

#if ECB_WIP_MULTI_INSTANCES
void FContentBrowserSingleton::SetSelectedPaths(const TArray<FString>& FolderPaths, bool bNeedsRefresh/* = false*/)
{
	// Make sure we have a valid browser
	if (!PrimaryContentBrowser.IsValid())
	{
		ChooseNewPrimaryBrowser();

		if (!PrimaryContentBrowser.IsValid())
		{
			SummonNewBrowser();
		}
	}

	if (PrimaryContentBrowser.IsValid())
	{
		PrimaryContentBrowser.Pin()->SetSelectedPaths(FolderPaths, bNeedsRefresh);
	}
}
#endif

/////////////////////////////////////
// FPluginfo Impl
//

FExtContentBrowserPluginInfo::FExtContentBrowserPluginInfo()
{
	TSharedPtr<IPlugin> MyPlugin = IPluginManager::Get().FindPlugin(TEXT("UAssetBrowser"));
	FriendlyName = FText::FromString(MyPlugin->GetDescriptor().FriendlyName);
	VersionName = FText::FromString(TEXT("v") + MyPlugin->GetDescriptor().VersionName);
	CreatedBy = FText::FromString(TEXT("by ") + MyPlugin->GetDescriptor().CreatedBy);

	ResourcesDir = MyPlugin->GetBaseDir() / TEXT("Resources");
}

#undef LOCTEXT_NAMESPACE

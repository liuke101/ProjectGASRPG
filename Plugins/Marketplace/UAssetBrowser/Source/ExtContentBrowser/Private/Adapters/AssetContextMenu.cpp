// Copyright 2017-2021 marynate. All Rights Reserved.

#include "AssetContextMenu.h"
#include "ExtAssetData.h"
#include "SExtAssetView.h"
#include "SMetaDataView.h"
#include "ExtContentBrowserCommands.h"
#include "ExtContentBrowserUtils.h"
#include "ExtContentBrowserMenuContexts.h"
#include "ExtContentBrowserSingleton.h"

#include "Templates/SubclassOf.h"
#include "Styling/SlateTypes.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/FileManager.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/MetaData.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "EditorStyleSet.h"
#include "EditorReimportHandler.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealClient.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/Material.h"
#include "SourceControlOperations.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "Settings/EditorExperimentalSettings.h"
#include "Materials/MaterialInstanceConstant.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Dialogs/Dialogs.h"

#include "ObjectTools.h"
#include "PackageTools.h"
#include "Editor.h"

#include "PropertyEditorModule.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "ConsolidateWindow.h"
#include "ReferencedAssetsUtils.h"
#include "Internationalization/PackageLocalizationUtil.h"
#include "Internationalization/TextLocalizationResource.h"

#include "SourceControlWindows.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ComponentAssetBroker.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include "SourceCodeNavigation.h"
#include "IDocumentation.h"
#include "EditorClassUtils.h"

#include "Internationalization/Culture.h"
#include "Internationalization/TextPackageNamespaceUtil.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/LevelStreaming.h"

#include "PackageHelperFunctions.h"
#include "EngineUtils.h"
#include "Subsystems/AssetEditorSubsystem.h"

#include "Commandlets/TextAssetCommandlet.h"
#include "Misc/FileHelper.h"

// Collection
//#include "CollectionAssetManagement.h"

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

FAssetContextMenu::FAssetContextMenu(const TWeakPtr<SExtAssetView>& InAssetView)
	: AssetView(InAssetView)
	, bAtLeastOneNonRedirectorSelected(false)
	, bAtLeastOneClassSelected(false)
	, bCanExecuteSCCMerge(false)
	, bCanExecuteSCCCheckOut(false)
	, bCanExecuteSCCOpenForAdd(false)
	, bCanExecuteSCCCheckIn(false)
	, bCanExecuteSCCHistory(false)
	, bCanExecuteSCCRevert(false)
	, bCanExecuteSCCSync(false)
{
	
}

void FAssetContextMenu::BindCommands(TSharedPtr< FUICommandList >& Commands)
{
	Commands->MapAction(FGlobalEditorCommonCommands::Get().FindInContentBrowser, FUIAction(
		FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteSyncToAssetTree),
		FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteSyncToAssetTree)
		));
}

TSharedRef<SWidget> FAssetContextMenu::MakeContextMenu(const TArray<FExtAssetData>& InSelectedAssets, const FSourcesData& InSourcesData, TSharedPtr< FUICommandList > InCommandList)
{
	SetSelectedAssets(InSelectedAssets);
	SourcesData = InSourcesData;
#if ECB_LEGACY

	// Cache any vars that are used in determining if you can execute any actions.
	// Useful for actions whose "CanExecute" will not change or is expensive to calculate.
	CacheCanExecuteVars();

	// Get all menu extenders for this context menu from the content browser module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	TArray<FContentBrowserMenuExtender_SelectedAssets> MenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
#endif
	TArray<TSharedPtr<FExtender>> Extenders;
#if ECB_LEGACY
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(SelectedAssets));
		}
	}
#endif
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	UExtContentBrowserAssetContextMenuContext* ContextObject = NewObject<UExtContentBrowserAssetContextMenuContext>();
	ContextObject->AssetContextMenu = SharedThis(this);

	UToolMenus* ToolMenus = UToolMenus::Get();

	static const FName BaseMenuName("ExtContentBrowser.AssetContextMenu");
	RegisterContextMenu(BaseMenuName);

	TArray<UObject*> SelectedObjects;

	// Create menu hierarchy based on class hierarchy
	FName MenuName = BaseMenuName;
	{

		// Objects must be loaded for this operation... for now
		TArray<FString> ObjectPaths;
		for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
		{
			ObjectPaths.Add(SelectedAssets[AssetIdx].ObjectPath.ToString());
		}

		ContextObject->SelectedObjects.Reset();

		ContextObject->SetNumSelectedObjects(SelectedAssets.Num());
#if ECB_LEGACY
		if (ContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, SelectedObjects) && SelectedObjects.Num() > 0)
		{
			ContextObject->SelectedObjects.Append(SelectedObjects);

			// Find common class for selected objects
			UClass* CommonClass = SelectedObjects[0]->GetClass();
			for (int32 ObjIdx = 1; ObjIdx < SelectedObjects.Num(); ++ObjIdx)
			{
				while (!SelectedObjects[ObjIdx]->IsA(CommonClass))
				{
					CommonClass = CommonClass->GetSuperClass();
				}
			}
			ContextObject->CommonClass = CommonClass;

			MenuName = UToolMenus::JoinMenuPaths(BaseMenuName, CommonClass->GetFName());

			RegisterMenuHierarchy(CommonClass);

			// Find asset actions for common class
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TSharedPtr<IAssetTypeActions> CommonAssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(ContextObject->CommonClass).Pin();
			if (CommonAssetTypeActions.IsValid() && CommonAssetTypeActions->HasActions(SelectedObjects))
			{
				ContextObject->CommonAssetTypeActions = CommonAssetTypeActions;
			}
		}
#endif
	}

	FToolMenuContext MenuContext(InCommandList, MenuExtender, ContextObject);
	return ToolMenus->GenerateWidget(MenuName, MenuContext);
}

void FAssetContextMenu::RegisterMenuHierarchy(UClass* InClass)
{
	static const FName BaseMenuName("ContentBrowser.AssetContextMenu");

	UToolMenus* ToolMenus = UToolMenus::Get();

	for (UClass* CurrentClass = InClass; CurrentClass; CurrentClass = CurrentClass->GetSuperClass())
	{
		FName CurrentMenuName = UToolMenus::JoinMenuPaths(BaseMenuName, CurrentClass->GetFName());
		if (!ToolMenus->IsMenuRegistered(CurrentMenuName))
		{
			FName ParentMenuName;
			UClass* ParentClass = CurrentClass->GetSuperClass();
			if (ParentClass == UObject::StaticClass() || ParentClass == nullptr)
			{
				ParentMenuName = BaseMenuName;
			}
			else
			{
				ParentMenuName = UToolMenus::JoinMenuPaths(BaseMenuName, ParentClass->GetFName());
			}

			ToolMenus->RegisterMenu(CurrentMenuName, ParentMenuName);

			if (ParentMenuName == BaseMenuName)
			{
				break;
			}
		}
	}
}

void FAssetContextMenu::RegisterContextMenu(const FName MenuName)
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus->IsMenuRegistered(MenuName))
	{
		UToolMenu* Menu = ToolMenus->RegisterMenu(MenuName);
		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

		Section.AddDynamicEntry("GetActions", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			UExtContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UExtContentBrowserAssetContextMenuContext>();
			if (Context && Context->CommonAssetTypeActions.IsValid())
			{
				Context->CommonAssetTypeActions.Pin()->GetActions(Context->GetSelectedObjects(), InSection);
			}
		}));

		Section.AddDynamicEntry("GetActionsLegacy", FNewToolMenuDelegateLegacy::CreateLambda([](FMenuBuilder& MenuBuilder, UToolMenu* InMenu)
		{
			UExtContentBrowserAssetContextMenuContext* Context = InMenu->FindContext<UExtContentBrowserAssetContextMenuContext>();
			if (Context && Context->CommonAssetTypeActions.IsValid())
			{
				Context->CommonAssetTypeActions.Pin()->GetActions(Context->GetSelectedObjects(), MenuBuilder);
			}
		}));

		Menu->AddDynamicSection("AddMenuOptions", FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
		{
			UExtContentBrowserAssetContextMenuContext* Context = InMenu->FindContext<UExtContentBrowserAssetContextMenuContext>();
			if (Context && Context->AssetContextMenu.IsValid())
			{
				Context->AssetContextMenu.Pin()->AddMenuOptions(InMenu);
			}
		}));
	}
}

void FAssetContextMenu::AddMenuOptions(UToolMenu* InMenu)
{
	UExtContentBrowserAssetContextMenuContext* Context = InMenu->FindContext<UExtContentBrowserAssetContextMenuContext>();
#if ECB_LEGACY
	if (!Context || Context->SelectedObjects.Num() == 0)
#else
	if (!Context || Context->GetNumSelectedObjects() == 0)
#endif
	{
		return;
	}

	// Add any type-specific context menu options
	AddAssetTypeMenuOptions(InMenu, Context->SelectedObjects.Num() > 0);

	// Add imported asset context menu options
	//AddImportedAssetMenuOptions(InMenu);

	// Add quick access to common commands.
	AddCommonMenuOptions(InMenu);

	// Add quick access to view commands
	AddExploreMenuOptions(InMenu);

	// Add reference options
	AddReferenceMenuOptions(InMenu);

	// Add copy full path options
	AddCopyMenuOptions(InMenu);

#if ECB_WIP_COLLECTION
	// Add collection options
	AddCollectionMenuOptions(InMenu);
#endif

#if ECB_LEGACY
	// Add documentation options
	AddDocumentationMenuOptions(InMenu);

	// Add source control options
	AddSourceControlMenuOptions(InMenu);
#endif
}

void FAssetContextMenu::SetSelectedAssets(const TArray<FExtAssetData>& InSelectedAssets)
{
	SelectedAssets = InSelectedAssets;
}

#if ECB_LEGACY
void FAssetContextMenu::SetOnFindInAssetTreeRequested(const FOnFindInAssetTreeRequested& InOnFindInAssetTreeRequested)
{
	OnFindInAssetTreeRequested = InOnFindInAssetTreeRequested;
}
#endif

void FAssetContextMenu::SetOnRenameRequested(const FOnRenameRequested& InOnRenameRequested)
{
	OnRenameRequested = InOnRenameRequested;
}

void FAssetContextMenu::SetOnRenameFolderRequested(const FOnRenameFolderRequested& InOnRenameFolderRequested)
{
	OnRenameFolderRequested = InOnRenameFolderRequested;
}

void FAssetContextMenu::SetOnDuplicateRequested(const FOnDuplicateRequested& InOnDuplicateRequested)
{
	OnDuplicateRequested = InOnDuplicateRequested;
}

void FAssetContextMenu::SetOnAssetViewRefreshRequested(const FOnAssetViewRefreshRequested& InOnAssetViewRefreshRequested)
{
	OnAssetViewRefreshRequested = InOnAssetViewRefreshRequested;
}

bool FAssetContextMenu::AddImportedAssetMenuOptions(UToolMenu* Menu)
{
#if ECB_LEGACY
	if (AreImportedAssetActionsVisible())
	{
		TArray<FString> ResolvedFilePaths;
		TArray<FString> SourceFileLabels;
		int32 ValidSelectedAssetCount = 0;
		GetSelectedAssetSourceFilePaths(ResolvedFilePaths, SourceFileLabels, ValidSelectedAssetCount);

		FToolMenuSection& Section = Menu->AddSection("ImportedAssetActions", LOCTEXT("ImportedAssetActionsMenuHeading", "Imported Asset"));
		{
			auto CreateSubMenu = [this](UToolMenu* SubMenu, bool bReimportWithNewFile)
			{
				//Get the data, we cannot use the closure since the lambda will be call when the function scope will be gone
				TArray<FString> ResolvedFilePaths;
				TArray<FString> SourceFileLabels;
				int32 ValidSelectedAssetCount = 0;
				GetSelectedAssetSourceFilePaths(ResolvedFilePaths, SourceFileLabels, ValidSelectedAssetCount);
				if (SourceFileLabels.Num() > 0 )
				{
					FToolMenuSection& SubSection = SubMenu->AddSection("Section");
					for (int32 SourceFileIndex = 0; SourceFileIndex < SourceFileLabels.Num(); ++SourceFileIndex)
					{
						FText ReimportLabel = FText::Format(LOCTEXT("ReimportNoLabel", "SourceFile {0}"), SourceFileIndex);
						FText ReimportLabelTooltip;
						if (ValidSelectedAssetCount == 1)
						{
							ReimportLabelTooltip = FText::Format(LOCTEXT("ReimportNoLabelTooltip", "Reimport File: {0}"), FText::FromString(ResolvedFilePaths[SourceFileIndex]));
						}
						if (SourceFileLabels[SourceFileIndex].Len() > 0)
						{
							ReimportLabel = FText::Format(LOCTEXT("ReimportLabel", "{0}"), FText::FromString(SourceFileLabels[SourceFileIndex]));
							if (ValidSelectedAssetCount == 1)
							{
								ReimportLabelTooltip = FText::Format(LOCTEXT("ReimportLabelTooltip", "Reimport {0} File: {1}"), FText::FromString(SourceFileLabels[SourceFileIndex]), FText::FromString(ResolvedFilePaths[SourceFileIndex]));
							}
						}
						if (bReimportWithNewFile)
						{
							SubSection.AddMenuEntry(
								NAME_None,
								ReimportLabel,
								ReimportLabelTooltip,
								FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.ReimportAsset"),
								FUIAction(
									FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteReimportWithNewFile, SourceFileIndex),
									FCanExecuteAction()
								)
							);
						}
						else
						{
							SubSection.AddMenuEntry(
								NAME_None,
								ReimportLabel,
								ReimportLabelTooltip,
								FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.ReimportAsset"),
								FUIAction(
									FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteReimport, SourceFileIndex),
									FCanExecuteAction()
								)
							);
						}
						
					}
				}
			};

			// Show Source In Explorer
			Section.AddMenuEntry(
				"FindSourceFile",
				LOCTEXT("FindSourceFile", "Open Source Location"),
				LOCTEXT("FindSourceFileTooltip", "Opens the folder containing the source of the selected asset(s)."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.OpenSourceLocation"),
				FUIAction(
					FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteFindSourceInExplorer, ResolvedFilePaths),
					FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteImportedAssetActions, ResolvedFilePaths)
				)
			);
			// Open In External Editor
			Section.AddMenuEntry(
				"OpenInExternalEditor",
				LOCTEXT("OpenInExternalEditor", "Open In External Editor"),
				LOCTEXT("OpenInExternalEditorTooltip", "Open the selected asset(s) in the default external editor."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.OpenInExternalEditor"),
				FUIAction(
					FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteOpenInExternalEditor, ResolvedFilePaths),
					FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteImportedAssetActions, ResolvedFilePaths)
					)
				);
		}

		return true;
	}
#endif
	return false;
}

bool FAssetContextMenu::AddCommonMenuOptions(UToolMenu* Menu)
{
	//FToolMenuSection& Section = Menu->AddSection("CommonAssetActions", LOCTEXT("CommonAssetActionsMenuHeading", "Common"));
	FToolMenuSection& ImportSection = Menu->AddSection("ImportAssetActions", LOCTEXT("ImportAssetActionsMenuHeading", "Import"));
	{
		// Validate
		ImportSection.AddMenuEntry(
			"Validate",
			LOCTEXT("Validate", "Validate"),
			LOCTEXT("ValidateTooltip", "Validate selected uasset files"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteValidateAsset),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteValidateAsset)
			)
		);

		// Import
		ImportSection.AddMenuEntry(
			"Import",
			LOCTEXT("Import", "Import"),
			LOCTEXT("ImportAssetToolTip", "Import .uasset files..."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteImportAsset),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteImportAsset)
			)
		);

		// Flat Import
		ImportSection.AddMenuEntry(
			"FlatImport",
			LOCTEXT("FlatImport", "Flat Import"),
			LOCTEXT("FlatImportTooltip", "Collect dependencies and import all into one selected folder"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteFlatImportAsset),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteFlatImportAsset)
			)
		);

		// Direct Copy
		ImportSection.AddMenuEntry(
			"DirectCopy",
			LOCTEXT("DirectCopy", "Direct Copy"),
			LOCTEXT("DirectCopyTooltip", "Copy selected files to current project's corresonpding folder without dependency files"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteDirectCopyAsset),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteDirectCopyAsset)
			)
		);


	}

	FToolMenuSection& ParseSection = Menu->AddSection("ReActions", LOCTEXT("ReActionsMenuHeading", "Re"));
	{
		// ReValidate
		ParseSection.AddMenuEntry(
			"Revalidate",
			LOCTEXT("Revalidate", "Revalidate"),
			LOCTEXT("RevalidateTooltip", "Revalidate selected uasset files"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteRevalidateAsset),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteRevalidateAsset)
			)
		);
#if ECB_WIP_REPARSE_ASSET
		// Reparse Asset
		ParseSection.AddMenuEntry(
			"ReparseAsset",
			LOCTEXT("ReparseAsset", "Reparse Asset"),
			LOCTEXT("ReparseAssetTooltip", "Reparse selected assets."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteReparseAsset),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteReparseAsset)
			)
		);
#endif
	}

#if ECB_LEGACY
	int32 NumAssetItems, NumClassItems;
	ContentBrowserUtils::CountItemTypes(SelectedAssets, NumAssetItems, NumClassItems);

	{
		FToolMenuSection& Section = Menu->AddSection("CommonAssetActions", LOCTEXT("CommonAssetActionsMenuHeading", "Common"));

		// Edit
		Section.AddMenuEntry(
			"EditAsset",
			LOCTEXT("EditAsset", "Edit..."),
			LOCTEXT("EditAssetTooltip", "Opens the selected asset(s) for edit."),
			FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Edit"),
			FUIAction( FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteEditAsset) )
			);
	
		// Only add these options if assets are selected
		if (NumAssetItems > 0)
		{
			// Rename
			Section.AddMenuEntry(FGenericCommands::Get().Rename,
				LOCTEXT("Rename", "Rename"),
				LOCTEXT("RenameTooltip", "Rename the selected asset."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Rename")
				);

			// Duplicate
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate,
				LOCTEXT("Duplicate", "Duplicate"),
				LOCTEXT("DuplicateTooltip", "Create a copy of the selected asset(s)."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Duplicate")
				);

			// Save
			Section.AddMenuEntry(FContentBrowserCommands::Get().SaveSelectedAsset,
				LOCTEXT("SaveAsset", "Save"),
				LOCTEXT("SaveAssetTooltip", "Saves the asset to file."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "Level.SaveIcon16x")
				);

			// Delete
			Section.AddMenuEntry(FGenericCommands::Get().Delete,
				LOCTEXT("Delete", "Delete"),
				LOCTEXT("DeleteTooltip", "Delete the selected assets."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Delete")
				);

			// Asset Actions sub-menu
			Section.AddSubMenu(
				"AssetActionsSubMenu",
				LOCTEXT("AssetActionsSubMenuLabel", "Asset Actions"),
				LOCTEXT("AssetActionsSubMenuToolTip", "Other asset actions"),
				FNewToolMenuDelegate::CreateSP(this, &FAssetContextMenu::MakeAssetActionsSubMenu),
				FUIAction(
					FExecuteAction(),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteAssetActions )
					),
				EUserInterfaceActionType::Button,
				false, 
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions")
				);

			if (NumClassItems == 0)
			{
				// Asset Localization sub-menu
				Section.AddSubMenu(
					"LocalizationSubMenu",
					LOCTEXT("LocalizationSubMenuLabel", "Asset Localization"),
					LOCTEXT("LocalizationSubMenuToolTip", "Manage the localization of this asset"),
					FNewToolMenuDelegate::CreateSP(this, &FAssetContextMenu::MakeAssetLocalizationSubMenu),
					FUIAction(),
					EUserInterfaceActionType::Button,
					false,
					FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetLocalization")
					);
			}
		}
	}
#endif

	return true;
}

void FAssetContextMenu::AddExploreMenuOptions(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection("AssetContextExploreMenuOptions", LOCTEXT("AssetContextExploreMenuOptionsHeading", "Explore"));
	{
#if ECB_LEGACY
		// Find in Content Browser
		Section.AddMenuEntry(
			FGlobalEditorCommonCommands::Get().FindInContentBrowser, 
			LOCTEXT("ShowInFolderView", "Show in Folder View"),
			LOCTEXT("ShowInFolderViewTooltip", "Selects the folder that contains this asset in the Content Browser Sources Panel.")
			);
#endif

		// Find in Explorer
		Section.AddMenuEntry(
			"FindInExplorer",
			ExtContentBrowserUtils::GetExploreFolderText(),
			LOCTEXT("FindInExplorerTooltip", "Finds this asset on disk"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteFindInExplorer ),
				FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteFindInExplorer )
				)
			);
#if ECB_FEA_SYNC_IN_CB
		// Sync in Content Browser
		Section.AddMenuEntry(
			"FindinContentBrowser",
			LOCTEXT("FindinContentBrowser", "Find in Content Browser"),
			LOCTEXT("FindinContentBrowserTooltip", "Find if same uasset file exists in Content Browser."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteSyncToContentBrowser),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteSyncToContentBrowser)
			)
		);
#endif

#if ECB_WIP_IMPORT_FOR_DUMP
		// Dump assets via export
		Section.AddMenuEntry(
			"ExportRaw",
			LOCTEXT("ExportRaw", "Export Raw"),
			LOCTEXT("ExportRawTooltip", "Export raw aseet data from selected uasset file."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteDumpExport),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteDumpExport)
			)
		);
#endif
	}
}

void FAssetContextMenu::MakeAssetActionsSubMenu(UToolMenu* Menu)
{	
#if ECB_LEGACY
	{
		FToolMenuSection& Section = Menu->AddSection("AssetActionsSection");

		// Create BP Using This
		Section.AddMenuEntry(
			"CreateBlueprintUsing",
			LOCTEXT("CreateBlueprintUsing", "Create Blueprint Using This..."),
			LOCTEXT("CreateBlueprintUsingTooltip", "Create a new Blueprint and add this asset to it"),
			FSlateIcon(FAppStyle::GetStyleSetName(), "LevelEditor.CreateClassBlueprint"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteCreateBlueprintUsing),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteCreateBlueprintUsing)
			)
		);

		// Capture Thumbnail
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		if (SelectedAssets.Num() == 1 && AssetToolsModule.Get().AssetUsesGenericThumbnail(SelectedAssets[0]))
		{
			Section.AddMenuEntry(
				"CaptureThumbnail",
				LOCTEXT("CaptureThumbnail", "Capture Thumbnail"),
				LOCTEXT("CaptureThumbnailTooltip", "Captures a thumbnail from the active viewport."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.CreateThumbnail"),
				FUIAction(
					FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteCaptureThumbnail),
					FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteCaptureThumbnail)
				)
			);
		}

		// Clear Thumbnail
		if (CanClearCustomThumbnails())
		{
			Section.AddMenuEntry(
				"ClearCustomThumbnail",
				LOCTEXT("ClearCustomThumbnail", "Clear Thumbnail"),
				LOCTEXT("ClearCustomThumbnailTooltip", "Clears all custom thumbnails for selected assets."),
				FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.AssetActions.DeleteThumbnail"),
				FUIAction(FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteClearThumbnail))
			);
		}
	}

	// FIND ACTIONS
	{
		FToolMenuSection& Section = Menu->AddSection("AssetContextFindActions", LOCTEXT("AssetContextFindActionsMenuHeading", "Find"));
		// Select Actors Using This Asset
		Section.AddMenuEntry(
			"FindAssetInWorld",
			LOCTEXT("FindAssetInWorld", "Select Actors Using This Asset"),
			LOCTEXT("FindAssetInWorldTooltip", "Selects all actors referencing this asset."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteFindAssetInWorld),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteFindAssetInWorld)
				)
			);
	}

	// MOVE ACTIONS
	{
		FToolMenuSection& Section = Menu->AddSection("AssetContextMoveActions", LOCTEXT("AssetContextMoveActionsMenuHeading", "Move"));
		bool bHasExportableAssets = false;
		for (const FAssetData& AssetData : SelectedAssets)
		{
			const UObject* Object = AssetData.GetAsset();
			if (Object)
			{
				const UPackage* Package = Object->GetOutermost();
				if (!Package->HasAnyPackageFlags(EPackageFlags::PKG_DisallowExport))
				{
					bHasExportableAssets = true;
					break;
				}
			}
		}

		if (bHasExportableAssets)
		{
			// Export
			Section.AddMenuEntry(
				"Export",
				LOCTEXT("Export", "Export..."),
				LOCTEXT("ExportTooltip", "Export the selected assets to file."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteExport ) )
				);

			// Bulk Export
			if (SelectedAssets.Num() > 1)
			{
				Section.AddMenuEntry(
					"BulkExport",
					LOCTEXT("BulkExport", "Bulk Export..."),
					LOCTEXT("BulkExportTooltip", "Export the selected assets to file in the selected directory"),
					FSlateIcon(),
					FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteBulkExport ) )
					);
			}
		}

		// Migrate
		Section.AddMenuEntry(
			"MigrateAsset",
			LOCTEXT("MigrateAsset", "Migrate..."),
			LOCTEXT("MigrateAssetTooltip", "Copies all selected assets and their dependencies to another project"),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteMigrateAsset ) )
			);
	}

	// ADVANCED ACTIONS
	{
		FToolMenuSection& Section = Menu->AddSection("AssetContextAdvancedActions", LOCTEXT("AssetContextAdvancedActionsMenuHeading", "Advanced"));

		// Reload
		Section.AddMenuEntry(
			"Reload",
			LOCTEXT("Reload", "Reload"),
			LOCTEXT("ReloadTooltip", "Reload the selected assets from their file on disk."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteReload),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteReload)
			)
		);

		// Replace References
		if (CanExecuteConsolidate())
		{
			Section.AddMenuEntry(
				"ReplaceReferences",
				LOCTEXT("ReplaceReferences", "Replace References"),
				LOCTEXT("ConsolidateTooltip", "Replace references to the selected assets."),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteConsolidate)
				)
				);
		}

		// Property Matrix
		bool bCanUsePropertyMatrix = true;
		// Materials can't be bulk edited currently as they require very special handling because of their dependencies with the rendering thread, and we'd have to hack the property matrix too much.
		for (auto& Asset : SelectedAssets)
		{
			if (Asset.AssetClass == UMaterial::StaticClass()->GetFName() || Asset.AssetClass == UMaterialInstanceConstant::StaticClass()->GetFName() || Asset.AssetClass == UMaterialFunction::StaticClass()->GetFName() || Asset.AssetClass == UMaterialFunctionInstance::StaticClass()->GetFName())
			{
				bCanUsePropertyMatrix = false;
				break;
			}
		}

		if (bCanUsePropertyMatrix)
		{
			TAttribute<FText>::FGetter DynamicTooltipGetter;
			DynamicTooltipGetter.BindSP(this, &FAssetContextMenu::GetExecutePropertyMatrixTooltip);
			TAttribute<FText> DynamicTooltipAttribute = TAttribute<FText>::Create(DynamicTooltipGetter);

			Section.AddMenuEntry(
				"PropertyMatrix",
				LOCTEXT("PropertyMatrix", "Bulk Edit via Property Matrix..."),
				DynamicTooltipAttribute,
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecutePropertyMatrix),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecutePropertyMatrix)
				)
				);
		}

		// Create Metadata menu
		Section.AddMenuEntry(
			"ShowAssetMetaData",
			LOCTEXT("ShowAssetMetaData", "Show Metadata"),
			LOCTEXT("ShowAssetMetaDataTooltip", "Show the asset metadata dialog."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteShowAssetMetaData),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteShowAssetMetaData)
			)
		);

		// Chunk actions
		if (GetDefault<UEditorExperimentalSettings>()->bContextMenuChunkAssignments)
		{
			Section.AddMenuEntry(
				"AssignAssetChunk",
				LOCTEXT("AssignAssetChunk", "Assign to Chunk..."),
				LOCTEXT("AssignAssetChunkTooltip", "Assign this asset to a specific Chunk"),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteAssignChunkID) )
				);

			Section.AddSubMenu(
				"RemoveAssetFromChunk",
				LOCTEXT("RemoveAssetFromChunk", "Remove from Chunk..."),
				LOCTEXT("RemoveAssetFromChunkTooltip", "Removed an asset from a Chunk it's assigned to."),
				FNewToolMenuDelegate::CreateRaw(this, &FAssetContextMenu::MakeChunkIDListMenu)
				);

			Section.AddMenuEntry(
				"RemoveAllChunkAssignments",
				LOCTEXT("RemoveAllChunkAssignments", "Remove from all Chunks"),
				LOCTEXT("RemoveAllChunkAssignmentsTooltip", "Removed an asset from all Chunks it's assigned to."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteRemoveAllChunkID) )
				);
		}
	}

	if (GetDefault<UEditorExperimentalSettings>()->bTextAssetFormatSupport)
	{
		FToolMenuSection& FormatActionsSection = Menu->AddSection("AssetContextTextAssetFormatActions", LOCTEXT("AssetContextTextAssetFormatActionsHeading", "Text Assets"));
		{
			FormatActionsSection.AddMenuEntry(
				"ExportToTextFormat",
				LOCTEXT("ExportToTextFormat", "Export to text format"),
				LOCTEXT("ExportToTextFormatTooltip", "Exports the selected asset(s) to the experimental text asset format"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FAssetContextMenu::ExportSelectedAssetsToText))
			);

			FormatActionsSection.AddMenuEntry(
				"ViewSelectedAssetAsText",
				LOCTEXT("ViewSelectedAssetAsText", "View as text"),
				LOCTEXT("ViewSelectedAssetAsTextTooltip", "Opens a window showing the selected asset in text format"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FAssetContextMenu::ViewSelectedAssetAsText),
					FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanViewSelectedAssetAsText))
			);

			FormatActionsSection.AddMenuEntry(
				"ViewSelectedAssetAsText",
				LOCTEXT("TextFormatRountrip", "Run Text Asset Roundtrip"),
				LOCTEXT("TextFormatRountripTooltip", "Save the select asset backwards or forwards between text and binary formats and check for determinism"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FAssetContextMenu::DoTextFormatRoundtrip))
			);
		}
	}
#endif
}

void FAssetContextMenu::ExportSelectedAssetsToText()
{
#if ECB_LEGACY
	FString FailedPackage;
	for (const FExtAssetData& Asset : SelectedAssets)
	{
		UPackage* Package = Asset.GetPackage();
		FString Filename = FPackageName::LongPackageNameToFilename(Package->GetPathName(), FPackageName::GetTextAssetPackageExtension());
		if (!SavePackageHelper(Package, Filename))
		{
			FailedPackage = Package->GetPathName();
			break;
		}
	}

	if (FailedPackage.Len() > 0)
	{
		FNotificationInfo Info(LOCTEXT("ExportedTextAssetFailed", "Exported selected asset(s) failed"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		FNotificationInfo Info(LOCTEXT("ExportedTextAssetsSuccessfully", "Exported selected asset(s) successfully"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
#endif
}

bool FAssetContextMenu::CanExportSelectedAssetsToText() const
{
	return true;
}

bool FAssetContextMenu::CanExecuteAssetActions() const
{
	return !bAtLeastOneClassSelected;
}

void FAssetContextMenu::ExecuteFindInAssetTree(TArray<FName> InAssets)
{
#if ECB_LEGACY
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter ARFilter;
	ARFilter.ObjectPaths = MoveTemp(InAssets);
	
	TArray<FExtAssetData> FoundLocalizedAssetData;
	FExtContentBrowserSingleton::GetAssetRegistry().GetAssets(ARFilter, FoundLocalizedAssetData);

	OnFindInAssetTreeRequested.ExecuteIfBound(FoundLocalizedAssetData);
#endif
}

void FAssetContextMenu::ExecuteOpenEditorsForAssets(TArray<FName> InAssets)
{
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(InAssets);
}

bool FAssetContextMenu::AddReferenceMenuOptions(UToolMenu* Menu)
{
#if ECB_LEGACY
	MenuBuilder.BeginSection("AssetContExtDependencys", LOCTEXT("ReferencesMenuHeading", "References"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CopyReference", "Copy Reference"),
			LOCTEXT("CopyReferenceTooltip", "Copies reference paths for the selected assets to the clipboard."),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteCopyReference ) )
			);
	}
	MenuBuilder.EndSection();
#endif
	return true;
}

bool FAssetContextMenu::AddCopyMenuOptions(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection("AssetContextCopy", LOCTEXT("CoopyMenuHeading", "Copy"));
	{
		Section.AddMenuEntry(
			"CopyFilePath",
			LOCTEXT("CopyFilePath", "Copy File Path"),
			LOCTEXT("CopyFilePathTooltip", "Copies the file paths of the selected assets to the clipboard."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteCopyFilePath),
				FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanCopyFilePath)
			)
		);
	}

	return true;
}

bool FAssetContextMenu::AddAssetTypeMenuOptions(UToolMenu* Menu, bool bHasObjectsSelected)
{
	bool bAnyTypeOptions = false;
#if ECB_LEGACY
	if (bHasObjectsSelected)
	{
		// Label "GetAssetActions" section
		UExtContentBrowserAssetContextMenuContext* Context = Menu->FindContext<UExtContentBrowserAssetContextMenuContext>();
		if (Context)
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
			if (Context->CommonAssetTypeActions.IsValid())
			{
				Section.Label = FText::Format(NSLOCTEXT("AssetTools", "AssetSpecificOptionsMenuHeading", "{0} Actions"), Context->CommonAssetTypeActions.Pin()->GetName());
			}
			else if (Context->CommonClass)
			{
				Section.Label = FText::Format(NSLOCTEXT("AssetTools", "AssetSpecificOptionsMenuHeading", "{0} Actions"), FText::FromName(Context->CommonClass->GetFName()));
			}
			else
			{
				Section.Label = FText::Format(NSLOCTEXT("AssetTools", "AssetSpecificOptionsMenuHeading", "{0} Actions"), FText::FromString(TEXT("Asset")));
			}

			bAnyTypeOptions = true;
		}
	}
#endif
	return bAnyTypeOptions;
}

bool FAssetContextMenu::AddCollectionMenuOptions(FMenuBuilder& MenuBuilder)
{
#if ECB_LEGACY
	class FManageCollectionsContextMenu
	{
	public:
		static void CreateManageCollectionsSubMenu(FMenuBuilder& SubMenuBuilder, TSharedRef<FCollectionAssetManagement> QuickAssetManagement)
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			TArray<FCollectionNameType> AvailableCollections;
			CollectionManagerModule.Get().GetRootCollections(AvailableCollections);

			CreateManageCollectionsSubMenu(SubMenuBuilder, QuickAssetManagement, MoveTemp(AvailableCollections));
		}

		static void CreateManageCollectionsSubMenu(FMenuBuilder& SubMenuBuilder, TSharedRef<FCollectionAssetManagement> QuickAssetManagement, TArray<FCollectionNameType> AvailableCollections)
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			AvailableCollections.Sort([](const FCollectionNameType& One, const FCollectionNameType& Two) -> bool
			{
				return One.Name.LexicalLess(Two.Name);
			});

			for (const FCollectionNameType& AvailableCollection : AvailableCollections)
			{
				// Never display system collections
				if (AvailableCollection.Type == ECollectionShareType::CST_System)
				{
					continue;
				}

				// Can only manage assets for static collections
				ECollectionStorageMode::Type StorageMode = ECollectionStorageMode::Static;
				CollectionManagerModule.Get().GetCollectionStorageMode(AvailableCollection.Name, AvailableCollection.Type, StorageMode);
				if (StorageMode != ECollectionStorageMode::Static)
				{
					continue;
				}

				TArray<FCollectionNameType> AvailableChildCollections;
				CollectionManagerModule.Get().GetChildCollections(AvailableCollection.Name, AvailableCollection.Type, AvailableChildCollections);

				if (AvailableChildCollections.Num() > 0)
				{
					SubMenuBuilder.AddSubMenu(
						FText::FromName(AvailableCollection.Name), 
						FText::GetEmpty(), 
						FNewMenuDelegate::CreateStatic(&FManageCollectionsContextMenu::CreateManageCollectionsSubMenu, QuickAssetManagement, AvailableChildCollections),
						FUIAction(
							FExecuteAction::CreateStatic(&FManageCollectionsContextMenu::OnCollectionClicked, QuickAssetManagement, AvailableCollection),
							FCanExecuteAction::CreateStatic(&FManageCollectionsContextMenu::IsCollectionEnabled, QuickAssetManagement, AvailableCollection),
							FGetActionCheckState::CreateStatic(&FManageCollectionsContextMenu::GetCollectionCheckState, QuickAssetManagement, AvailableCollection)
							), 
						NAME_None, 
						EUserInterfaceActionType::ToggleButton,
						false,
						FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(AvailableCollection.Type))
						);
				}
				else
				{
					SubMenuBuilder.AddMenuEntry(
						FText::FromName(AvailableCollection.Name), 
						FText::GetEmpty(), 
						FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(AvailableCollection.Type)), 
						FUIAction(
							FExecuteAction::CreateStatic(&FManageCollectionsContextMenu::OnCollectionClicked, QuickAssetManagement, AvailableCollection),
							FCanExecuteAction::CreateStatic(&FManageCollectionsContextMenu::IsCollectionEnabled, QuickAssetManagement, AvailableCollection),
							FGetActionCheckState::CreateStatic(&FManageCollectionsContextMenu::GetCollectionCheckState, QuickAssetManagement, AvailableCollection)
							), 
						NAME_None, 
						EUserInterfaceActionType::ToggleButton
						);
				}
			}
		}

	private:
		static bool IsCollectionEnabled(TSharedRef<FCollectionAssetManagement> QuickAssetManagement, FCollectionNameType InCollectionKey)
		{
			return QuickAssetManagement->IsCollectionEnabled(InCollectionKey);
		}

		static ECheckBoxState GetCollectionCheckState(TSharedRef<FCollectionAssetManagement> QuickAssetManagement, FCollectionNameType InCollectionKey)
		{
			return QuickAssetManagement->GetCollectionCheckState(InCollectionKey);
		}

		static void OnCollectionClicked(TSharedRef<FCollectionAssetManagement> QuickAssetManagement, FCollectionNameType InCollectionKey)
		{
			// The UI actions don't give you the new check state, so we need to emulate the behavior of SCheckBox
			// Basically, checked will transition to unchecked (removing items), and anything else will transition to checked (adding items)
			if (GetCollectionCheckState(QuickAssetManagement, InCollectionKey) == ECheckBoxState::Checked)
			{
				QuickAssetManagement->RemoveCurrentAssetsFromCollection(InCollectionKey);
			}
			else
			{
				QuickAssetManagement->AddCurrentAssetsToCollection(InCollectionKey);
			}
		}
	};

	bool bHasAddedItems = false;

	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

	MenuBuilder.BeginSection("AssetContextCollections", LOCTEXT("AssetCollectionOptionsMenuHeading", "Collections"));

	// Show a sub-menu that allows you to quickly add or remove the current asset selection from the available collections
	if (CollectionManagerModule.Get().HasCollections())
	{
		TSharedRef<FCollectionAssetManagement> QuickAssetManagement = MakeShareable(new FCollectionAssetManagement());
		QuickAssetManagement->SetCurrentAssets(SelectedAssets);

		MenuBuilder.AddSubMenu(
			LOCTEXT("ManageCollections", "Manage Collections"),
			LOCTEXT("ManageCollections_ToolTip", "Manage the collections that the selected asset(s) belong to."),
			FNewMenuDelegate::CreateStatic(&FManageCollectionsContextMenu::CreateManageCollectionsSubMenu, QuickAssetManagement)
			);

		bHasAddedItems = true;
	}

	// "Remove from collection" (only display option if exactly one collection is selected)
	if ( SourcesData.Collections.Num() == 1 && !SourcesData.IsDynamicCollection() )
	{
		MenuBuilder.AddMenuEntry(
			FText::Format(LOCTEXT("RemoveFromCollectionFmt", "Remove From {0}"), FText::FromName(SourcesData.Collections[0].Name)),
			LOCTEXT("RemoveFromCollection_ToolTip", "Removes the selected asset from the current collection."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteRemoveFromCollection ),
				FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteRemoveFromCollection )
				)
			);

		bHasAddedItems = true;
	}

	MenuBuilder.EndSection();

	return bHasAddedItems;
#endif
	return false;
}

bool FAssetContextMenu::AreImportedAssetActionsVisible() const
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	
	// Check that all of the selected assets are imported
	for (auto& SelectedAsset : SelectedAssets)
	{
		auto AssetClass = SelectedAsset.GetClass();
		if (AssetClass)
		{
			auto AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetClass).Pin();
			if (!AssetTypeActions.IsValid() || !AssetTypeActions->IsImportedAsset())
			{
				return false;
			}
		}
	}

	return true;
}

bool FAssetContextMenu::CanExecuteImportedAssetActions(const TArray<FString> ResolvedFilePaths) const
{
	// Verify that all the file paths are legitimate
	for (const auto& SourceFilePath : ResolvedFilePaths)
	{
		if (!SourceFilePath.Len() || IFileManager::Get().FileSize(*SourceFilePath) == INDEX_NONE)
		{
			return false;
		}
	}

	return true;
}

void FAssetContextMenu::ExecuteFindSourceInExplorer(const TArray<FString> ResolvedFilePaths)
{
	// Open all files in the explorer
	for (const auto& SourceFilePath : ResolvedFilePaths)
	{
		FPlatformProcess::ExploreFolder(*FPaths::GetPath(SourceFilePath));
	}
}

void FAssetContextMenu::ExecuteOpenInExternalEditor(const TArray<FString> ResolvedFilePaths)
{
	// Open all files in their respective editor
	for (const auto& SourceFilePath : ResolvedFilePaths)
	{
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*SourceFilePath, NULL, ELaunchVerb::Edit);
	}
}

void FAssetContextMenu::GetSelectedAssetsByClass(TMap<UClass*, TArray<UObject*> >& OutSelectedAssetsByClass) const
{
	// Sort all selected assets by class
	for (const auto& SelectedAsset : SelectedAssets)
	{
		auto Asset = SelectedAsset.GetAsset();
		auto AssetClass = Asset->GetClass();

		if ( !OutSelectedAssetsByClass.Contains(AssetClass) )
		{
			OutSelectedAssetsByClass.Add(AssetClass);
		}
		
		OutSelectedAssetsByClass[AssetClass].Add(Asset);
	}
}

void FAssetContextMenu::GetSelectedAssetSourceFilePaths(TArray<FString>& OutFilePaths, TArray<FString>& OutUniqueSourceFileLabels, int32 &OutValidSelectedAssetCount) const
{
	OutFilePaths.Empty();
	OutUniqueSourceFileLabels.Empty();
	TMap<UClass*, TArray<UObject*> > SelectedAssetsByClass;
	GetSelectedAssetsByClass(SelectedAssetsByClass);
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	OutValidSelectedAssetCount = 0;
	// Get the source file paths for the assets of each type
	for (const auto& AssetsByClassPair : SelectedAssetsByClass)
	{
		const auto AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetsByClassPair.Key);
		if (AssetTypeActions.IsValid())
		{
			const auto& TypeAssets = AssetsByClassPair.Value;
			OutValidSelectedAssetCount += TypeAssets.Num();
			TArray<FString> AssetSourcePaths;
			AssetTypeActions.Pin()->GetResolvedSourceFilePaths(TypeAssets, AssetSourcePaths);
			OutFilePaths.Append(AssetSourcePaths);

			TArray<FString> AssetSourceLabels;
			AssetTypeActions.Pin()->GetSourceFileLabels(TypeAssets, AssetSourceLabels);
			for (const FString& Label : AssetSourceLabels)
			{
				OutUniqueSourceFileLabels.AddUnique(Label);
			}
		}
	}
}

void FAssetContextMenu::ExecuteSyncToAssetTree()
{
#if ECB_LEGACY
	// Copy this as the sync may adjust our selected assets array
	const TArray<FExtAssetData> SelectedAssetsCopy = SelectedAssets;
	OnFindInAssetTreeRequested.ExecuteIfBound(SelectedAssetsCopy);
#endif
}

void FAssetContextMenu::ExecuteCopyFilePath()
{
	ExtContentBrowserUtils::CopyFilePathsToClipboard(SelectedAssets);
}


void FAssetContextMenu::ExecuteFindInExplorer()
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
#if ECB_LEGACY
		const UObject* Asset = SelectedAssets[AssetIdx].GetAsset();
		if (Asset)
		{
			FExtAssetData AssetData(Asset);

			const FString PackageName = AssetData.PackageName.ToString();

			static const TCHAR* ScriptString = TEXT("/Script/");
			if (PackageName.StartsWith(ScriptString))
			{
				// Handle C++ classes specially, as FPackageName::LongPackageNameToFilename won't return the correct path in this case
				const FString ModuleName = PackageName.RightChop(FCString::Strlen(ScriptString));
				FString ModulePath;
				if (FSourceCodeNavigation::FindModulePath(ModuleName, ModulePath))
				{
					FString RelativePath;
					if (AssetData.GetTagValue("ModuleRelativePath", RelativePath))
					{
						const FString FullFilePath = FPaths::ConvertRelativePathToFull(ModulePath / (*RelativePath));
						FPlatformProcess::ExploreFolder(*FullFilePath);
					}
				}

				return;
			}

			const bool bIsWorldAsset = (AssetData.AssetClass == UWorld::StaticClass()->GetFName());
			const FString Extension = bIsWorldAsset ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
			const FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, Extension);
			const FString FullFilePath = FPaths::ConvertRelativePathToFull(FilePath);
			FPlatformProcess::ExploreFolder(*FullFilePath);
		}
#endif
		FExtAssetData& AssetData = SelectedAssets[AssetIdx];
		//if (AssetData.IsValid()) // Enable Explore Folder option even for Invalid Assets
		{
			const FString FullFilePath = AssetData.PackageFilePath.ToString();
			FPlatformProcess::ExploreFolder(*FullFilePath);
		}
	}
}

void FAssetContextMenu::ExecuteValidateAsset()
{
	if (SelectedAssets.Num() > 0)
	{
		FString ResultMessage;
		FExtAssetValidator::ValidateDependency(SelectedAssets, &ResultMessage, /*bShowProgess*/ true);

		if (SelectedAssets.Num() == 1)
		{
			ExtContentBrowserUtils::DisplayMessagePopup(FText::FromString(ResultMessage));
		}
		else
		{
			ExtContentBrowserUtils::NotifyMessage(FText::FromString(ResultMessage));
		}
	}
}

void FAssetContextMenu::ExecuteRevalidateAsset()
{
	if (SelectedAssets.Num() > 0)
	{
		FExtAssetValidator::InValidateDependency(SelectedAssets);

		ExecuteValidateAsset();
	}
}

void FAssetContextMenu::ExecuteImportAsset()
{
	if (SelectedAssets.Num() > 0)
	{
		FExtAssetImporter::ImportAssets(SelectedAssets, FUAssetImportSetting::GetSavedImportSetting());
	}
}

void FAssetContextMenu::ExecuteFlatImportAsset()
{
	if (SelectedAssets.Num() > 0)
	{
#if ECB_TODO // todo: double check flat import a level
		// Map is not supported
		for (const FExtAssetData& AssetData : SelectedAssets)
		{
			if (AssetData.AssetClass == UWorld::StaticClass()->GetFName())
			{
				ExtContentBrowserUtils::DisplayMessagePopup(FText::FromString(TEXT("Map is not supporting Flat Import.")));
				return;
			}
		}
#endif

		FUAssetImportSetting ImportSetting = FUAssetImportSetting::GetFlatModeImportSetting();
		FExtAssetImporter::ImportAssetsWithPathPicker(SelectedAssets, ImportSetting);
	}
}

void FAssetContextMenu::ExecuteDirectCopyAsset()
{
	FUAssetImportSetting ImportSetting = FUAssetImportSetting::GetDirectCopyModeImportSetting();
	FExtAssetImporter::ImportAssets(SelectedAssets, ImportSetting);
}

void FAssetContextMenu::ExecuteReparseAsset()
{
	if (SelectedAssets.Num() > 0 )
	{
		FScopedSlowTask SlowTask(SelectedAssets.Num(), LOCTEXT("ReparseAssets", "Reparse Assets.."));
		SlowTask.MakeDialog();

		FExtAssetRegistry& AssetRegistry = FExtContentBrowserSingleton::GetAssetRegistry();

		for (FExtAssetData& AssetData : SelectedAssets)
		{
			SlowTask.EnterProgressFrame();
			if (FExtAssetData* CachedAssetData = AssetRegistry.GetCachedAssetByFilePath(AssetData.PackageFilePath))
			{
				CachedAssetData->ReParse();
				AssetRegistry.BroadcastAssetUpdatedEvent(*CachedAssetData);
			}
		}
	}
}

void FAssetContextMenu::ExecuteSyncToContentBrowser()
{
	if (SelectedAssets.Num() == 1)
	{
		FExtAssetData& AssetData = SelectedAssets[0];
		if (AssetData.IsValid())
		{
			const bool bFound = FExtAssetDataUtil::SyncToContentBrowser(AssetData);
			if (!bFound)
			{
				ExtContentBrowserUtils::NotifyMessage(FText::FromString(TEXT("No corresponding asset found in Content Browser.")));
			}
		}
	}
}

void FAssetContextMenu::ExecuteCreateBlueprintUsing()
{
	if(SelectedAssets.Num() == 1)
	{
		UObject* Asset = SelectedAssets[0].GetAsset();
		FKismetEditorUtilities::CreateBlueprintUsingAsset(Asset, true);
	}
}

void FAssetContextMenu::GetSelectedAssets(TArray<UObject*>& Assets, bool SkipRedirectors) const
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		if (SkipRedirectors && (SelectedAssets[AssetIdx].AssetClass == UObjectRedirector::StaticClass()->GetFName()))
		{
			// Don't operate on Redirectors
			continue;
		}

		UObject* Object = SelectedAssets[AssetIdx].GetAsset();

		if (Object)
		{
			Assets.Add(Object);
		}
	}
}

/** Generates a reference graph of the world and can then find actors referencing specified objects */
struct WorldReferenceGenerator : public FFindReferencedAssets
{
	void BuildReferencingData()
	{
		MarkAllObjects();

		const int32 MaxRecursionDepth = 0;
		const bool bIncludeClasses = true;
		const bool bIncludeDefaults = false;
		const bool bReverseReferenceGraph = true;


		UWorld* World = GWorld;

		// Generate the reference graph for the world
		FReferencedAssets* WorldReferencer = new(Referencers)FReferencedAssets(World);
		FFindAssetsArchive(World, WorldReferencer->AssetList, &ReferenceGraph, MaxRecursionDepth, bIncludeClasses, bIncludeDefaults, bReverseReferenceGraph);

		// Also include all the streaming levels in the results
		for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
		{
			if (StreamingLevel)
			{
				if (ULevel* Level = StreamingLevel->GetLoadedLevel())
				{
					// Generate the reference graph for each streamed in level
					FReferencedAssets* LevelReferencer = new(Referencers) FReferencedAssets(Level);			
					FFindAssetsArchive(Level, LevelReferencer->AssetList, &ReferenceGraph, MaxRecursionDepth, bIncludeClasses, bIncludeDefaults, bReverseReferenceGraph);
				}
			}
		}

		TArray<UObject*> ReferencedObjects;
		// Special case for blueprints
		for (AActor* Actor : FActorRange(World))
		{
			ReferencedObjects.Reset();
			Actor->GetReferencedContentObjects(ReferencedObjects);
			for(UObject* Reference : ReferencedObjects)
			{
				TSet<UObject*>& Objects = ReferenceGraph.FindOrAdd(Reference);
				Objects.Add(Actor);
			}
		}
	}

	void MarkAllObjects()
	{
		// Mark all objects so we don't get into an endless recursion
		for (FThreadSafeObjectIterator It; It; ++It)
		{
			It->Mark(OBJECTMARK_TagExp);
		}
	}

	void Generate( const UObject* AssetToFind, TSet<const UObject*>& OutObjects )
	{
		// Don't examine visited objects
		if (!AssetToFind->HasAnyMarks(OBJECTMARK_TagExp))
		{
			return;
		}

		AssetToFind->UnMark(OBJECTMARK_TagExp);

		// Return once we find a parent object that is an actor
		if (AssetToFind->IsA(AActor::StaticClass()))
		{
			OutObjects.Add(AssetToFind);
			return;
		}

		// Traverse the reference graph looking for actor objects
		TSet<UObject*>* ReferencingObjects = ReferenceGraph.Find(AssetToFind);
		if (ReferencingObjects)
		{
			for(TSet<UObject*>::TConstIterator SetIt(*ReferencingObjects); SetIt; ++SetIt)
			{
				Generate(*SetIt, OutObjects);
			}
		}
	}
};

void FAssetContextMenu::ExecuteFindAssetInWorld()
{
	TArray<UObject*> AssetsToFind;
	const bool SkipRedirectors = true;
	GetSelectedAssets(AssetsToFind, SkipRedirectors);

	const bool NoteSelectionChange = true;
	const bool DeselectBSPSurfs = true;
	const bool WarnAboutManyActors = false;
	GEditor->SelectNone(NoteSelectionChange, DeselectBSPSurfs, WarnAboutManyActors);

	if (AssetsToFind.Num() > 0)
	{
		FScopedSlowTask SlowTask(2 + AssetsToFind.Num(), NSLOCTEXT("AssetContextMenu", "FindAssetInWorld", "Finding actors that use this asset..."));
		SlowTask.MakeDialog();

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		TSet<const UObject*> OutObjects;
		WorldReferenceGenerator ObjRefGenerator;

		SlowTask.EnterProgressFrame();
		ObjRefGenerator.BuildReferencingData();

		for (UObject* AssetToFind : AssetsToFind)
		{
			SlowTask.EnterProgressFrame();
			ObjRefGenerator.MarkAllObjects();
			ObjRefGenerator.Generate(AssetToFind, OutObjects);
		}

		SlowTask.EnterProgressFrame();

		if (OutObjects.Num() > 0)
		{
			const bool InSelected = true;
			const bool Notify = false;

			// Select referencing actors
			for (const UObject* Object : OutObjects)
			{
				GEditor->SelectActor(const_cast<AActor*>(CastChecked<AActor>(Object)), InSelected, Notify);
			}

			GEditor->NoteSelectionChange();
		}
		else
		{
			FNotificationInfo Info(LOCTEXT("NoReferencingActorsFound", "No actors found."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}
}

void FAssetContextMenu::ExecutePropertyMatrix()
{
	TArray<UObject*> ObjectsForPropertiesMenu;
	const bool SkipRedirectors = true;
	GetSelectedAssets(ObjectsForPropertiesMenu, SkipRedirectors);

	if ( ObjectsForPropertiesMenu.Num() > 0 )
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
		PropertyEditorModule.CreatePropertyEditorToolkit(TSharedPtr<IToolkitHost>(), ObjectsForPropertiesMenu);
	}
}

void FAssetContextMenu::ExecuteShowAssetMetaData()
{
	for (const FExtAssetData& AssetData : SelectedAssets)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Asset)
		{
			TMap<FName, FString>* TagValues = UMetaData::GetMapForObject(Asset);
			if (TagValues)
			{
				// Create and display a resizable window to display the MetaDataView for each asset with metadata
				FString Title = FString::Printf(TEXT("Metadata: %s"), *AssetData.AssetName.ToString());

				TSharedPtr< SWindow > Window = SNew(SWindow)
					.Title(FText::FromString(Title))
					.SupportsMaximize(false)
					.SupportsMinimize(false)
					.MinWidth(500.0f)
					.MinHeight(250.0f)
					[
						SNew(SBorder)
						.Padding(4.f)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SMetaDataView, *TagValues)
						]
					];

				FSlateApplication::Get().AddWindow(Window.ToSharedRef());
			}
		}
	}
}

void FAssetContextMenu::ExecuteEditAsset()
{
	TMap<UClass*, TArray<UObject*> > SelectedAssetsByClass;
	GetSelectedAssetsByClass(SelectedAssetsByClass);

	// Open 
	for (const auto& AssetsByClassPair : SelectedAssetsByClass)
	{
		const auto& TypeAssets = AssetsByClassPair.Value;
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(TypeAssets);
	}
}

void FAssetContextMenu::ExecuteSaveAsset()
{
	TArray<UPackage*> PackagesToSave;
	GetSelectedPackages(PackagesToSave);

	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
}

void FAssetContextMenu::ExecuteDiffSelected() const
{
	if (SelectedAssets.Num() >= 2)
	{
		UObject* FirstObjectSelected = SelectedAssets[0].GetAsset();
		UObject* SecondObjectSelected = SelectedAssets[1].GetAsset();

		if ((FirstObjectSelected != NULL) && (SecondObjectSelected != NULL))
		{
			// Load the asset registry module
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

			FRevisionInfo CurrentRevision; 
			CurrentRevision.Revision = TEXT("");

			AssetToolsModule.Get().DiffAssets(FirstObjectSelected, SecondObjectSelected, CurrentRevision, CurrentRevision);
		}
	}
}

bool FAssetContextMenu::CanExecuteReload() const
{
#if ECB_LEGACY
	TArray< FExtAssetData > AssetViewSelectedAssets = AssetView.Pin()->GetSelectedAssets();
	TArray< FString > SelectedFolders = AssetView.Pin()->GetSelectedFolders();

	int32 NumAssetItems, NumClassItems;
	ContentBrowserUtils::CountItemTypes(AssetViewSelectedAssets, NumAssetItems, NumClassItems);

	int32 NumAssetPaths, NumClassPaths;
	ContentBrowserUtils::CountPathTypes(SelectedFolders, NumAssetPaths, NumClassPaths);

	bool bHasSelectedCollections = false;
	for (const FString& SelectedFolder : SelectedFolders)
	{
		if (ContentBrowserUtils::IsCollectionPath(SelectedFolder))
		{
			bHasSelectedCollections = true;
			break;
		}
	}

	// We can't reload classes, or folders containing classes, or any collection folders
	return ((NumAssetItems > 0 && NumClassItems == 0) || (NumAssetPaths > 0 && NumClassPaths == 0)) && !bHasSelectedCollections;
#endif
	return false;
}

void FAssetContextMenu::ExecuteReload()
{
#if ECB_LEGACY
	// Don't allow asset reload during PIE
	if (GIsEditor)
	{
		UEditorEngine* Editor = GEditor;
		FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext();
		if (PIEWorldContext)
		{
			FNotificationInfo Notification(LOCTEXT("CannotReloadAssetInPIE", "Assets cannot be reloaded while in PIE."));
			Notification.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Notification);
			return;
		}
	}

	TArray<FExtAssetData> AssetViewSelectedAssets = AssetView.Pin()->GetSelectedAssets();
	if (AssetViewSelectedAssets.Num() > 0)
	{
		TArray<UPackage*> PackagesToReload;

		for (auto AssetIt = AssetViewSelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FExtAssetData& AssetData = *AssetIt;

			if (AssetData.AssetClass == UObjectRedirector::StaticClass()->GetFName())
			{
				// Don't operate on Redirectors
				continue;
			}

			if (AssetData.AssetClass == UUserDefinedStruct::StaticClass()->GetFName())
			{
				FNotificationInfo Notification(LOCTEXT("CannotReloadUserStruct", "User created structures cannot be safely reloaded."));
				Notification.ExpireDuration = 3.0f;
				FSlateNotificationManager::Get().AddNotification(Notification);
				continue;
			}

			if (AssetData.AssetClass == UUserDefinedEnum::StaticClass()->GetFName())
			{
				FNotificationInfo Notification(LOCTEXT("CannotReloadUserEnum", "User created enumerations cannot be safely reloaded."));
				Notification.ExpireDuration = 3.0f;
				FSlateNotificationManager::Get().AddNotification(Notification);
				continue;
			}

			PackagesToReload.AddUnique(AssetData.GetPackage());
		}

		if (PackagesToReload.Num() > 0)
		{
			UPackageTools::ReloadPackages(PackagesToReload);
		}
	}
#endif
}

void FAssetContextMenu::ExecuteMigrateAsset()
{
	// Get a list of package names for input into MigratePackages
	TArray<FName> PackageNames;
	PackageNames.Reserve(SelectedAssets.Num());
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		PackageNames.Add(SelectedAssets[AssetIdx].PackageName);
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().MigratePackages( PackageNames );
}

void FAssetContextMenu::ExecuteGoToCodeForAsset(UClass* SelectedClass)
{
	if (SelectedClass)
	{
		FString ClassHeaderPath;
		if( FSourceCodeNavigation::FindClassHeaderPath( SelectedClass, ClassHeaderPath ) && IFileManager::Get().FileSize( *ClassHeaderPath ) != INDEX_NONE )
		{
			const FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*ClassHeaderPath);
			FSourceCodeNavigation::OpenSourceFile( AbsoluteHeaderPath );
		}
	}
}

void FAssetContextMenu::ExecuteGoToDocsForAsset(UClass* SelectedClass)
{
	ExecuteGoToDocsForAsset(SelectedClass, FString());
}

void FAssetContextMenu::ExecuteGoToDocsForAsset(UClass* SelectedClass, const FString ExcerptSection)
{
	if (SelectedClass)
	{
		FString DocumentationLink = FEditorClassUtils::GetDocumentationLink(SelectedClass, ExcerptSection);
		if (!DocumentationLink.IsEmpty())
		{
			IDocumentation::Get()->Open(DocumentationLink, FDocumentationSourceInfo(TEXT("cb_docs")));
		}
	}
}

void FAssetContextMenu::ExecuteCopyReference()
{
	//ContentBrowserUtils::CopyAssetReferencesToClipboard(SelectedAssets);
}

void FAssetContextMenu::ExecuteCopyTextToClipboard(FString InText)
{
	FPlatformApplicationMisc::ClipboardCopy(*InText);
}

void FAssetContextMenu::ExecuteShowLocalizationCache(const FString InPackageFilename)
{
	FString CachedLocalizationId;
	TArray<FGatherableTextData> GatherableTextDataArray;

	// Read the localization data from the cache in the package header
	{
		TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*InPackageFilename));
		if (FileReader)
		{
			// Read package file summary from the file
			FPackageFileSummary PackageFileSummary;
			*FileReader << PackageFileSummary;

			CachedLocalizationId = PackageFileSummary.LocalizationId;

			if (PackageFileSummary.GatherableTextDataOffset > 0)
			{
				FileReader->Seek(PackageFileSummary.GatherableTextDataOffset);

				GatherableTextDataArray.SetNum(PackageFileSummary.GatherableTextDataCount);
				for (int32 GatherableTextDataIndex = 0; GatherableTextDataIndex < PackageFileSummary.GatherableTextDataCount; ++GatherableTextDataIndex)
				{
					*FileReader << GatherableTextDataArray[GatherableTextDataIndex];
				}
			}
		}
	}

	// Convert the gathered text array into a readable format
	FString LocalizationCacheStr = FString::Printf(TEXT("Package: %s"), *CachedLocalizationId);
	for (const FGatherableTextData& GatherableTextData : GatherableTextDataArray)
	{
		if (LocalizationCacheStr.Len() > 0)
		{
			LocalizationCacheStr += TEXT("\n\n");
		}

		FString KeysStr;
		FString EditorOnlyKeysStr;
		for (const FTextSourceSiteContext& TextSourceSiteContext : GatherableTextData.SourceSiteContexts)
		{
			FString* KeysStrPtr = TextSourceSiteContext.IsEditorOnly ? &EditorOnlyKeysStr : &KeysStr;
			if (KeysStrPtr->Len() > 0)
			{
				*KeysStrPtr += TEXT(", ");
			}
			*KeysStrPtr += TextSourceSiteContext.KeyName;
		}

		LocalizationCacheStr += FString::Printf(TEXT("Namespace: %s\n"), *GatherableTextData.NamespaceName);
		if (KeysStr.Len() > 0)
		{
			LocalizationCacheStr += FString::Printf(TEXT("Keys: %s\n"), *KeysStr);
		}
		if (EditorOnlyKeysStr.Len() > 0)
		{
			LocalizationCacheStr += FString::Printf(TEXT("Keys (Editor-Only): %s\n"), *EditorOnlyKeysStr);
		}
		LocalizationCacheStr += FString::Printf(TEXT("Source: %s"), *GatherableTextData.SourceData.SourceString);
	}

	// Generate a message box for the result
	SGenericDialogWidget::OpenDialog(LOCTEXT("LocalizationCache", "Localization Cache"), 
		SNew(SBox)
		.MaxDesiredWidth(800.0f)
		.MaxDesiredHeight(400.0f)
		[
			SNew(SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.AutoWrapText(true)
			.Text(FText::AsCultureInvariant(LocalizationCacheStr))
		],
		SGenericDialogWidget::FArguments()
		.UseScrollBox(false)
	);
}

void FAssetContextMenu::ExecuteDumpExport()
{
	FUAssetImportSetting ImportSetting = FUAssetImportSetting::GetSandboxImportForDumpSetting();
	FExtAssetImporter::ImportAssets(SelectedAssets, ImportSetting);
}

bool FAssetContextMenu::CanExecuteDumpExport() const
{
	return SelectedAssets.Num() == 1;
}

void FAssetContextMenu::ExecuteExport()
{
	TArray<UObject*> ObjectsToExport;
	const bool SkipRedirectors = false;
	GetSelectedAssets(ObjectsToExport, SkipRedirectors);

	if ( ObjectsToExport.Num() > 0 )
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

		AssetToolsModule.Get().ExportAssetsWithDialog(ObjectsToExport, true);
	}
}

void FAssetContextMenu::ExecuteBulkExport()
{
	TArray<UObject*> ObjectsToExport;
	const bool SkipRedirectors = false;
	GetSelectedAssets(ObjectsToExport, SkipRedirectors);

	if ( ObjectsToExport.Num() > 0 )
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

		AssetToolsModule.Get().ExportAssetsWithDialog(ObjectsToExport, false);
	}
}

void FAssetContextMenu::ExecuteRemoveFromCollection()
{
	if ( ensure(SourcesData.Collections.Num() == 1) )
	{
		TArray<FSoftObjectPath> AssetsToRemove;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			AssetsToRemove.Add((*AssetIt).GetSoftObjectPath());
		}

		if ( AssetsToRemove.Num() > 0 )
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			const FCollectionNameType& Collection = SourcesData.Collections[0];
			CollectionManagerModule.Get().RemoveFromCollection(Collection.Name, Collection.Type, AssetsToRemove);
			OnAssetViewRefreshRequested.ExecuteIfBound();
		}
	}
}

void FAssetContextMenu::ExecuteEnableSourceControl()
{
	ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modeless);
}

bool FAssetContextMenu::CanExecuteSyncToAssetTree() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteSyncToContentBrowser() const
{
	return SelectedAssets.Num() == 1;
}

bool FAssetContextMenu::CanCopyFilePath() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteFindInExplorer() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteValidateAsset() const
{
	for (const auto& Asset : SelectedAssets)
	{
		if (Asset.CanImportFast())
		{
			return true;
		}
	}
	return false;
}

bool FAssetContextMenu::CanExecuteRevalidateAsset() const
{
	return CanExecuteValidateAsset();
}

bool FAssetContextMenu::CanExecuteImportAsset() const
{
	return CanExecuteValidateAsset();
}

bool FAssetContextMenu::CanExecuteFlatImportAsset() const
{
	return CanExecuteValidateAsset();
}

bool FAssetContextMenu::CanExecuteDirectCopyAsset() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteReparseAsset() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteCreateBlueprintUsing() const
{
	// Only work if you have a single asset selected
	if(SelectedAssets.Num() == 1)
	{
		UObject* Asset = SelectedAssets[0].GetAsset();
		// See if we know how to make a component from this asset
		TArray< TSubclassOf<UActorComponent> > ComponentClassList = FComponentAssetBrokerage::GetComponentsForAsset(Asset);
		return (ComponentClassList.Num() > 0);
	}

	return false;
}

bool FAssetContextMenu::CanExecuteFindAssetInWorld() const
{
	return bAtLeastOneNonRedirectorSelected;
}

bool FAssetContextMenu::CanExecuteProperties() const
{
	return bAtLeastOneNonRedirectorSelected;
}

bool FAssetContextMenu::CanExecutePropertyMatrix(FText& OutErrorMessage) const
{
	bool bResult = bAtLeastOneNonRedirectorSelected;
	if (bAtLeastOneNonRedirectorSelected)
	{
		TArray<UObject*> ObjectsForPropertiesMenu;
		const bool SkipRedirectors = true;
		GetSelectedAssets(ObjectsForPropertiesMenu, SkipRedirectors);

		// Ensure all Blueprints are valid.
		for (UObject* Object : ObjectsForPropertiesMenu)
		{
			if (UBlueprint* BlueprintObj = Cast<UBlueprint>(Object))
			{
				if (BlueprintObj->GeneratedClass == nullptr)
				{
					OutErrorMessage = LOCTEXT("InvalidBlueprint", "A selected Blueprint is invalid.");
					bResult = false;
					break;
				}
			}
		}
	}
	return bResult;
}

bool FAssetContextMenu::CanExecutePropertyMatrix() const
{
	FText ErrorMessageDummy;
	return CanExecutePropertyMatrix(ErrorMessageDummy);
}

FText FAssetContextMenu::GetExecutePropertyMatrixTooltip() const
{
	FText ResultTooltip;
	if (CanExecutePropertyMatrix(ResultTooltip))
	{
		ResultTooltip = LOCTEXT("PropertyMatrixTooltip", "Opens the property matrix editor for the selected assets.");
	}
	return ResultTooltip;
}

bool FAssetContextMenu::CanExecuteShowAssetMetaData() const
{
	TArray<UObject*> ObjectsForPropertiesMenu;
	const bool SkipRedirectors = true;
	GetSelectedAssets(ObjectsForPropertiesMenu, SkipRedirectors);

	bool bResult = false;
	for (const UObject* Asset : ObjectsForPropertiesMenu)
	{
		if (Asset && UMetaData::GetMapForObject(Asset))
		{
			bResult = true;
			break;
		}
	}
	return bResult;
}

bool FAssetContextMenu::CanExecuteRename() const
{
	return false;
}

bool FAssetContextMenu::CanExecuteDelete() const
{
	return false;
}

bool FAssetContextMenu::CanExecuteRemoveFromCollection() const 
{
	return SourcesData.Collections.Num() == 1 && !SourcesData.IsDynamicCollection();
}

bool FAssetContextMenu::CanExecuteSaveAsset() const
{
	if ( bAtLeastOneClassSelected )
	{
		return false;
	}

	TArray<UPackage*> Packages;
	GetSelectedPackages(Packages);

	// only enabled if at least one selected package is loaded at all
	for (int32 PackageIdx = 0; PackageIdx < Packages.Num(); ++PackageIdx)
	{
		if ( Packages[PackageIdx] != NULL )
		{
			return true;
		}
	}

	return false;
}

bool FAssetContextMenu::CanExecuteDiffSelected() const
{
	return false;
}

bool FAssetContextMenu::CanClearCustomThumbnails() const
{
	return false;
}

void FAssetContextMenu::GetSelectedPackageNames(TArray<FString>& OutPackageNames) const
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		OutPackageNames.Add(SelectedAssets[AssetIdx].PackageName.ToString());
	}
}

void FAssetContextMenu::GetSelectedPackages(TArray<UPackage*>& OutPackages) const
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		UPackage* Package = FindPackage(NULL, *SelectedAssets[AssetIdx].PackageName.ToString());

		if ( Package )
		{
			OutPackages.Add(Package);
		}
	}
}

#undef LOCTEXT_NAMESPACE

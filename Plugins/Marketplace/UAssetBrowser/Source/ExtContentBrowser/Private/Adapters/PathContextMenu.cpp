// Copyright 2017-2021 marynate. All Rights Reserved.

#include "PathContextMenu.h"
#include "ExtContentBrowser.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtContentBrowserUtils.h"
#include "ExtContentBrowserCommands.h"
#include "Widgets/SExtPathView.h"

#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/PackageName.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Textures/SlateIcon.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Colors/SColorBlock.h"
#include "EditorStyleSet.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ContentBrowserModule.h"
#include "Misc/FileHelper.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Widgets/Input/SDirectoryPicker.h"



#define LOCTEXT_NAMESPACE "ExtContentBrowser"


FPathContextMenu::FPathContextMenu(const TWeakPtr<SWidget>& InParentContent)
	: ParentContent(InParentContent)
	, bCanExecuteRootDirsActions(false)
	, bHasSelectedPath(false)
{
	
}

void FPathContextMenu::SetOnRenameFolderRequested(const FOnRenameFolderRequested& InOnRenameFolderRequested)
{
	OnRenameFolderRequested = InOnRenameFolderRequested;
}

void FPathContextMenu::SetOnFolderDeleted(const FOnFolderDeleted& InOnFolderDeleted)
{
	OnFolderDeleted = InOnFolderDeleted;
}

void FPathContextMenu::SetOnFolderFavoriteToggled(const FOnFolderFavoriteToggled& InOnFolderFavoriteToggled)
{
	OnFolderFavoriteToggled = InOnFolderFavoriteToggled;
}

const TArray<FString>& FPathContextMenu::GetSelectedPaths() const
{
	return SelectedPaths;
}

void FPathContextMenu::SetSelectedPaths(const TArray<FString>& InSelectedPaths)
{
	SelectedPaths = InSelectedPaths;
}

TSharedRef<FExtender> FPathContextMenu::MakePathViewContextMenuExtender(const TArray<FString>& InSelectedPaths)
{
	// Cache any vars that are used in determining if you can execute any actions.
	// Useful for actions whose "CanExecute" will not change or is expensive to calculate.
	CacheCanExecuteVars();

	// Get all menu extenders for this context menu from the content browser module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	TArray<FContentBrowserMenuExtender_SelectedPaths> MenuExtenderDelegates = ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			//Extenders.Add(MenuExtenderDelegates[i].Execute( InSelectedPaths ));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	MenuExtender->AddMenuExtension("FolderContext", EExtensionHook::After, TSharedPtr<FUICommandList>(), FMenuExtensionDelegate::CreateSP(this, &FPathContextMenu::MakePathViewContextMenu));

	return MenuExtender.ToSharedRef();
}

void FPathContextMenu::MakePathViewContextMenu(FMenuBuilder& MenuBuilder)
{
	int32 NumAssetPaths = SelectedPaths.Num();
	int32 NumClassPaths = 0;

	// Only add something if at least one folder is selected
	if ( NumAssetPaths > 0 )
	{
		const bool bHasAssetPaths = NumAssetPaths > 0;
		const bool bHasClassPaths = NumClassPaths > 0;

		// Root folder operations section //
		if (CanExecuteRootDirsActions())
		{
			MenuBuilder.BeginSection("PathViewFolderOptions", LOCTEXT("PathViewRootOptionsMenuHeading", "Root Folder Options"));
			{
#if ECB_FEA_RELOAD_ROOT_FOLDER
				// Reload content folder
				MenuBuilder.AddMenuEntry(
					LOCTEXT("Reload", "Reload"),
					LOCTEXT("ReloadRootFolderTooltip", "Reload selected root content folder"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteReloadRootFolder))
				);
#endif

				// Remove content folder
				MenuBuilder.AddMenuEntry(
					LOCTEXT("Remove", "Remove"),
					LOCTEXT("RemoveRootFolderTooltip", "Remove selected root content folder"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteRemoveRootFolder))
				);
			}
			MenuBuilder.EndSection();
		}

		// Common operations section //
		MenuBuilder.BeginSection("PathViewFolderOptions", LOCTEXT("PathViewOptionsMenuHeading", "Folder Options") );
		{
			if(bHasAssetPaths)
			{
				FText NewAssetToolTip;
				if(SelectedPaths.Num() == 1)
				{
					if(CanCreateAsset())
					{
						NewAssetToolTip = FText::Format(LOCTEXT("NewAssetTooltip_CreateIn", "Create a new asset in {0}."), FText::FromString(SelectedPaths[0]));
					}
					else
					{
						NewAssetToolTip = FText::Format(LOCTEXT("NewAssetTooltip_InvalidPath", "Cannot create new assets in {0}."), FText::FromString(SelectedPaths[0]));
					}
				}
				else
				{
					NewAssetToolTip = LOCTEXT("NewAssetTooltip_InvalidNumberOfPaths", "Can only create assets when there is a single path selected.");
				}
#if ECB_LEGACY
				// New Asset (submenu)
				MenuBuilder.AddSubMenu(
					LOCTEXT( "NewAssetLabel", "New Asset" ),
					NewAssetToolTip,
					FNewMenuDelegate::CreateRaw( this, &FPathContextMenu::MakeNewAssetSubMenu ),
					FUIAction(
						FExecuteAction(),
						FCanExecuteAction::CreateRaw( this, &FPathContextMenu::CanCreateAsset )
						),
					NAME_None,
					EUserInterfaceActionType::Button,
					false,
					FSlateIcon()
					);
#endif
			}

#if ECB_LEGACY
			if(bHasClassPaths)
			{
				FText NewClassToolTip;
				if(SelectedPaths.Num() == 1)
				{
					if(CanCreateClass())
					{
						NewClassToolTip = FText::Format(LOCTEXT("NewClassTooltip_CreateIn", "Create a new class in {0}."), FText::FromString(SelectedPaths[0]));
					}
					else
					{
						NewClassToolTip = FText::Format(LOCTEXT("NewClassTooltip_InvalidPath", "Cannot create new classes in {0}."), FText::FromString(SelectedPaths[0]));
					}
				}
				else
				{
					NewClassToolTip = LOCTEXT("NewClassTooltip_InvalidNumberOfPaths", "Can only create classes when there is a single path selected.");
				}

				// New Class
				MenuBuilder.AddMenuEntry(
					LOCTEXT("NewClassLabel", "New C++ Class..."),
					NewClassToolTip,
					FSlateIcon(FAppStyle::GetStyleSetName(), "MainFrame.AddCodeToProject"),
					FUIAction(
						FExecuteAction::CreateRaw( this, &FPathContextMenu::ExecuteCreateClass ),
						FCanExecuteAction::CreateRaw( this, &FPathContextMenu::CanCreateClass )
						)
					);
			}
#endif
			// Explore
			MenuBuilder.AddMenuEntry(
				ExtContentBrowserUtils::GetExploreFolderText(),
				LOCTEXT("ExploreTooltip", "Finds this folder on disk."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP( this, &FPathContextMenu::ExecuteExplore ) )
				);

#if ECB_FEA_VALIDATE
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ValidateAssetsInFolder", "Validate Assets"),
				LOCTEXT("ValidateAssetsInFolderTooltip", "Validate all assets in current folder."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteValidateAssetsInFolder))
			);
#endif

			// Rescan folder
			if (CanExecuteRescanFolder())
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("RescanFolders", "Rescan Folders"),
					LOCTEXT("RescanFoldersTooltip", "Rescan folders in current selected folder recursively"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteRescanFolder))
				);
			}
#if 0
			// Rescan Assets Recursively
			if (CanExecuteRescanFolder())
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("RescanAssets", "Rescan Assets"),
					LOCTEXT("RescanAssetsTooltip", "Rescan assets in current selected folder recursively"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteCacheAllAssetSAsync))
				);
			}
#endif
			// Import folder
			if (CanExecuteImportFolder())
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("ImportAssets", "Import Assets"),
					LOCTEXT("ImportAssetsTooltip", "Import assets in current selected folder"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteImportFolder))
				);
#if ECB_WIP
				MenuBuilder.AddMenuEntry(
					LOCTEXT("ImportAssetsRecursively", "Import Assets Recursively"),
					LOCTEXT("ImportAssetsRecursivelyTooltip", "Refresh assets in current selected folder and it's children folders recursively"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteImportFolder))
				);
#endif
			}
			
#if ECB_LEGACY
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename, NAME_None,
				LOCTEXT("RenameFolder", "Rename"),
				LOCTEXT("RenameFolderTooltip", "Rename the selected folder.")
				);

			// If any colors have already been set, display color options as a sub menu
			if ( ContentBrowserUtils::HasCustomColors() )
			{
				// Set Color (submenu)
				MenuBuilder.AddSubMenu(
					LOCTEXT("SetColor", "Set Color"),
					LOCTEXT("SetColorTooltip", "Sets the color this folder should appear as."),
					FNewMenuDelegate::CreateRaw( this, &FPathContextMenu::MakeSetColorSubMenu ),
					false,
					FSlateIcon()
					);
			}
			else
			{
				// Set Color
				MenuBuilder.AddMenuEntry(
					LOCTEXT("SetColor", "Set Color"),
					LOCTEXT("SetColorTooltip", "Sets the color this folder should appear as."),
					FSlateIcon(),
					FUIAction( FExecuteAction::CreateSP( this, &FPathContextMenu::ExecutePickColor ) )
					);
			}			

			// If this folder is already favorited, show the option to remove from favorites
			if (ContentBrowserUtils::IsFavoriteFolder(SelectedPaths[0]))
			{
				// Remove from favorites
				MenuBuilder.AddMenuEntry(
					LOCTEXT("RemoveFromFavorites", "Remove From Favorites"),
					LOCTEXT("RemoveFromFavoritesTooltip", "Removes this folder from the favorites section."),
					FSlateIcon(FAppStyle::GetStyleSetName(), "PropertyWindow.Favorites_Disabled"),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteFavorite))
				);
			}
			else
			{
				// Add to favorites
				MenuBuilder.AddMenuEntry(
					LOCTEXT("AddToFavorites", "Add To Favorites"),
					LOCTEXT("AddToFavoritesTooltip", "Adds this folder to the favorites section for easy access."),
					FSlateIcon(FAppStyle::GetStyleSetName(), "PropertyWindow.Favorites_Enabled"),
					FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteFavorite))
				);
			}
#endif
		}
		MenuBuilder.EndSection();

		if(bHasAssetPaths)
		{
			// Bulk operations section //
			MenuBuilder.BeginSection("PathContextBulkOperations", LOCTEXT("AssetTreeBulkMenuHeading", "Bulk Operations") );
			{
#if ECB_LEGACY
				// Save
				MenuBuilder.AddMenuEntry(FContentBrowserCommands::Get().SaveAllCurrentFolder, NAME_None,
					LOCTEXT("ImportFolder", "Import All"),
					LOCTEXT("SaveFolderTooltip", "Saves all modified assets in this folder.")
					);

				// Fix Up Redirectors in Folder
				MenuBuilder.AddMenuEntry(
					LOCTEXT("FixUpRedirectorsInFolder", "Fix Up Redirectors in Folder"),
					LOCTEXT("FixUpRedirectorsInFolderTooltip", "Finds referencers to all redirectors in the selected folders and resaves them if possible, then deletes any redirectors that had all their referencers fixed."),
					FSlateIcon(),
					FUIAction( FExecuteAction::CreateSP( this, &FPathContextMenu::ExecuteFixUpRedirectorsInFolder ) )
					);

				if ( NumAssetPaths == 1 && NumClassPaths == 0 )
				{
					// Migrate Folder
					MenuBuilder.AddMenuEntry(
						LOCTEXT("MigrateFolder", "Migrate..."),
						LOCTEXT("MigrateFolderTooltip", "Copies assets found in this folder and their dependencies to another game content folder."),
						FSlateIcon(),
						FUIAction( FExecuteAction::CreateSP( this, &FPathContextMenu::ExecuteMigrateFolder ) )
						);
				}
#endif
			}
			MenuBuilder.EndSection();
		}
	}

	MenuBuilder.BeginSection("PathViewProjectOptions", LOCTEXT("PathViewProjectOptionsMenuHeading", "Project"));
	{
		if (CanExecuteApplyProjectFolderColors())
		{
			// Apply Project Folder Colors
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ImportProjectColors", "Import Project Colors"),
				LOCTEXT("ImportProjectColorsTooltip", "Import folder colors from selected project in UAssetBrowser to current project.\n (Will override current project's folder color setting)"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteApplyProjectFolderColors),
					FCanExecuteAction::CreateSP(this, &FPathContextMenu::CanExecuteApplyProjectFolderColors)
				)
			);
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("PathViewFolderListOptions", LOCTEXT("PathViewFolderListOptionsMenuHeading", "Folder List"));
	{
		// Save
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Save", "Save"),
			LOCTEXT("SaveFolderListTooltip", "Save root content folder list"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteExportRootFolderList))
		);

		// Load & Merge
		MenuBuilder.AddMenuEntry(
			LOCTEXT("LoadAndMergeFolderList", "Load & Merge"),
			LOCTEXT("LoadAndMergeFolderListTooltip", "Load root content folder list and merge with current folder list"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteLoadAndMergeRootFolderList))
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("LoadAndReplaceFolderList", "Load & Replace"),
			LOCTEXT("LoadAndReplaceFolderListTooltip", "Replace current root content folder with loaded list"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FPathContextMenu::ExecuteLoadAndReplaceRootFolderList))
		);
	}
	MenuBuilder.EndSection();
}

bool FPathContextMenu::CanCreateAsset() const
{
	// We can only create assets when we have a single asset path selected
	return SelectedPaths.Num() == 1 && !ExtContentBrowserUtils::IsClassPath(SelectedPaths[0]);
}

void FPathContextMenu::MakeNewAssetSubMenu(FMenuBuilder& MenuBuilder)
{
#if ECB_LEGACY
	if ( SelectedPaths.Num() )
	{
		FNewAssetOrClassContextMenu::MakeContextMenu(
			MenuBuilder, 
			SelectedPaths, 
			OnNewAssetRequested, 
			FNewAssetOrClassContextMenu::FOnNewClassRequested(), 
			FNewAssetOrClassContextMenu::FOnNewFolderRequested(), 
			OnImportAssetRequested, 
			FNewAssetOrClassContextMenu::FOnGetContentRequested()
			);
	}
#endif
}

void FPathContextMenu::MakeSetColorSubMenu(FMenuBuilder& MenuBuilder)
{
	// New Color
	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewColor", "New Color"),
		LOCTEXT("NewColorTooltip", "Changes the color this folder should appear as."),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateSP( this, &FPathContextMenu::ExecutePickColor ) )
		);

	// Clear Color (only required if any of the selection has one)
	if ( SelectedHasCustomColors() )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearColor", "Clear Color"),
			LOCTEXT("ClearColorTooltip", "Resets the color this folder appears as."),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateSP( this, &FPathContextMenu::ExecuteResetColor ) )
			);
	}

	// Add all the custom colors the user has chosen so far
	TArray< FLinearColor > CustomColors;
	if ( ExtContentBrowserUtils::HasCustomColors( &CustomColors ) )
	{	
		MenuBuilder.BeginSection("PathContextCustomColors", LOCTEXT("CustomColorsExistingColors", "Existing Colors") );
		{
			for ( int32 ColorIndex = 0; ColorIndex < CustomColors.Num(); ColorIndex++ )
			{
				const FLinearColor& Color = CustomColors[ ColorIndex ];
				MenuBuilder.AddWidget(
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0, 0, 0)
						[
							SNew(SButton)
							.ButtonStyle( FAppStyle::Get(), "Menu.Button" )
							.OnClicked( this, &FPathContextMenu::OnColorClicked, Color )
							[
								SNew(SColorBlock)
								.Color( Color )
								.Size( FVector2D(77,16) )
							]
						],
					FText::GetEmpty(),
					/*bNoIndent=*/true
				);
			}
		}
		MenuBuilder.EndSection();
	}
}

void FPathContextMenu::ExecuteMigrateFolder()
{
	const FString& SourcesPath = GetFirstSelectedPath();
	if ( ensure(SourcesPath.Len()) )
	{
		// @todo Make sure the asset registry has completed discovering assets, or else GetAssetsByPath() will not find all the assets in the folder! Add some UI to wait for this with a cancel button
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if ( AssetRegistryModule.Get().IsLoadingAssets() )
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT( "MigrateFolderAssetsNotDiscovered", "You must wait until asset discovery is complete to migrate a folder" ));
			return;
		}

		// Get a list of package names for input into MigratePackages
		TArray<FExtAssetData> AssetDataList;
		TArray<FName> PackageNames;
		ExtContentBrowserUtils::GetAssetsInPaths(SelectedPaths, AssetDataList);
		for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			PackageNames.Add((*AssetIt).PackageName);
		}

		// Load all the assets in the selected paths
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().MigratePackages( PackageNames );
	}
}

void FPathContextMenu::ExecuteExplore()
{
	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		FString FilePath = Path;

		if (!FilePath.IsEmpty())
		{
			// If the folder has not yet been created, make is right before we try to explore to it
#if 0
			if (!IFileManager::Get().DirectoryExists(*FilePath))
			{
				IFileManager::Get().MakeDirectory(*FilePath, /*Tree=*/true);
			}
#endif

			FPlatformProcess::ExploreFolder(*FilePath);
		}
	}
}

void FPathContextMenu::ExecuteApplyProjectFolderColors()
{
	const FString& SourcesPath = GetFirstSelectedPath();
	if (SourcesPath.Len())
	{
		FExtAssetImporter::ImportProjectFolderColors(SourcesPath);
	}
}

void FPathContextMenu::ExecuteValidateAssetsInFolder()
{
	TArray<FExtAssetData*> AssetsInFolder;

	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		TArray<FExtAssetData*> Assets = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetsByFolder(*Path);
		AssetsInFolder.Append(Assets);
	}
	
	if (AssetsInFolder.Num() > 0)
	{
		FString ValidateResult;
		FExtAssetValidator::ValidateDependency(AssetsInFolder, &ValidateResult, /*bProgress*/ true);
		ExtContentBrowserUtils::NotifyMessage(ValidateResult);
	}
}

bool FPathContextMenu::CanExecuteRescanFolder() const
{
	return bHasSelectedPath;
}

bool FPathContextMenu::CanExecuteImportFolder() const
{
	return bHasSelectedPath;
}

bool FPathContextMenu::CanExecuteApplyProjectFolderColors() const
{
	const FString& SourcesPath = GetFirstSelectedPath();
	if (SourcesPath.Len())
	{
// 		FString AssetContentDir;
// 		if (FExtContentBrowserSingleton::GetAssetRegistry().GetAssetContentRootDir(SourcesPath, AssetContentDir))
// 		{
// 			return FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetContentRootConfigDirs().Contains(*AssetContentDir);
// 		}

		TArray<FName> ContentRootHosts;
		FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetContentRootHostDirs().GenerateKeyArray(ContentRootHosts);
		if (ContentRootHosts.Contains(*SourcesPath))
		{
			return true;
		}

		FExtAssetData::EContentType ContentType;
		if (FExtContentBrowserSingleton::GetAssetRegistry().IsRootFolder(SourcesPath)
			&& FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPathInfo(SourcesPath, nullptr, &ContentType)
			&& (ContentType == FExtAssetData::EContentType::Project || ContentType == FExtAssetData::EContentType::Plugin)
			)
		{
			return true;
		}
	}
	return false;
}

void FPathContextMenu::ExecuteRescanFolder()
{
	if (SelectedPaths.Num() > 0)
	{
		FExtContentBrowserSingleton::GetAssetRegistry().ReGatheringFolders(SelectedPaths);
	}
}

void FPathContextMenu::ExecuteImportFolder()
{
	TArray<FExtAssetData*> AssetsInFolder;

	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		TArray<FExtAssetData*> Assets = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetsByFolder(*Path);
		AssetsInFolder.Append(Assets);
	}

	if (AssetsInFolder.Num() > 0)
	{
		TArray<FExtAssetData> AssetsToImport;
		for (FExtAssetData* AssetPtr : AssetsInFolder)
		{
			AssetsToImport.Emplace(*AssetPtr);
		}
		FExtAssetImporter::ImportAssets(AssetsToImport, FUAssetImportSetting::GetSavedImportSetting());
	}
}

bool FPathContextMenu::CanExecuteRootDirsActions() const
{
	return bCanExecuteRootDirsActions;
}

void FPathContextMenu::ExecuteReloadRootFolder()
{
	TArray<FString> Reloaded;
	if (FExtContentBrowserSingleton::GetAssetRegistry().ReloadRootFolders(GetSelectedPaths(), &Reloaded))
	{
		FString ReloadMessage = Reloaded.Num() == 0 ? TEXT("")
			: (Reloaded.Num() == 1
				? FString::Printf(TEXT("%s reloaded. "), *Reloaded[0])
				: FString::Printf(TEXT("%s and other %d reloaded. "), *Reloaded[0], Reloaded.Num()));

		ExtContentBrowserUtils::NotifyMessage(FText::FromString(ReloadMessage), false, 3.f);
	}
}

void FPathContextMenu::ExecuteRemoveRootFolder()
{
	TArray<FString> Removed;
	if (FExtContentBrowserSingleton::GetAssetRegistry().RemoveRootFolders(GetSelectedPaths(), &Removed))
	{
		FString RemovedMessage = Removed.Num() == 0 ? TEXT("")
			: (Removed.Num() == 1
				? FString::Printf(TEXT("%s removed. "), *Removed[0])
				: FString::Printf(TEXT("%s and other %d removed. "), *Removed[0], Removed.Num()));

		ExtContentBrowserUtils::NotifyMessage(FText::FromString(RemovedMessage), false, 3.f);
	}
}

void FPathContextMenu::ExecuteAddRootFolder()
{
	bool bFolderSelected = false;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		void* TopWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow().IsValid() ? FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle() : nullptr;

		FString DefaultPath;
		FString FolderName;
		bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			TopWindowHandle,
			/*Title=*/TEXT("Select root content folder"),
			DefaultPath,
			FolderName
		);

		if (bFolderSelected)
		{
			ECB_LOG(Display, TEXT("Select a folder as root content folder: %s"), *FolderName);

			if (FExtContentBrowserSingleton::GetAssetRegistry().IsRootFolder(FolderName))
			{
				FString AlreadyExist = FString::Printf(TEXT("%s already exist. skip."), *FolderName);
				ExtContentBrowserUtils::NotifyMessage(FText::FromString(AlreadyExist), false, 3.f);
				return;
			}

			TArray<FString> Added;
			TArray<FString> Combined;
			if (FExtContentBrowserSingleton::GetAssetRegistry().AddRootFolder(FolderName, &Added, &Combined))
			{
				FString AddedMessage = Added.Num() == 0 ? TEXT("")
					: ( Added.Num() == 1 
						? FString::Printf(TEXT("%s added. "), *Added[0])
						: FString::Printf(TEXT("%s and other %d added. "), *Added[0], Added.Num()));

				FString CombinedMessage = Combined.Num() == 0 ? TEXT("")
					: (Combined.Num() == 1
						? FString::Printf(TEXT("%s consolidated. "), *Combined[0])
						: FString::Printf(TEXT("%s and other %d consolidated. "), *Combined[0], Combined.Num()));

				AddedMessage = AddedMessage + CombinedMessage;
				ExtContentBrowserUtils::NotifyMessage(FText::FromString(AddedMessage), false, 3.f);
			}
		}
	}
}

void FPathContextMenu::ExecuteExportRootFolderList()
{
	TArray<FString> RootContentPaths;
	FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPaths(RootContentPaths);

	if (RootContentPaths.Num() == 0)
	{
		return;
	}
	
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		const void* TopWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

		TArray<FString> OutFilenames;
		FString DefaultPath;
		FString DefaultFile(TEXT("UAssetBrowserFolderList.txt"));
		const FString FileTypes = TEXT("Text Files (*.txt)|*.txt");
		const bool bSelected = DesktopPlatform->SaveFileDialog(
			TopWindowHandle, 
			TEXT("Save root content folder list"), 
			DefaultPath, 
			DefaultFile, 
			FileTypes, 
			EFileDialogFlags::None, 
			OutFilenames);

		if (bSelected && OutFilenames.Num() > 0)
		{
			FFileHelper::SaveStringArrayToFile(RootContentPaths, *OutFilenames[0]);
		}
	}
}

void FPathContextMenu::LoadRootFolderList(bool bReplaceCurrent)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		const FText Title =
			bReplaceCurrent 
			? LOCTEXT("LoadRootFolderList", "Load root content folder list and replace current folder list")
			: LOCTEXT("LoadAndMergeRootFolderList", "Load and merge root content folder list");
		const FString FileTypes = TEXT("Text Files (*.txt)|*.txt");

		FString DefaultPath;
		FString DefaultFile(TEXT(""));
		TArray<FString> OutFilenames;
		DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			Title.ToString(),
			DefaultPath,
			DefaultFile,
			FileTypes,
			EFileDialogFlags::None,
			OutFilenames
		);

		if (OutFilenames.Num() == 1)
		{
			TArray<FString> FileContentLines;
			FString Message;
			if (!FFileHelper::LoadFileToStringArray(FileContentLines, *OutFilenames[0]))
			{
				Message = FString::Printf(TEXT("Couldn't read folder list file: %s"), *OutFilenames[0]);
				ECB_LOG(Error, TEXT("%s"), *Message);
			}
			else
			{
				if (bReplaceCurrent)
				{
					Message = FString::Printf(TEXT("%s loaded."), *OutFilenames[0]);
					FExtContentBrowserSingleton::GetAssetRegistry().ReplaceRootContentPathsWith(FileContentLines);
				}
				else
				{
					Message = FString::Printf(TEXT("%s loaded and merged."), *OutFilenames[0]);
					FExtContentBrowserSingleton::GetAssetRegistry().MergeRootContentPathsWith(FileContentLines, /*bReplace*/ false);
				}
			}

			ExtContentBrowserUtils::NotifyMessage(Message);
		}
	}
}

void FPathContextMenu::ExecuteLoadAndReplaceRootFolderList()
{
	LoadRootFolderList(/*bReplaceCurrent*/ true);
}

void FPathContextMenu::ExecuteLoadAndMergeRootFolderList()
{
	LoadRootFolderList(/*bReplaceCurrent*/ false);
}

bool FPathContextMenu::CanExecuteRename() const
{
	return false;// ContentBrowserUtils::CanRenameFromPathView(SelectedPaths);
}

void FPathContextMenu::ExecuteRename()
{
	check(SelectedPaths.Num() == 1);
	if (OnRenameFolderRequested.IsBound())
	{
		OnRenameFolderRequested.Execute(SelectedPaths[0]);
	}
}

void FPathContextMenu::ExecuteResetColor()
{
	ResetColors();
}

void FPathContextMenu::ExecutePickColor()
{
	if (SelectedPaths.Num() == 0)
	{
		return;
	}

	// Spawn a color picker, so the user can select which color they want
	FLinearColor InitialColor = ExtContentBrowserUtils::GetDefaultColor();

	if ( SelectedPaths.Num() > 0 )
	{
		// Make sure an color entry exists for all the paths, otherwise they won't update in realtime with the widget color
		for (int32 PathIdx = SelectedPaths.Num() - 1; PathIdx >= 0; --PathIdx)
		{
			const FString& Path = SelectedPaths[PathIdx];
			TSharedPtr<FLinearColor> Color = ExtContentBrowserUtils::LoadColor( Path );
			if ( Color.IsValid() )
			{
				// Default the color to the first valid entry
				InitialColor = *Color.Get();
				break;
			}
		}
	}

	FColorPickerArgs PickerArgs;
	PickerArgs.InitialColor = InitialColor;
	PickerArgs.bIsModal = false;
	PickerArgs.ParentWidget = ParentContent.Pin();
		
	PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateSP(this, &FPathContextMenu::NewColorComplete);

	OpenColorPicker(PickerArgs);
}

void FPathContextMenu::ExecuteFavorite()
{
	OnFolderFavoriteToggled.ExecuteIfBound(SelectedPaths);
}

void FPathContextMenu::NewColorComplete(const TSharedRef<SWindow>& Window)
{
	// Save the colors back in the config (ptr should have already updated by the widget)
	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		const TSharedPtr<FLinearColor> Color = ExtContentBrowserUtils::LoadColor( Path );
		check( Color.IsValid() );
		ExtContentBrowserUtils::SaveColor( Path, Color );		
	}
}

FReply FPathContextMenu::OnColorClicked( const FLinearColor InColor )
{
	// Make sure an color entry exists for all the paths, otherwise it can't save correctly
	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		TSharedPtr<FLinearColor> Color = ExtContentBrowserUtils::LoadColor( Path );
		if ( !Color.IsValid() )
		{
			Color = MakeShareable( new FLinearColor() );
		}
		*Color.Get() = InColor;
		ExtContentBrowserUtils::SaveColor( Path, Color );
	}

	// Dismiss the menu here, as we can't make the 'clear' option appear if a folder has just had a color set for the first time
	FSlateApplication::Get().DismissAllMenus();

	return FReply::Handled();
}

void FPathContextMenu::ResetColors()
{
	// Clear the custom colors for all the selected paths
	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		ExtContentBrowserUtils::SaveColor( Path, NULL );		
	}
}

void FPathContextMenu::ExecuteSaveFolder()
{
	// Get a list of package names in the selected paths
	TArray<FString> PackageNames;
	GetPackageNamesInSelectedPaths(PackageNames);

	// Form a list of packages from the assets
	TArray<UPackage*> Packages;
	for (int32 PackageIdx = 0; PackageIdx < PackageNames.Num(); ++PackageIdx)
	{
		UPackage* Package = FindPackage(NULL, *PackageNames[PackageIdx]);

		// Only save loaded and dirty packages
		if ( Package != NULL && Package->IsDirty() )
		{
			Packages.Add(Package);
		}
	}

	// Save all packages that were found
	if ( Packages.Num() )
	{
		ExtContentBrowserUtils::SavePackages(Packages);
	}
}

void FPathContextMenu::ExecuteResaveFolder()
{
	// Get a list of package names in the selected paths
	TArray<FString> PackageNames;
	GetPackageNamesInSelectedPaths(PackageNames);

	TArray<UPackage*> Packages;
	for (const FString& PackageName : PackageNames)
	{
		UPackage* Package = FindPackage(nullptr, *PackageName);
		if (!Package)
		{
			Package = LoadPackage(nullptr, *PackageName, LOAD_None);
		}

		if (Package)
		{
			Packages.Add(Package);
		}
	}

	if (Packages.Num())
	{
		ExtContentBrowserUtils::SavePackages(Packages);
	}
}

bool FPathContextMenu::CanExecuteDelete() const
{
	return false;// ContentBrowserUtils::CanDeleteFromPathView(SelectedPaths);
}

void FPathContextMenu::ExecuteDelete()
{
	// Don't allow asset deletion during PIE
	if (GIsEditor)
	{
		UEditorEngine* Editor = GEditor;
		FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext();
		if (PIEWorldContext)
		{
			FNotificationInfo Notification(LOCTEXT("CannotDeleteAssetInPIE", "Assets cannot be deleted while in PIE."));
			Notification.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Notification);
			return;
		}
	}

	check(SelectedPaths.Num() > 0);
	if (ParentContent.IsValid())
	{
		FText Prompt;
		if ( SelectedPaths.Num() == 1 )
		{
			Prompt = FText::Format(LOCTEXT("FolderDeleteConfirm_Single", "Delete folder '{0}'?"), FText::FromString(SelectedPaths[0]));
		}
		else
		{
			Prompt = FText::Format(LOCTEXT("FolderDeleteConfirm_Multiple", "Delete {0} folders?"), FText::AsNumber(SelectedPaths.Num()));
		}
		
		// Spawn a confirmation dialog since this is potentially a highly destructive operation
		FOnClicked OnYesClicked = FOnClicked::CreateSP( this, &FPathContextMenu::ExecuteDeleteFolderConfirmed );
 		ExtContentBrowserUtils::DisplayConfirmationPopup(
			Prompt,
			LOCTEXT("FolderDeleteConfirm_Yes", "Delete"),
			LOCTEXT("FolderDeleteConfirm_No", "Cancel"),
			ParentContent.Pin().ToSharedRef(),
			OnYesClicked);
	}
}

void FPathContextMenu::ExecuteFixUpRedirectorsInFolder()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (const auto& Path : SelectedPaths)
	{
		Filter.PackagePaths.Emplace(*Path);
		Filter.ClassPaths.Emplace(FTopLevelAssetPath(TEXT("/Script/CoreUObject"), TEXT("ObjectRedirector")));
	}

	// Query for a list of assets in the selected paths
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		TArray<FString> ObjectPaths;
		for (const auto& Asset : AssetList)
		{
			ObjectPaths.Add(Asset.GetSoftObjectPath().ToString());
		}

		TArray<UObject*> Objects;
		const bool bAllowedToPromptToLoadAssets = true;
		const bool bLoadRedirects = true;
		if (ExtContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, Objects, bAllowedToPromptToLoadAssets, bLoadRedirects))
		{
			// Transform Objects array to ObjectRedirectors array
			TArray<UObjectRedirector*> Redirectors;
			for (auto Object : Objects)
			{
				auto Redirector = CastChecked<UObjectRedirector>(Object);
				Redirectors.Add(Redirector);
			}

			// Load the asset tools module
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			AssetToolsModule.Get().FixupReferencers(Redirectors);
		}
	}
}


FReply FPathContextMenu::ExecuteDeleteFolderConfirmed()
{
	if ( ExtContentBrowserUtils::DeleteFolders(SelectedPaths) )
	{
		ResetColors();
		if (OnFolderDeleted.IsBound())
		{
			OnFolderDeleted.Execute();
		}
	}

	return FReply::Handled();
}

void FPathContextMenu::CacheCanExecuteVars()
{
	bCanExecuteRootDirsActions = FExtContentBrowserSingleton::GetAssetRegistry().IsRootFolders(SelectedPaths);
	bHasSelectedPath = SelectedPaths.Num() > 0;

#if ECB_LEGACY
	// Cache whether we can execute any of the source control commands
	bCanExecuteSCCCheckOut = false;
	bCanExecuteSCCOpenForAdd = false;
	bCanExecuteSCCCheckIn = false;

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( SourceControlProvider.IsEnabled() && SourceControlProvider.IsAvailable() )
	{
		TArray<FString> PackageNames;
		GetPackageNamesInSelectedPaths(PackageNames);

		// Check the SCC state for each package in the selected paths
		for ( auto PackageIt = PackageNames.CreateConstIterator(); PackageIt; ++PackageIt )
		{
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(*PackageIt), EStateCacheUsage::Use);
			if(SourceControlState.IsValid())
			{
				if ( SourceControlState->CanCheckout() )
				{
					bCanExecuteSCCCheckOut = true;
				}
				else if ( !SourceControlState->IsSourceControlled() )
				{
					bCanExecuteSCCOpenForAdd = true;
				}
				else if ( SourceControlState->CanCheckIn() )
				{
					bCanExecuteSCCCheckIn = true;
				}
			}

			if ( bCanExecuteSCCCheckOut && bCanExecuteSCCOpenForAdd && bCanExecuteSCCCheckIn )
			{
				// All SCC options are available, no need to keep iterating
				break;
			}
		}
	}
#endif
}

void FPathContextMenu::GetPackageNamesInSelectedPaths(TArray<FString>& OutPackageNames) const
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		const FString& Path = SelectedPaths[PathIdx];
		new (Filter.PackagePaths) FName(*Path);
	}

	// Query for a list of assets in the selected paths
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	// Form a list of unique package names from the assets
	TSet<FName> UniquePackageNames;
	for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
	{
		UniquePackageNames.Add(AssetList[AssetIdx].PackageName);
	}

	// Add all unique package names to the output
	for ( auto PackageIt = UniquePackageNames.CreateConstIterator(); PackageIt; ++PackageIt )
	{
		OutPackageNames.Add( (*PackageIt).ToString() );
	}
}

FString FPathContextMenu::GetFirstSelectedPath() const
{
	return SelectedPaths.Num() > 0 ? SelectedPaths[0] : TEXT("");
}

bool FPathContextMenu::SelectedHasCustomColors() const
{
	for (int32 PathIdx = 0; PathIdx < SelectedPaths.Num(); ++PathIdx)
	{
		// Ignore any that are the default color
		const FString& Path = SelectedPaths[PathIdx];
		const TSharedPtr<FLinearColor> Color = ExtContentBrowserUtils::LoadColor( Path );
		if ( Color.IsValid() && !Color->Equals( ExtContentBrowserUtils::GetDefaultColor() ) )
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

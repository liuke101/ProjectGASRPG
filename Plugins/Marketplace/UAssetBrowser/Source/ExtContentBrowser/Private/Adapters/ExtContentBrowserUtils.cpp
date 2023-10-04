// Copyright 2017-2021 marynate. All Rights Reserved.


#include "ExtContentBrowserUtils.h"
#include "ExtContentBrowserSingleton.h"
#include "SExtAssetView.h"
#include "SExtPathView.h"

#include "HAL/IConsoleManager.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"

#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Modules/ModuleManager.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Layout/WidgetPath.h"
#include "SlateOptMacros.h"
#include "EditorStyleSet.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "Settings/EditorExperimentalSettings.h"
#include "FileHelpers.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"

#include "Subsystems/AssetEditorSubsystem.h"
#include "PackagesDialog.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "ImageUtils.h"
#include "Logging/MessageLog.h"
#include "Misc/EngineBuildSettings.h"
#include "Interfaces/IPluginManager.h"


#define LOCTEXT_NAMESPACE "ExtContentBrowser"

#define MAX_CLASS_NAME_LENGTH 32 // Enforce a reasonable class name length so the path is not too long for FPlatformMisc::GetMaxPathLength()

namespace ExtContentBrowserUtils
{
	// Keep a map of all the paths that have custom colors, so updating the color in one location updates them all
	static TMap< FString, TSharedPtr< FLinearColor > > PathColors;

	/** Internal function to delete a folder from disk, but only if it is empty. InPathToDelete is in FPackageName format. */
	bool DeleteEmptyFolderFromDisk(const FString& InPathToDelete);
}

class SExtContentBrowserPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SExtContentBrowserPopup ){}

		SLATE_ATTRIBUTE( FText, Message )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			.Padding(10)
			.OnMouseButtonDown(this, &SExtContentBrowserPopup::OnBorderClicked)
			.BorderBackgroundColor(this, &SExtContentBrowserPopup::GetBorderBackgroundColor)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(SImage) .Image( FAppStyle::GetBrush("ContentBrowser.PopupMessageIcon") )
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(InArgs._Message)
					.WrapTextAt(450)
				]
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	static void DisplayMessage( const FText& Message, const FSlateRect& ScreenAnchor, TSharedRef<SWidget> ParentContent )
	{
		TSharedRef<SExtContentBrowserPopup> PopupContent = SNew(SExtContentBrowserPopup) .Message(Message);

		const FVector2D ScreenLocation = FVector2D(ScreenAnchor.Left, ScreenAnchor.Top);
		const bool bFocusImmediately = true;
		const FVector2D SummonLocationSize = ScreenAnchor.GetSize();

		TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
			ParentContent,
			FWidgetPath(),
			PopupContent,
			ScreenLocation,
			FPopupTransitionEffect( FPopupTransitionEffect::TopMenu ),
			bFocusImmediately,
			SummonLocationSize);

		PopupContent->SetMenu(Menu);
	}

private:
	void SetMenu(const TSharedPtr<IMenu>& InMenu)
	{
		Menu = InMenu;
	}

	FReply OnBorderClicked(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
	{
		if (Menu.IsValid())
		{
			Menu.Pin()->Dismiss();
		}

		return FReply::Handled();
	}

	FSlateColor GetBorderBackgroundColor() const
	{
		return IsHovered() ? FLinearColor(0.5, 0.5, 0.5, 1) : FLinearColor::White;
	}

private:
	TWeakPtr<IMenu> Menu;
};

/** A miniture confirmation popup for quick yes/no questions */
class SExtContentBrowserConfirmPopup :  public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SExtContentBrowserConfirmPopup ) {}
			
		/** The text to display */
		SLATE_ARGUMENT(FText, Prompt)

		/** The Yes Button to display */
		SLATE_ARGUMENT(FText, YesText)

		/** The No Button to display */
		SLATE_ARGUMENT(FText, NoText)

		/** Invoked when yes is clicked */
		SLATE_EVENT(FOnClicked, OnYesClicked)

		/** Invoked when no is clicked */
		SLATE_EVENT(FOnClicked, OnNoClicked)

	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs )
	{
		OnYesClicked = InArgs._OnYesClicked;
		OnNoClicked = InArgs._OnNoClicked;

		ChildSlot
		[
			SNew(SBorder)
			. BorderImage(FAppStyle::GetBrush("Menu.Background"))
			. Padding(10)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 5)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
						.Text(InArgs._Prompt)
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(3)
					+ SUniformGridPanel::Slot(0, 0)
					.HAlign(HAlign_Fill)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(InArgs._YesText)
						.OnClicked( this, &SExtContentBrowserConfirmPopup::YesClicked )
					]

					+ SUniformGridPanel::Slot(1, 0)
					.HAlign(HAlign_Fill)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(InArgs._NoText)
						.OnClicked( this, &SExtContentBrowserConfirmPopup::NoClicked )
					]
				]
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/** Opens the popup using the specified component as its parent */
	void OpenPopup(const TSharedRef<SWidget>& ParentContent)
	{
		// Show dialog to confirm the delete
		Menu = FSlateApplication::Get().PushMenu(
			ParentContent,
			FWidgetPath(),
			SharedThis(this),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect( FPopupTransitionEffect::TopMenu )
			);
	}

private:
	/** The yes button was clicked */
	FReply YesClicked()
	{
		if ( OnYesClicked.IsBound() )
		{
			OnYesClicked.Execute();
		}

		if (Menu.IsValid())
		{
			Menu.Pin()->Dismiss();
		}

		return FReply::Handled();
	}

	/** The no button was clicked */
	FReply NoClicked()
	{
		if ( OnNoClicked.IsBound() )
		{
			OnNoClicked.Execute();
		}

		if (Menu.IsValid())
		{
			Menu.Pin()->Dismiss();
		}

		return FReply::Handled();
	}

	/** The IMenu prepresenting this popup */
	TWeakPtr<IMenu> Menu;

	/** Delegates for button clicks */
	FOnClicked OnYesClicked;
	FOnClicked OnNoClicked;
};

bool ExtContentBrowserUtils::OpenEditorForAsset(const FString& ObjectPath)
{
	// Load the asset if unloaded
	TArray<UObject*> LoadedObjects;
	TArray<FString> ObjectPaths;
	ObjectPaths.Add(ObjectPath);
	ExtContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedObjects);

	// Open the editor for the specified asset
	UObject* FoundObject = FindObject<UObject>(NULL, *ObjectPath);
			
	return OpenEditorForAsset(FoundObject);
}

bool ExtContentBrowserUtils::OpenEditorForAsset(UObject* Asset)
{
	if( Asset != NULL )
	{
		// @todo toolkit minor: Needs world-centric support?
		return GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
	}

	return false;
}

bool ExtContentBrowserUtils::OpenEditorForAsset(const TArray<UObject*>& Assets)
{
	if ( Assets.Num() == 1 )
	{
		return OpenEditorForAsset(Assets[0]);
	}
	else if ( Assets.Num() > 1 )
	{
		return GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets);
	}
	
	return false;
}

bool ExtContentBrowserUtils::LoadAssetsIfNeeded(const TArray<FString>& ObjectPaths, TArray<UObject*>& LoadedObjects, bool bAllowedToPromptToLoadAssets, bool bLoadRedirects)
{
	bool bAnyObjectsWereLoadedOrUpdated = false;

	// Build a list of unloaded assets
	TArray<FString> UnloadedObjectPaths;
	bool bAtLeastOneUnloadedMap = false;
	for (int32 PathIdx = 0; PathIdx < ObjectPaths.Num(); ++PathIdx)
	{
		const FString& ObjectPath = ObjectPaths[PathIdx];

		UObject* FoundObject = FindObject<UObject>(NULL, *ObjectPath);
		if ( FoundObject )
		{
			LoadedObjects.Add(FoundObject);
		}
		else
		{
			// Unloaded asset, we will load it later
			UnloadedObjectPaths.Add(ObjectPath);
			if ( FEditorFileUtils::IsMapPackageAsset(ObjectPath) )
			{
				bAtLeastOneUnloadedMap = true;
			}
		}
	}

	// Make sure all selected objects are loaded, where possible
	if ( UnloadedObjectPaths.Num() > 0 )
	{
		// Get the maximum objects to load before displaying the slow task
		const bool bShowProgressDialog = (UnloadedObjectPaths.Num() > GetDefault<UExtContentBrowserSettings>()->NumObjectsToLoadBeforeWarning) || bAtLeastOneUnloadedMap;
		FScopedSlowTask SlowTask(UnloadedObjectPaths.Num(), LOCTEXT("LoadingObjects", "Loading Objects..."));
		if (bShowProgressDialog)
		{
			SlowTask.MakeDialog();
		}

		GIsEditorLoadingPackage = true;

		// We usually don't want to follow redirects when loading objects for the Content Browser.  It would
		// allow a user to interact with a ghost/unverified asset as if it were still alive.
		// This can be overridden by providing bLoadRedirects = true as a parameter.
		const ELoadFlags LoadFlags = bLoadRedirects ? LOAD_None : LOAD_NoRedirects;

		bool bSomeObjectsFailedToLoad = false;
		for (int32 PathIdx = 0; PathIdx < UnloadedObjectPaths.Num(); ++PathIdx)
		{
			const FString& ObjectPath = UnloadedObjectPaths[PathIdx];
			SlowTask.EnterProgressFrame(1, FText::Format(LOCTEXT("LoadingObjectf", "Loading {0}..."), FText::FromString(ObjectPath)));

			// Load up the object
			UObject* LoadedObject = LoadObject<UObject>(NULL, *ObjectPath, NULL, LoadFlags, NULL);
			if ( LoadedObject )
			{
				LoadedObjects.Add(LoadedObject);
			}
			else
			{
				bSomeObjectsFailedToLoad = true;
			}

			if (GWarn->ReceivedUserCancel())
			{
				// If the user has canceled stop loading the remaining objects. We don't add the remaining objects to the failed string,
				// this would only result in launching another dialog when by their actions the user clearly knows not all of the 
				// assets will have been loaded.
				break;
			}
		}
		GIsEditorLoadingPackage = false;

		if ( bSomeObjectsFailedToLoad )
		{
			FNotificationInfo Info(LOCTEXT("LoadObjectFailed", "Failed to load assets"));
			Info.ExpireDuration = 5.0f;
			Info.Hyperlink = FSimpleDelegate::CreateStatic([](){ FMessageLog("LoadErrors").Open(EMessageSeverity::Info, true); });
			Info.HyperlinkText = LOCTEXT("LoadObjectHyperlink", "Show Message Log");

			FSlateNotificationManager::Get().AddNotification(Info);
			return false;
		}
	}

	return true;
}

void ExtContentBrowserUtils::GetUnloadedAssets(const TArray<FString>& ObjectPaths, TArray<FString>& OutUnloadedObjects)
{
	OutUnloadedObjects.Empty();

	// Build a list of unloaded assets and check if there are any parent folders
	for (int32 PathIdx = 0; PathIdx < ObjectPaths.Num(); ++PathIdx)
	{
		const FString& ObjectPath = ObjectPaths[PathIdx];

		UObject* FoundObject = FindObject<UObject>(NULL, *ObjectPath);
		if ( !FoundObject )
		{
			// Unloaded asset, we will load it later
			OutUnloadedObjects.Add(ObjectPath);
		}
	}
}

bool ExtContentBrowserUtils::PromptToLoadAssets(const TArray<FString>& UnloadedObjects)
{
	bool bShouldLoadAssets = false;

	// Prompt the user to load assets
	const FText Question = FText::Format( LOCTEXT("ConfirmLoadAssets", "You are about to load {0} assets. Would you like to proceed?"), FText::AsNumber( UnloadedObjects.Num() ) );
	if ( EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, Question) )
	{
		bShouldLoadAssets = true;
	}

	return bShouldLoadAssets;
}

bool ExtContentBrowserUtils::CanRenameFolder(const FString& InFolderPath)
{
	// Cannot rename folders that are part of a classes or collections root
	return !ExtContentBrowserUtils::IsClassPath(InFolderPath) && !ExtContentBrowserUtils::IsCollectionPath(InFolderPath);
}

bool ExtContentBrowserUtils::CanRenameAsset(const FAssetData& InAssetData)
{
	// Cannot rename redirectors or classes or cooked packages
	return !InAssetData.IsRedirector() && InAssetData.AssetClassPath != FTopLevelAssetPath(TEXT("/Script/CoreUObject"), NAME_Class) && !(InAssetData.PackageFlags & PKG_FilterEditorOnly);
}

void ExtContentBrowserUtils::RenameAsset(UObject* Asset, const FString& NewName, FText& ErrorMessage)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FAssetRenameData> AssetsAndNames;
	const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	new(AssetsAndNames) FAssetRenameData(Asset, PackagePath, NewName);
	AssetToolsModule.Get().RenameAssetsWithDialog(AssetsAndNames);
}

void ExtContentBrowserUtils::CopyAssets(const TArray<UObject*>& Assets, const FString& DestPath)
{
#if ECB_LEGACY
	TArray<UObject*> NewObjects;
	ObjectTools::DuplicateObjects(Assets, TEXT(""), DestPath, /*bOpenDialog=*/false, &NewObjects);

	// If any objects were duplicated, report the success
	if ( NewObjects.Num() )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("Number"), NewObjects.Num() );
		const FText Message = FText::Format( LOCTEXT("AssetsDroppedCopy", "{Number} asset(s) copied"), Args );
		FSlateNotificationManager::Get().AddNotification(FNotificationInfo(Message));

		// Now branch the files in source control if possible
		check(Assets.Num() == NewObjects.Num());
		for(int32 ObjectIndex = 0; ObjectIndex < Assets.Num(); ObjectIndex++)
		{
			UObject* SourceAsset = Assets[ObjectIndex];
			UObject* DestAsset = NewObjects[ObjectIndex];
			SourceControlHelpers::BranchPackage(DestAsset->GetOutermost(), SourceAsset->GetOutermost());
		}
	}
#endif
}

void ExtContentBrowserUtils::MoveAssets(const TArray<UObject*>& Assets, const FString& DestPath, const FString& SourcePath)
{
	check(DestPath.Len() > 0);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FAssetRenameData> AssetsAndNames;
	for ( auto AssetIt = Assets.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		UObject* Asset = *AssetIt;

		if ( !ensure(Asset) )
		{
			continue;
		}

		FString PackagePath;
		FString ObjectName = Asset->GetName();

		if ( SourcePath.Len() )
		{
			const FString CurrentPackageName = Asset->GetOutermost()->GetName();

			// This is a relative operation
			if ( !ensure(CurrentPackageName.StartsWith(SourcePath)) )
			{
				continue;
			}
				
			// Collect the relative path then use it to determine the new location
			// For example, if SourcePath = /Game/MyPath and CurrentPackageName = /Game/MyPath/MySubPath/MyAsset
			//     /Game/MyPath/MySubPath/MyAsset -> /MySubPath

			const int32 ShortPackageNameLen = FPackageName::GetLongPackageAssetName(CurrentPackageName).Len();
			const int32 RelativePathLen = CurrentPackageName.Len() - ShortPackageNameLen - SourcePath.Len() - 1; // -1 to exclude the trailing "/"
			const FString RelativeDestPath = CurrentPackageName.Mid(SourcePath.Len(), RelativePathLen);

			PackagePath = DestPath + RelativeDestPath;
		}
		else
		{
			// Only a DestPath was supplied, use it
			PackagePath = DestPath;
		}

		new(AssetsAndNames) FAssetRenameData(Asset, PackagePath, ObjectName);
	}

	if ( AssetsAndNames.Num() > 0 )
	{
		AssetToolsModule.Get().RenameAssetsWithDialog(AssetsAndNames);
	}
}

int32 ExtContentBrowserUtils::DeleteAssets(const TArray<UObject*>& AssetsToDelete)
{
	return ObjectTools::DeleteObjects(AssetsToDelete);
}

bool ExtContentBrowserUtils::DeleteFolders(const TArray<FString>& PathsToDelete)
{
	// Get a list of assets in the paths to delete
	TArray<FExtAssetData> AssetDataList;
	GetAssetsInPaths(PathsToDelete, AssetDataList);

	const int32 NumAssetsInPaths = AssetDataList.Num();
	bool bAllowFolderDelete = false;
	if ( NumAssetsInPaths == 0 )
	{
		// There were no assets, allow the folder delete.
		bAllowFolderDelete = true;
	}
	else
	{
		// Load all the assets in the folder and attempt to delete them.
		// If it was successful, allow the folder delete.

		// Get a list of object paths for input into LoadAssetsIfNeeded
		TArray<FString> ObjectPaths;
		for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ObjectPaths.Add((*AssetIt).ObjectPath.ToString());
		}

		// Load all the assets in the selected paths
		TArray<UObject*> LoadedAssets;
		if ( ExtContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets) )
		{
			// Make sure we loaded all of them
			if ( LoadedAssets.Num() == NumAssetsInPaths )
			{
				TArray<UObject*> ToDelete = LoadedAssets;
				ObjectTools::AddExtraObjectsToDelete(ToDelete);
				const int32 NumAssetsDeleted = ExtContentBrowserUtils::DeleteAssets(ToDelete);
				if ( NumAssetsDeleted == ToDelete.Num() )
				{
					// Successfully deleted all assets in the specified path. Allow the folder to be removed.
					bAllowFolderDelete = true;
				}
				else
				{
					// Not all the assets in the selected paths were deleted
				}
			}
			else
			{
				// Not all the assets in the selected paths were loaded
			}
		}
		else
		{
			// The user declined to load some assets or some assets failed to load
		}
	}
	
	if ( bAllowFolderDelete )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		for (const FString& PathToDelete : PathsToDelete)
		{
			if (DeleteEmptyFolderFromDisk(PathToDelete))
			{
				AssetRegistryModule.Get().RemovePath(PathToDelete);
			}
		}

		return true;
	}

	return false;
}

bool ExtContentBrowserUtils::DeleteEmptyFolderFromDisk(const FString& InPathToDelete)
{
	struct FEmptyFolderVisitor : public IPlatformFile::FDirectoryVisitor
	{
		bool bIsEmpty;

		FEmptyFolderVisitor()
			: bIsEmpty(true)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				bIsEmpty = false;
				return false; // abort searching
			}

			return true; // continue searching
		}
	};

	FString PathToDeleteOnDisk;
	if (FPackageName::TryConvertLongPackageNameToFilename(InPathToDelete, PathToDeleteOnDisk))
	{
		// Look for files on disk in case the folder contains things not tracked by the asset registry
		FEmptyFolderVisitor EmptyFolderVisitor;
		IFileManager::Get().IterateDirectoryRecursively(*PathToDeleteOnDisk, EmptyFolderVisitor);

		if (EmptyFolderVisitor.bIsEmpty)
		{
			return IFileManager::Get().DeleteDirectory(*PathToDeleteOnDisk, false, true);
		}
	}

	return false;
}

void ExtContentBrowserUtils::GetAssetsInPaths(const TArray<FString>& InPaths, TArray<FExtAssetData>& OutAssetDataList)
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (int32 PathIdx = 0; PathIdx < InPaths.Num(); ++PathIdx)
	{
		new (Filter.PackagePaths) FName(*InPaths[PathIdx]);
	}

	// Query for a list of assets in the selected paths
	FExtContentBrowserSingleton::GetAssetRegistry().GetAssets(Filter, OutAssetDataList);
}

bool ExtContentBrowserUtils::SavePackages(const TArray<UPackage*>& Packages)
{
	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(Packages, bCheckDirty, bPromptToSave);

	return Return == FEditorFileUtils::EPromptReturnCode::PR_Success;
}

bool ExtContentBrowserUtils::SaveDirtyPackages()
{
	const bool bPromptUserToSave = true;
	const bool bSaveMapPackages = true;
	const bool bSaveContentPackages = true;
	const bool bFastSave = false;
	const bool bNotifyNoPackagesSaved = false;
	const bool bCanBeDeclined = false;
	return FEditorFileUtils::SaveDirtyPackages( bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, bCanBeDeclined );
}

TArray<UPackage*> ExtContentBrowserUtils::LoadPackages(const TArray<FString>& PackageNames)
{
	TArray<UPackage*> LoadedPackages;

	GWarn->BeginSlowTask( LOCTEXT("LoadingPackages", "Loading Packages..."), true );

	for (int32 PackageIdx = 0; PackageIdx < PackageNames.Num(); ++PackageIdx)
	{
		const FString& PackageName = PackageNames[PackageIdx];

		if ( !ensure(PackageName.Len() > 0) )
		{
			// Empty package name. Skip it.
			continue;
		}

		UPackage* Package = FindPackage(NULL, *PackageName);

		if ( Package != NULL )
		{
			// The package is at least partially loaded. Fully load it.
			Package->FullyLoad();
		}
		else
		{
			// The package is unloaded. Try to load the package from disk.
			Package = UPackageTools::LoadPackage(PackageName);
		}

		// If the package was loaded, add it to the loaded packages list.
		if ( Package != NULL )
		{
			LoadedPackages.Add(Package);
		}
	}

	GWarn->EndSlowTask();

	return LoadedPackages;
}

void ExtContentBrowserUtils::NotifyMessage(const FText& Message, bool bAlsoPrintToConsole, float InDuration /*= 1.0f*/)
{
	if (!Message.IsEmpty())
	{
		FNotificationInfo NotificationInfo(Message);
		NotificationInfo.ExpireDuration = InDuration;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		if (bAlsoPrintToConsole)
		{
			ECB_INFO(Display, TEXT("%s"), *Message.ToString());
		}
	}
}

void ExtContentBrowserUtils::NotifyMessage(const FString& Message, float InDuration /*= 3.f*/)
{
	NotifyMessage(FText::FromString(Message), false, InDuration);
}

void ExtContentBrowserUtils::DisplayMessage(const FText& Message, const FSlateRect& ScreenAnchor, const TSharedRef<SWidget>& ParentContent)
{
	SExtContentBrowserPopup::DisplayMessage(Message, ScreenAnchor, ParentContent);
}

void ExtContentBrowserUtils::DisplayMessagePopup(const FText& Message)
{
	FMessageDialog::Open(EAppMsgType::Ok, Message);
}

void ExtContentBrowserUtils::DisplayConfirmationPopup(const FText& Message, const FText& YesString, const FText& NoString, const TSharedRef<SWidget>& ParentContent, const FOnClicked& OnYesClicked, const FOnClicked& OnNoClicked)
{
	TSharedRef<SExtContentBrowserConfirmPopup> Popup = 
		SNew(SExtContentBrowserConfirmPopup)
		.Prompt(Message)
		.YesText(YesString)
		.NoText(NoString)
		.OnYesClicked( OnYesClicked )
		.OnNoClicked( OnNoClicked );

	Popup->OpenPopup(ParentContent);
}

#if ECB_LEGACY

bool ExtContentBrowserUtils::RenameFolder(const FString& DestPath, const FString& SourcePath)
{
	if (DestPath == SourcePath)
	{
		return false;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// move any assets in our folder
	TArray<FAssetData> AssetsInFolder;
	AssetRegistryModule.Get().GetAssetsByPath(*SourcePath, AssetsInFolder, true);
	TArray<UObject*> ObjectsInFolder;
	GetObjectsInAssetData(AssetsInFolder, ObjectsInFolder);
	MoveAssets(ObjectsInFolder, DestPath, SourcePath);

	// Now check to see if the original folder is empty, if so we can delete it
	TArray<FAssetData> AssetsInOriginalFolder;
	AssetRegistryModule.Get().GetAssetsByPath(*SourcePath, AssetsInOriginalFolder, true);
	if (AssetsInOriginalFolder.Num() == 0)
	{
		TArray<FString> FoldersToDelete;
		FoldersToDelete.Add(SourcePath);
		DeleteFolders(FoldersToDelete);
	}

	// set color of folder to new path
	const TSharedPtr<FLinearColor> FolderColor = LoadColor(SourcePath);
	if (FolderColor.IsValid())
	{
		SaveColor(SourcePath, nullptr);
		SaveColor(DestPath, FolderColor);
	}

	return true;
}

#endif
bool ExtContentBrowserUtils::CopyFolders(const TArray<FString>& InSourcePathNames, const FString& DestPath)
{
	TMap<FString, TArray<UObject*> > SourcePathToLoadedAssets;

	// Make sure the destination path is not in the source path list
	TArray<FString> SourcePathNames = InSourcePathNames;
	SourcePathNames.Remove(DestPath);

	// Load all assets in the source paths
	if (!PrepareFoldersForDragDrop(SourcePathNames, SourcePathToLoadedAssets))
	{
		return false;
	}

	// Load the Asset Registry to update paths during the copy
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	// For every path which contained valid assets...
	for ( auto PathIt = SourcePathToLoadedAssets.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Put dragged folders in a sub-folder under the destination path
		const FString SourcePath = PathIt.Key();
		FString SubFolderName = FPackageName::GetLongPackageAssetName(SourcePath);
		FString Destination = DestPath + TEXT("/") + SubFolderName;

		// Add the new path to notify sources views
		{
// 			TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();
// 			EmptyFolderVisibilityManager->SetAlwaysShowPath(Destination);
		}
		AssetRegistryModule.Get().AddPath(Destination);

		// If any assets were in this path...
		if ( PathIt.Value().Num() > 0 )
		{
			// Copy assets and supply a source path to indicate it is relative
			ObjectTools::DuplicateObjects( PathIt.Value(), SourcePath, Destination, /*bOpenDialog=*/false );
		}

		const TSharedPtr<FLinearColor> FolderColor = LoadColor(SourcePath);
		if (FolderColor.IsValid())
		{
			SaveColor(Destination, FolderColor);
		}
	}

	return true;
}

#if ECB_LEGACY
bool ExtContentBrowserUtils::MoveFolders(const TArray<FString>& InSourcePathNames, const FString& DestPath)
{
	TMap<FString, TArray<UObject*> > SourcePathToLoadedAssets;
	FString DestPathWithTrailingSlash = DestPath / "";

	// Do not allow parent directories to be moved to themselves or children.
	TArray<FString> SourcePathNames = InSourcePathNames;
	TArray<FString> SourcePathNamesToRemove;
	for (auto SourcePathIt = SourcePathNames.CreateConstIterator(); SourcePathIt; ++SourcePathIt)
	{
		if(DestPathWithTrailingSlash.StartsWith(*SourcePathIt / ""))
		{
			SourcePathNamesToRemove.Add(*SourcePathIt);
		}
	}
	for (auto SourcePathToRemoveIt = SourcePathNamesToRemove.CreateConstIterator(); SourcePathToRemoveIt; ++SourcePathToRemoveIt)
	{
		SourcePathNames.Remove(*SourcePathToRemoveIt);
	}

	// Load all assets in the source paths
	if (!PrepareFoldersForDragDrop(SourcePathNames, SourcePathToLoadedAssets))
	{
		return false;
	}
	
	// Load the Asset Registry to update paths during the move
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// For every path which contained valid assets...
	for ( auto PathIt = SourcePathToLoadedAssets.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Put dragged folders in a sub-folder under the destination path
		const FString SourcePath = PathIt.Key();
		const FString SubFolderName = FPackageName::GetLongPackageAssetName(SourcePath);
		const FString Destination = DestPathWithTrailingSlash + SubFolderName;

		// Add the new path to notify sources views
		{
// 			TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();
// 			EmptyFolderVisibilityManager->SetAlwaysShowPath(Destination);
		}
		AssetRegistryModule.Get().AddPath(Destination);

		// If any assets were in this path...
		if ( PathIt.Value().Num() > 0 )
		{
			// Move assets and supply a source path to indicate it is relative
			MoveAssets( PathIt.Value(), Destination, PathIt.Key() );
		}

		// Attempt to remove the old path
		if (DeleteEmptyFolderFromDisk(SourcePath))
		{
			AssetRegistryModule.Get().RemovePath(SourcePath);
		}

		const TSharedPtr<FLinearColor> FolderColor = LoadColor(SourcePath);
		if (FolderColor.IsValid())
		{
			SaveColor(SourcePath, nullptr);
			SaveColor(Destination, FolderColor);
		}
	}

	return true;
}

#endif

bool ExtContentBrowserUtils::PrepareFoldersForDragDrop(const TArray<FString>& SourcePathNames, TMap< FString, TArray<UObject*> >& OutSourcePathToLoadedAssets)
{
	TSet<UObject*> AllFoundObjects;

	// Load the Asset Registry to update paths during the move
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	
	// Check up-front how many assets we might load in this operation & warn the user
	TArray<FString> ObjectPathsToWarnAbout;
	for ( auto PathIt = SourcePathNames.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Get all assets in this path
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByPath(FName(**PathIt), AssetDataList, true);

		for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ObjectPathsToWarnAbout.Add((*AssetIt).GetSoftObjectPath().ToString());
		}
	}

	GWarn->BeginSlowTask(LOCTEXT("FolderDragDrop_Loading", "Loading folders"), true);

	// For every source path, load every package in the path (if necessary) and keep track of the assets that were loaded
	for ( auto PathIt = SourcePathNames.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Get all assets in this path
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByPath(FName(**PathIt), AssetDataList, true);

		// Form a list of all object paths for these assets
		TArray<FString> ObjectPaths;
		for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ObjectPaths.Add((*AssetIt).GetSoftObjectPath().ToString());
		}

		// Load all assets in this path if needed
		TArray<UObject*> AllLoadedAssets;
		LoadAssetsIfNeeded(ObjectPaths, AllLoadedAssets, false);

		// Add a slash to the end of the path so StartsWith doesn't get a false positive on similarly named folders
		const FString SourcePathWithSlash = *PathIt + TEXT("/");

		// Find all files in this path and subpaths
		TArray<FString> Filenames;
		FString RootFolder = FPackageName::LongPackageNameToFilename(SourcePathWithSlash);
		FPackageName::FindPackagesInDirectory(Filenames, RootFolder);

		// Now find all assets in memory that were loaded from this path that are valid for drag-droppping
		TArray<UObject*> ValidLoadedAssets;
		for ( auto AssetIt = AllLoadedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			UObject* Asset = *AssetIt;
			if ( (Asset->GetClass() != UObjectRedirector::StaticClass() &&				// Skip object redirectors
				 !AllFoundObjects.Contains(Asset)										// Skip assets we have already found to avoid processing them twice
				) )
			{
				ValidLoadedAssets.Add(Asset);
				AllFoundObjects.Add(Asset);
			}
		}

		// Add an entry of the map of source paths to assets found, whether any assets were found or not
		OutSourcePathToLoadedAssets.Add(*PathIt, ValidLoadedAssets);
	}

	GWarn->EndSlowTask();

	ensure(SourcePathNames.Num() == OutSourcePathToLoadedAssets.Num());
	return true;
}

void ExtContentBrowserUtils::CopyAssetReferencesToClipboard(const TArray<FAssetData>& AssetsToCopy)
{
	FString ClipboardText;
	for ( auto AssetIt = AssetsToCopy.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		if ( ClipboardText.Len() > 0 )
		{
			ClipboardText += LINE_TERMINATOR;
		}

		ClipboardText += (*AssetIt).GetExportTextName();
	}

	FPlatformApplicationMisc::ClipboardCopy( *ClipboardText );
}

void ExtContentBrowserUtils::CopyFilePathsToClipboard(const TArray<FExtAssetData>& AssetsToCopy)
{
	FString ClipboardText;
	for (const FExtAssetData& Asset : AssetsToCopy)
	{
		if (ClipboardText.Len() > 0)
		{
			ClipboardText += LINE_TERMINATOR;
		}
#if 0
		FString PackageFileName;
		FString PackageFile;
		if (FPackageName::TryConvertLongPackageNameToFilename(Asset.PackageName.ToString(), PackageFileName) &&
			FPackageName::FindPackageFileWithoutExtension(PackageFileName, PackageFile))
		{
			ClipboardText += FPaths::ConvertRelativePathToFull(PackageFile);
		}
		else
		{
			// Add a message for when a user tries to copy the path to a file that doesn't exist on disk of the form
			// <AssetName>: No file on disk
			ClipboardText += Asset.AssetName.ToString() + FString(": No file on disk");
		}
#endif
		if (Asset.IsValid())
		{
			ClipboardText += Asset.PackageFilePath.ToString();
		}
	}

	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
}

void ExtContentBrowserUtils::CaptureThumbnailFromViewport(FViewport* InViewport, const TArray<FAssetData>& InAssetsToAssign)
{
	//capture the thumbnail
	uint32 SrcWidth = InViewport->GetSizeXY().X;
	uint32 SrcHeight = InViewport->GetSizeXY().Y;
	// Read the contents of the viewport into an array.
	TArray<FColor> OrigBitmap;
	if (InViewport->ReadPixels(OrigBitmap))
	{
		check(OrigBitmap.Num() == SrcWidth * SrcHeight);

		//pin to smallest value
		int32 CropSize = FMath::Min<uint32>(SrcWidth, SrcHeight);
		//pin to max size
		int32 ScaledSize  = FMath::Min<uint32>(ThumbnailTools::DefaultThumbnailSize, CropSize);

		//calculations for cropping
		TArray<FColor> CroppedBitmap;
		CroppedBitmap.AddUninitialized(CropSize*CropSize);
		//Crop the image
		int32 CroppedSrcTop  = (SrcHeight - CropSize)/2;
		int32 CroppedSrcLeft = (SrcWidth - CropSize)/2;
		for (int32 Row = 0; Row < CropSize; ++Row)
		{
			//Row*Side of a row*byte per color
			int32 SrcPixelIndex = (CroppedSrcTop+Row)*SrcWidth + CroppedSrcLeft;
			const void* SrcPtr = &(OrigBitmap[SrcPixelIndex]);
			void* DstPtr = &(CroppedBitmap[Row*CropSize]);
			FMemory::Memcpy(DstPtr, SrcPtr, CropSize*4);
		}

		//Scale image down if needed
		TArray<FColor> ScaledBitmap;
		if (ScaledSize < CropSize)
		{
			FImageUtils::ImageResize( CropSize, CropSize, CroppedBitmap, ScaledSize, ScaledSize, ScaledBitmap, true );
		}
		else
		{
			//just copy the data over. sizes are the same
			ScaledBitmap = CroppedBitmap;
		}

		//setup actual thumbnail
		FObjectThumbnail TempThumbnail;
		TempThumbnail.SetImageSize( ScaledSize, ScaledSize );
		TArray<uint8>& ThumbnailByteArray = TempThumbnail.AccessImageData();

		// Copy scaled image into destination thumb
		int32 MemorySize = ScaledSize*ScaledSize*sizeof(FColor);
		ThumbnailByteArray.AddUninitialized(MemorySize);
		FMemory::Memcpy(&(ThumbnailByteArray[0]), &(ScaledBitmap[0]), MemorySize);

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		//check if each asset should receive the new thumb nail
		for ( auto AssetIt = InAssetsToAssign.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			const FAssetData& CurrentAsset = *AssetIt;

			//assign the thumbnail and dirty
			const FString ObjectFullName = CurrentAsset.GetFullName();
			const FString PackageName    = CurrentAsset.PackageName.ToString();

			UPackage* AssetPackage = FindObject<UPackage>( NULL, *PackageName );
			if ( ensure(AssetPackage) )
			{
				FObjectThumbnail* NewThumbnail = ThumbnailTools::CacheThumbnail(ObjectFullName, &TempThumbnail, AssetPackage);
				if ( ensure(NewThumbnail) )
				{
					//we need to indicate that the package needs to be resaved
					AssetPackage->MarkPackageDirty();

					// Let the content browser know that we've changed the thumbnail
					NewThumbnail->MarkAsDirty();
						
					// Signal that the asset was changed if it is loaded so thumbnail pools will update
					if ( CurrentAsset.IsAssetLoaded() )
					{
						CurrentAsset.GetAsset()->PostEditChange();
					}

					//Set that thumbnail as a valid custom thumbnail so it'll be saved out
					NewThumbnail->SetCreatedAfterCustomThumbsEnabled();
				}
			}
		}
	}
}

void ExtContentBrowserUtils::ClearCustomThumbnails(const TArray<FAssetData>& InAssetsToAssign)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	//check if each asset should receive the new thumb nail
	for ( auto AssetIt = InAssetsToAssign.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		const FAssetData& CurrentAsset = *AssetIt;

		// check whether this is a type that uses one of the shared static thumbnails
		if ( AssetToolsModule.Get().AssetUsesGenericThumbnail( CurrentAsset ) )
		{
			//assign the thumbnail and dirty
			const FString ObjectFullName = CurrentAsset.GetFullName();
			const FString PackageName    = CurrentAsset.PackageName.ToString();

			UPackage* AssetPackage = FindObject<UPackage>( NULL, *PackageName );
			if ( ensure(AssetPackage) )
			{
				ThumbnailTools::CacheEmptyThumbnail( ObjectFullName, AssetPackage);

				//we need to indicate that the package needs to be resaved
				AssetPackage->MarkPackageDirty();

				// Signal that the asset was changed if it is loaded so thumbnail pools will update
				if ( CurrentAsset.IsAssetLoaded() )
				{
					CurrentAsset.GetAsset()->PostEditChange();
				}
			}
		}
	}
}

bool ExtContentBrowserUtils::AssetHasCustomThumbnail( const FAssetData& AssetData )
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if ( AssetToolsModule.Get().AssetUsesGenericThumbnail(AssetData) )
	{
		return ThumbnailTools::AssetHasCustomThumbnail(AssetData.GetFullName());
	}

	return false;
}

ExtContentBrowserUtils::ECBFolderCategory ExtContentBrowserUtils::GetFolderCategory( const FString& InPath )
{
	static const FString ClassesPrefix = TEXT("/Classes_");
	static const FString GameClassesPrefix = TEXT("/Classes_Game");
	static const FString EngineClassesPrefix = TEXT("/Classes_Engine");

	const bool bIsClassDir = InPath.StartsWith(ClassesPrefix);
	if(bIsClassDir)
	{
		const bool bIsGameClassDir = InPath.StartsWith(GameClassesPrefix);
		if(bIsGameClassDir)
		{
			return ECBFolderCategory::GameClasses;
		}

		const bool bIsEngineClassDir = InPath.StartsWith(EngineClassesPrefix);
		if(bIsEngineClassDir)
		{
			return ECBFolderCategory::EngineClasses;
		}

		return ECBFolderCategory::PluginClasses;
	}
	else
	{
		if (IsEngineFolder(InPath))
		{
			return ECBFolderCategory::EngineContent;
		}

		if (IsDevelopersFolder(InPath))
		{
			return ECBFolderCategory::DeveloperContent;
		}

		EPluginLoadedFrom PluginSource;
		if (IsPluginFolder(InPath, &PluginSource))
		{
			if (PluginSource == EPluginLoadedFrom::Project)
			{
				return ECBFolderCategory::PluginContent;
			}
			else
			{
				checkSlow(PluginSource == EPluginLoadedFrom::Engine);
				return ECBFolderCategory::EngineContent;
			}
		}

		return ECBFolderCategory::GameContent;
	}
}

ExtContentBrowserUtils::ECBFolderCategory ExtContentBrowserUtils::GetRootFolderCategory(const FString& InPath)
{
	FText LocalizedFolderName = FText::GetEmpty();
	FExtAssetData::EContentType ContentType;
	if (FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPathInfo(InPath, &LocalizedFolderName, &ContentType))
	{
		if (ContentType == FExtAssetData::EContentType::Plugin)
		{
			return ECBFolderCategory::PluginContent;
		}
		if (ContentType == FExtAssetData::EContentType::Project)
		{
			return ECBFolderCategory::ProjectContent;
		}
		if (ContentType == FExtAssetData::EContentType::VaultCache)
		{
			return ECBFolderCategory::VaultCacheContent;
		}
	}

	return ECBFolderCategory::OrphanContent;
}

bool ExtContentBrowserUtils::IsEngineFolder( const FString& InPath )
{
	static const FString EnginePathWithSlash = TEXT("/Engine");
	static const FString EnginePathWithoutSlash = TEXT("Engine");

	return InPath.StartsWith(EnginePathWithSlash) || InPath == EnginePathWithoutSlash;
}

bool ExtContentBrowserUtils::IsDevelopersFolder( const FString& InPath )
{
	static const FString DeveloperPathWithSlash = FPackageName::FilenameToLongPackageName(FPaths::GameDevelopersDir());
	static const FString DeveloperPathWithoutSlash = DeveloperPathWithSlash.LeftChop(1);
		
	return InPath.StartsWith(DeveloperPathWithSlash) || InPath == DeveloperPathWithoutSlash;
}

static bool PathStartsWithPluginAssetPath(const FString& Path, const FString& PluginName)
{
	// accepted path examples for a plugin named "Plugin":
	// "/Plugin"
	// "/Plugin/"
	// "/Plugin/More/Stuff"
	const int32 PluginNameLength = PluginName.Len();
	const int32 PathLength = Path.Len();
	if (PathLength <= PluginNameLength)
	{
		return false;
	}
	else
	{
		const TCHAR* PathCh = *Path;
		return PathCh[0] == '/' && (PathCh[PluginNameLength + 1] == '/' || PathCh[PluginNameLength + 1] == 0) && FCString::Strnicmp(PathCh + 1, *PluginName, PluginNameLength) == 0;
	}
}

bool ExtContentBrowserUtils::IsPluginFolder(const FString& InPath, const TArray<TSharedRef<IPlugin>>& InPlugins, EPluginLoadedFrom* OutPluginSource)
{
	for (const TSharedRef<IPlugin>& PluginRef : InPlugins)
	{
		const IPlugin& Plugin = *PluginRef;
		const FString& PluginName = Plugin.GetName();
		if (PathStartsWithPluginAssetPath(InPath, PluginName) || InPath == PluginName)
		{
			if (OutPluginSource != nullptr)
			{
				*OutPluginSource = Plugin.GetLoadedFrom();
			}
			return true;
		}
	}
	return false;
}

bool ExtContentBrowserUtils::IsPluginFolder(const FString& InPath, EPluginLoadedFrom* OutPluginSource)
{
	return IsPluginFolder(InPath, IPluginManager::Get().GetEnabledPluginsWithContent(), OutPluginSource);
}

bool ExtContentBrowserUtils::IsClassesFolder(const FString& InPath)
{
	// Strip off any leading or trailing forward slashes
	// We just want the name without any path separators
	FString CleanFolderPath = InPath;
	while ( CleanFolderPath.StartsWith(TEXT("/")) )
	{
		CleanFolderPath = CleanFolderPath.Mid(1);
	}
	while ( CleanFolderPath.EndsWith(TEXT("/")) )
	{
		CleanFolderPath = CleanFolderPath.Mid(0, CleanFolderPath.Len() - 1);
	}

	static const FString ClassesPrefix = TEXT("Classes_");
	const bool bIsClassDir = InPath.StartsWith(ClassesPrefix);

	return bIsClassDir;
}

bool ExtContentBrowserUtils::IsLocalizationFolder( const FString& InPath )
{
	return FPackageName::IsLocalizedPackage(InPath);
}

void ExtContentBrowserUtils::GetObjectsInAssetData(const TArray<FAssetData>& AssetList, TArray<UObject*>& OutDroppedObjects)
{	
	for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
	{
		const FAssetData& AssetData = AssetList[AssetIdx];

		UObject* Obj = AssetData.GetAsset();
		if (Obj)
		{
			OutDroppedObjects.Add(Obj);
		}
	}
}

bool ExtContentBrowserUtils::IsValidFolderName(const FString& FolderName, FText& Reason)
{
	// Check length of the folder name
	if ( FolderName.Len() == 0 )
	{
		Reason = LOCTEXT( "InvalidFolderName_IsTooShort", "Please provide a name for this folder." );
		return false;
	}

	if ( FolderName.Len() > FPlatformMisc::GetMaxPathLength() )
	{
		Reason = FText::Format( LOCTEXT("InvalidFolderName_TooLongForCooking", "Filename '{0}' is too long; this may interfere with cooking for consoles. Unreal filenames should be no longer than {1} characters." ),
			FText::FromString(FolderName), FText::AsNumber(FPlatformMisc::GetMaxPathLength()) );
		return false;
	}

	const FString InvalidChars = INVALID_LONGPACKAGE_CHARACTERS TEXT("/[]"); // Slash is an invalid character for a folder name

	// See if the name contains invalid characters.
	FString Char;
	for( int32 CharIdx = 0; CharIdx < FolderName.Len(); ++CharIdx )
	{
		Char = FolderName.Mid(CharIdx, 1);

		if ( InvalidChars.Contains(*Char) )
		{
			FString ReadableInvalidChars = InvalidChars;
			ReadableInvalidChars.ReplaceInline(TEXT("\r"), TEXT(""));
			ReadableInvalidChars.ReplaceInline(TEXT("\n"), TEXT(""));
			ReadableInvalidChars.ReplaceInline(TEXT("\t"), TEXT(""));

			Reason = FText::Format(LOCTEXT("InvalidFolderName_InvalidCharacters", "A folder name may not contain any of the following characters: {0}"), FText::FromString(ReadableInvalidChars));
			return false;
		}
	}
	
	return FFileHelper::IsFilenameValidForSaving( FolderName, Reason );
}

bool ExtContentBrowserUtils::DoesFolderExist(const FString& FolderPath)
{
	// todo: jdale - CLASS - Will need updating to handle class folders

	TArray<FString> SubPaths;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetSubPaths(FPaths::GetPath(FolderPath), SubPaths, false);

	for(auto SubPathIt(SubPaths.CreateConstIterator()); SubPathIt; SubPathIt++)	
	{
		if ( *SubPathIt == FolderPath )
		{
			return true;
		}
	}

	return false;
}

bool ExtContentBrowserUtils::IsEmptyFolder(const FString& FolderPath, const bool bRecursive)
{
	if (ExtContentBrowserUtils::IsClassPath(FolderPath))
	{
// 		TSharedRef<FNativeClassHierarchy> NativeClassHierarchy = FContentBrowserSingleton::Get().GetNativeClassHierarchy();
// 		return !NativeClassHierarchy->HasClasses(*FolderPath, bRecursive);
	}
	else
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		return !AssetRegistryModule.Get().HasAssets(*FolderPath, bRecursive);
	}

	return false;
}

bool ExtContentBrowserUtils::IsRootDir(const FString& FolderPath)
{
	return IsAssetRootDir(FolderPath) || IsClassRootDir(FolderPath);
}

bool ExtContentBrowserUtils::IsAssetRootDir(const FString& FolderPath)
{
	// All root asset folders start with "/" (not "/Classes_") and contain only a single / (at the beginning)
	int32 LastSlashIndex = INDEX_NONE;
	return FolderPath.Len() > 1 && !IsClassPath(FolderPath) && FolderPath.FindLastChar(TEXT('/'), LastSlashIndex) && LastSlashIndex == 0;
}

bool ExtContentBrowserUtils::IsClassRootDir(const FString& FolderPath)
{
	// All root class folders start with "/Classes_" and contain only a single / (at the beginning)
	int32 LastSlashIndex = INDEX_NONE;
	return IsClassPath(FolderPath) && FolderPath.FindLastChar(TEXT('/'), LastSlashIndex) && LastSlashIndex == 0;
}

FText ExtContentBrowserUtils::GetRootDirDisplayName(const FString& FolderPath)
{
#if ECB_DISABLE
	// Strip off any trailing forward slashes
	FString CleanFolderPath = FolderPath;
	while (CleanFolderPath.EndsWith(TEXT("/")))
	{
		CleanFolderPath = CleanFolderPath.Mid(0, CleanFolderPath.Len() - 1);
	}

	static const FString VaultCacheContentFolderName = TEXT("/data/Content");
	static const FString VaultCacheDataFolderName = TEXT("/data");
	static const FString ProjectOrPluginContentFolderName = TEXT("/Content");

	if (CleanFolderPath.EndsWith(VaultCacheContentFolderName))
	{
		FString VaultCacheFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(CleanFolderPath, TEXT("../..")));
		if (FExtContentDirFinder::FindWithFolder(VaultCacheFolder, TEXT("manifest"), /*bExtension*/ false))
		{
			CleanFolderPath = VaultCacheFolder;// CleanFolderPath.Left(CleanFolderPath.Len() - VaultCacheContentFolderName.Len());
		}
	}
	else if (CleanFolderPath.EndsWith(VaultCacheDataFolderName))
	{
		FString VaultCacheFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(CleanFolderPath, TEXT("..")));
		if (FExtContentDirFinder::FindWithFolder(VaultCacheFolder, TEXT("manifest"), /*bExtension*/ false))
		{
			CleanFolderPath = VaultCacheFolder;// CleanFolderPath.Left(CleanFolderPath.Len() - VaultCacheDataFolderName.Len());
		}
	}
	else if (CleanFolderPath.EndsWith(ProjectOrPluginContentFolderName))
	{
		FString PluginFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(CleanFolderPath, TEXT("..")));
		if (FExtContentDirFinder::FindWithFolder(PluginFolder, TEXT(".uplugin"), /*bExtension*/ true))
		{
			CleanFolderPath = PluginFolder;// CleanFolderPath.Left(CleanFolderPath.Len() - VaultCacheDataFolderName.Len());
		}
	}
	else if (CleanFolderPath.EndsWith(ProjectOrPluginContentFolderName))
	{
		FString ProjectFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(CleanFolderPath, TEXT("..")));
		if (FExtContentDirFinder::FindWithFolder(ProjectFolder, TEXT(".uproject"), /*bExtension*/ true))
		{
			CleanFolderPath = ProjectFolder;// CleanFolderPath.Left(CleanFolderPath.Len() - VaultCacheDataFolderName.Len());
		}
	}

	FString DisplayFolderName = FPaths::GetPathLeaf(CleanFolderPath);
	FText LocalizedFolderName = DisplayFolderName.IsEmpty() ? FText::FromString(CleanFolderPath) : FText::FromString(DisplayFolderName);
#endif

	FText LocalizedFolderName = FText::GetEmpty();
	FExtAssetData::EContentType ContentType;
	if (!FExtContentBrowserSingleton::GetAssetRegistry().QueryRootContentPathInfo(FolderPath, &LocalizedFolderName, &ContentType))
	{
		// fallback
		FString CleanFolderPath = FolderPath;
		FPaths::NormalizeDirectoryName(CleanFolderPath);
		FString DisplayFolderName = FPaths::GetPathLeaf(CleanFolderPath);
		LocalizedFolderName = DisplayFolderName.IsEmpty() ? FText::FromString(CleanFolderPath) : FText::FromString(DisplayFolderName);
	}

#if ECB_LEGACY
	const bool bIsBackgroundGathering = FExtContentBrowserSingleton::GetAssetRegistry().IsFolderBackgroundGathering(FolderPath);
	if (bIsBackgroundGathering)
	{
		return FText::Format(LOCTEXT("RootContentFolderFmt", "{0} (Loading...)"), LocalizedFolderName);
	}
	else
#endif
	{
		return FText::Format(LOCTEXT("RootContentFolderFmt", "{0}"), LocalizedFolderName);
	}

}

bool ExtContentBrowserUtils::IsFolderBackgroundGathering(const FString& InFolder)
{
	const bool bIsBackgroundGathering = FExtContentBrowserSingleton::GetAssetRegistry().IsFolderBackgroundGathering(InFolder);
	return bIsBackgroundGathering;
}

FName ExtContentBrowserUtils::GetCurrentGatheringFolder()
{
	return FExtContentBrowserSingleton::GetAssetRegistry().GetCurrentGatheringFolder();
}

bool ExtContentBrowserUtils::IsClassPath(const FString& InPath)
{
	static const FString ClassesRootPrefix = TEXT("/Classes_");
	return InPath.StartsWith(ClassesRootPrefix);
}

bool ExtContentBrowserUtils::IsCollectionPath(const FString& InPath, FName* OutCollectionName, ECollectionShareType::Type* OutCollectionShareType)
{
	static const FString CollectionsRootPrefix = TEXT("/Collections");
	if (InPath.StartsWith(CollectionsRootPrefix))
	{
		TArray<FString> PathParts;
		InPath.ParseIntoArray(PathParts, TEXT("/"));
		check(PathParts.Num() > 2);

		// The second part of the path is the share type name
		if (OutCollectionShareType)
		{
			*OutCollectionShareType = ECollectionShareType::FromString(*PathParts[1]);
		}

		// The third part of the path is the collection name
		if (OutCollectionName)
		{
			*OutCollectionName = FName(*PathParts[2]);
		}

		return true;
	}
	return false;
}

void ExtContentBrowserUtils::CountPathTypes(const TArray<FString>& InPaths, int32& OutNumAssetPaths, int32& OutNumClassPaths)
{
	OutNumAssetPaths = 0;
	OutNumClassPaths = 0;

	for(const FString& Path : InPaths)
	{
		if(IsClassPath(Path))
		{
			++OutNumClassPaths;
		}
		else
		{
			++OutNumAssetPaths;
		}
	}
}

void ExtContentBrowserUtils::CountPathTypes(const TArray<FName>& InPaths, int32& OutNumAssetPaths, int32& OutNumClassPaths)
{
	OutNumAssetPaths = 0;
	OutNumClassPaths = 0;

	for(const FName& Path : InPaths)
	{
		if(IsClassPath(Path.ToString()))
		{
			++OutNumClassPaths;
		}
		else
		{
			++OutNumAssetPaths;
		}
	}
}

void ExtContentBrowserUtils::CountItemTypes(const TArray<FAssetData>& InItems, int32& OutNumAssetItems, int32& OutNumClassItems)
{
	OutNumAssetItems = 0;
	OutNumClassItems = 0;

	for(const FAssetData& Item : InItems)
	{
		if(Item.AssetClassPath == FTopLevelAssetPath(TEXT("/Script/CoreUObject"), NAME_Class))
		{
			++OutNumClassItems;
		}
		else
		{
			++OutNumAssetItems;
		}
	}
}

bool ExtContentBrowserUtils::IsValidPathToCreateNewClass(const FString& InPath)
{
	// Classes can currently only be added to game modules - if this is restricted, we can use IsClassPath here instead
	// Classes can only be created in modules, so that will be at least two folders deep (two /)
	static const FString GameClassesRootPrefix = TEXT("/Classes_Game");

	int32 LastSlashIndex = INDEX_NONE;
	return InPath.StartsWith(GameClassesRootPrefix) && InPath.FindLastChar(TEXT('/'), LastSlashIndex) && LastSlashIndex != 0;
}

bool ExtContentBrowserUtils::IsValidPathToCreateNewFolder(const FString& InPath)
{
	// We can't currently make folders in class paths
	// If we do later allow folders in class paths, they must only be created within modules (see IsValidPathToCreateNewClass above)
	return !IsClassPath(InPath);
}

const TSharedPtr<FLinearColor> ExtContentBrowserUtils::LoadColor(const FString& FolderPath, bool bNoCache)
{
	auto LoadColorInternal = [](const FString& InPath, bool bInNoCache) -> TSharedPtr<FLinearColor>
	{
		// See if we have a value cached first
		if (!bInNoCache)
		{
			TSharedPtr<FLinearColor> CachedColor = PathColors.FindRef(InPath);
			if (CachedColor.IsValid())
			{
				return CachedColor;
			}
		}
		
		// Loads the color of folder at the given path from the config
		if(FPaths::FileExists(GEditorPerProjectIni))
		{
			// Create a new entry from the config, skip if it's default
			FString ColorStr;
			if(GConfig->GetString(TEXT("PathColor"), *InPath, ColorStr, GEditorPerProjectIni))
			{
				FLinearColor Color;
				if(Color.InitFromString(ColorStr) && !Color.Equals(ExtContentBrowserUtils::GetDefaultColor()))
				{
					return PathColors.Add(InPath, MakeShareable(new FLinearColor(Color)));
				}
			}
			else if (!bInNoCache)
			{
				return PathColors.Add(InPath, MakeShareable(new FLinearColor(ExtContentBrowserUtils::GetDefaultColor())));
			}
		}
		return nullptr;
	};

	// First try and find the color using the given path, as this works correctly for both assets and classes
	TSharedPtr<FLinearColor> FoundColor = LoadColorInternal(FolderPath, bNoCache);
	if(FoundColor.IsValid())
	{
		return FoundColor;
	}

	// If that failed, try and use the filename (assets used to use this as their color key, but it doesn't work with classes)
	if(!IsClassPath(FolderPath))
	{
		const FString RelativePath = FPackageName::LongPackageNameToFilename(FolderPath + TEXT("/"));
		return LoadColorInternal(RelativePath, bNoCache);
	}

	return nullptr;
}

bool ExtContentBrowserUtils::LoadFolderColor(const FString& FolderPath, FLinearColor& OutFolderColor)
{
	return FExtContentBrowserSingleton::GetAssetRegistry().GetFolderColor(FolderPath, OutFolderColor);
}

void ExtContentBrowserUtils::SaveColor(const FString& FolderPath, const TSharedPtr<FLinearColor>& FolderColor, bool bForceAdd)
{
	auto SaveColorInternal = [](const FString& InPath, const TSharedPtr<FLinearColor>& InFolderColor)
	{
		// Saves the color of the folder to the config
		if(FPaths::FileExists(GEditorPerProjectIni))
		{
			GConfig->SetString(TEXT("PathColor"), *InPath, *InFolderColor->ToString(), GEditorPerProjectIni);
		}

		// Update the map too
		PathColors.Add(InPath, InFolderColor);
	};

	auto RemoveColorInternal = [](const FString& InPath)
	{
		// Remove the color of the folder from the config
		if(FPaths::FileExists(GEditorPerProjectIni))
		{
			GConfig->RemoveKey(TEXT("PathColor"), *InPath, GEditorPerProjectIni);
		}

		// Update the map too
		PathColors.Remove(InPath);
	};

	// Remove the color if it's invalid or default
	const bool bRemove = !FolderColor.IsValid() || (!bForceAdd && FolderColor->Equals(ExtContentBrowserUtils::GetDefaultColor()));

	if(bRemove)
	{
		RemoveColorInternal(FolderPath);
	}
	else
	{
		SaveColorInternal(FolderPath, FolderColor);
	}

	// Make sure and remove any colors using the legacy path format
	if(!IsClassPath(FolderPath))
	{
		const FString RelativePath = FPackageName::LongPackageNameToFilename(FolderPath + TEXT("/"));
		return RemoveColorInternal(RelativePath);
	}
}

bool ExtContentBrowserUtils::HasCustomColors( TArray< FLinearColor >* OutColors )
{
	// Check to see how many paths are currently using this color
	// Note: we have to use the config, as paths which haven't been rendered yet aren't registered in the map
	bool bHasCustom = false;
	if (FPaths::FileExists(GEditorPerProjectIni))
	{
		// Read individual entries from a config file.
		TArray< FString > Section; 
		GConfig->GetSection( TEXT("PathColor"), Section, GEditorPerProjectIni );

		for( int32 SectionIndex = 0; SectionIndex < Section.Num(); SectionIndex++ )
		{
			FString EntryStr = Section[ SectionIndex ];
			EntryStr.TrimStartInline();

			FString PathStr;
			FString ColorStr;
			if ( EntryStr.Split( TEXT( "=" ), &PathStr, &ColorStr ) )
			{
				// Ignore any that have invalid or default colors
				FLinearColor CurrentColor;
				if( CurrentColor.InitFromString( ColorStr ) && !CurrentColor.Equals( ExtContentBrowserUtils::GetDefaultColor() ) )
				{
					bHasCustom = true;
					if ( OutColors )
					{
						// Only add if not already present (ignores near matches too)
						bool bAdded = false;
						for( int32 ColorIndex = 0; ColorIndex < OutColors->Num(); ColorIndex++ )
						{
							const FLinearColor& Color = (*OutColors)[ ColorIndex ];
							if( CurrentColor.Equals( Color ) )
							{
								bAdded = true;
								break;
							}
						}
						if ( !bAdded )
						{
							OutColors->Add( CurrentColor );
						}
					}
					else
					{
						break;
					}
				}
			}
		}
	}
	return bHasCustom;
}

FLinearColor ExtContentBrowserUtils::GetDefaultColor()
{
	// The default tint the folder should appear as
	return FLinearColor::Gray;
}

FText ExtContentBrowserUtils::GetExploreFolderText()
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("FileManagerName"), FPlatformMisc::GetFileManagerName());
	return FText::Format(NSLOCTEXT("GenericPlatform", "ShowInFileManager", "Show in {FileManagerName}"), Args);
}

#if ECB_DISABLE
static const auto CVarMaxFullPathLength = 
	IConsoleManager::Get().RegisterConsoleVariable( TEXT("MaxAssetFullPath"), FPlatformMisc::GetMaxPathLength(), TEXT("Maximum full path name of an asset.") )->AsVariableInt();

bool ExtContentBrowserUtils::IsValidObjectPathForCreate(const FString& ObjectPath, FText& OutErrorMessage, bool bAllowExistingAsset)
{
	const FString ObjectName = FPackageName::ObjectPathToObjectName(ObjectPath);

	// Make sure the name is not already a class or otherwise invalid for saving
	if ( !FFileHelper::IsFilenameValidForSaving(ObjectName, OutErrorMessage) )
	{
		// Return false to indicate that the user should enter a new name
		return false;
	}

	// Make sure the new name only contains valid characters
	if ( !FName::IsValidXName( ObjectName, INVALID_OBJECTNAME_CHARACTERS INVALID_LONGPACKAGE_CHARACTERS, &OutErrorMessage ) )
	{
		// Return false to indicate that the user should enter a new name
		return false;
	}

	// Make sure we are not creating an FName that is too large
	if ( ObjectPath.Len() > NAME_SIZE )
	{
		// This asset already exists at this location, inform the user and continue
		OutErrorMessage = LOCTEXT("AssetNameTooLong", "This asset name is too long. Please choose a shorter name.");
		// Return false to indicate that the user should enter a new name
		return false;
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);

	if (!IsValidPackageForCooking(PackageName, OutErrorMessage))
	{
		return false;
	}

	// Make sure we are not creating an path that is too long for the OS
	const FString RelativePathFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());	// full relative path with name + extension
	const FString FullPath = FPaths::ConvertRelativePathToFull(RelativePathFilename);	// path to file on disk
	if ( ObjectPath.Len() > (FPlatformMisc::GetMaxPathLength() - MAX_CLASS_NAME_LENGTH) || FullPath.Len() > CVarMaxFullPathLength->GetValueOnGameThread() )
	{
		// The full path for the asset is too long
		OutErrorMessage = FText::Format( LOCTEXT("AssetPathTooLong", 
			"The full path for the asset is too deep, the maximum is '{0}'. \nPlease choose a shorter name for the asset or create it in a shallower folder structure."), 
			FText::AsNumber(FPlatformMisc::GetMaxPathLength()) );
		// Return false to indicate that the user should enter a new name
		return false;
	}

	// Check for an existing asset, unless it we were asked not to.
	if ( !bAllowExistingAsset )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData ExistingAsset = AssetRegistryModule.Get().GetAssetByObjectPath(FName(*ObjectPath));
		if (ExistingAsset.IsValid())
		{
			// This asset already exists at this location, inform the user and continue
			OutErrorMessage = FText::Format( LOCTEXT("RenameAssetAlreadyExists", "An asset already exists at this location with the name '{0}'."), FText::FromString( ObjectName ) );

			// Return false to indicate that the user should enter a new name
			return false;
		}
	}

	return true;
}
#endif

bool ExtContentBrowserUtils::IsValidFolderPathForCreate(const FString& InFolderPath, const FString& NewFolderName, FText& OutErrorMessage)
{
	if (!ExtContentBrowserUtils::IsValidFolderName(NewFolderName, OutErrorMessage))
	{
		return false;
	}

	const FString NewFolderPath = InFolderPath / NewFolderName;

	if (ExtContentBrowserUtils::DoesFolderExist(NewFolderPath))
	{
		OutErrorMessage = LOCTEXT("RenameFolderAlreadyExists", "A folder already exists at this location with this name.");
		return false;
	}

	// Make sure we are not creating a folder path that is too long
	if (NewFolderPath.Len() > FPlatformMisc::GetMaxPathLength() - MAX_CLASS_NAME_LENGTH)
	{
		// The full path for the folder is too long
		OutErrorMessage = FText::Format(LOCTEXT("RenameFolderPathTooLong",
			"The full path for the folder is too deep, the maximum is '{0}'. Please choose a shorter name for the folder or create it in a shallower folder structure."),
			FText::AsNumber(FPlatformMisc::GetMaxPathLength()));
		// Return false to indicate that the user should enter a new name for the folder
		return false;
	}

	const bool bDisplayL10N = GetDefault<UExtContentBrowserSettings>()->GetDisplayL10NFolder();
	if (!bDisplayL10N && ExtContentBrowserUtils::IsLocalizationFolder(NewFolderPath))
	{
		OutErrorMessage = LOCTEXT("LocalizationFolderReserved", "The L10N folder is reserved for localized content and is currently hidden.");
		return false;
	}

	return true;
}

int32 ExtContentBrowserUtils::GetPackageLengthForCooking(const FString& PackageName, bool IsInternalBuild)
{
	// We assume the game name is 20 characters (the maximum allowed) to make sure that content can be ported between projects
	static const int32 MaxGameNameLen = 20;

	// Pad out the game name to the maximum allowed
	const FString GameName = FApp::GetProjectName();
	FString GameNamePadded = GameName;
	while (GameNamePadded.Len() < MaxGameNameLen)
	{
		GameNamePadded += TEXT(" ");
	}

	// We use "WindowsNoEditor" below as it's the longest platform name, so will also prove that any shorter platform names will validate correctly
	const FString AbsoluteRootPath = FPaths::ConvertRelativePathToFull(FPaths::RootDir());
	const FString AbsoluteGamePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString AbsoluteEnginePath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	const FString AbsoluteEngineCookPath = AbsoluteGamePath / TEXT("Saved") / TEXT("Cooked") / TEXT("WindowsNoEditor") / TEXT("Engine");
	const FString AbsoluteGameCookPath = AbsoluteGamePath / TEXT("Saved") / TEXT("Cooked") / TEXT("WindowsNoEditor") / GameName;

	EPluginLoadedFrom PluginLoadedFrom;
	const bool bIsPluginAsset = ExtContentBrowserUtils::IsPluginFolder(PackageName, &PluginLoadedFrom);
	const bool bIsEngineAsset = ExtContentBrowserUtils::IsEngineFolder(PackageName) || (bIsPluginAsset && PluginLoadedFrom == EPluginLoadedFrom::Engine);
	const bool bIsProjectAsset = !bIsEngineAsset;

	int32 AbsoluteCookPathToAssetLength = 0;

	FString RelativePathToAsset;

	const FString AbsolutePath = bIsEngineAsset ? AbsoluteEnginePath : AbsoluteGamePath;

	const FString& AbsoluteCookPath = bIsEngineAsset ? AbsoluteEngineCookPath : AbsoluteGameCookPath;

	if(FPackageName::TryConvertLongPackageNameToFilename(PackageName, RelativePathToAsset, FPackageName::GetAssetPackageExtension()))
	{
		const FString AbsolutePathToAsset = FPaths::ConvertRelativePathToFull(RelativePathToAsset);

		FString AssetPathWithinCookDir = AbsolutePathToAsset;
		FPaths::RemoveDuplicateSlashes(AssetPathWithinCookDir);
		AssetPathWithinCookDir.RemoveFromStart(AbsolutePath, ESearchCase::CaseSensitive);


		if (IsInternalBuild)
		{
			// We assume a constant size for the build machine base path, so strip either the root or game path from the start
			// (depending on whether the project is part of the main UE4 source tree or located elsewhere)
			FString CookDirWithoutBasePath = AbsoluteCookPath;
			if (CookDirWithoutBasePath.StartsWith(AbsoluteRootPath, ESearchCase::CaseSensitive))
			{
				CookDirWithoutBasePath.RemoveFromStart(AbsoluteRootPath, ESearchCase::CaseSensitive);
			}
			else
			{
				CookDirWithoutBasePath.RemoveFromStart(AbsoluteCookPath, ESearchCase::CaseSensitive);
			}
			
			FString AbsoluteBuildMachineCookPathToAsset = FString(TEXT("D:/BuildFarm/buildmachine_++depot+UE4-Releases+4.10")) / CookDirWithoutBasePath / AssetPathWithinCookDir;

			// only add game name padding if it is not an engine asset, otherwise it is considered portable already
			if(!bIsEngineAsset)
			{
				AbsoluteBuildMachineCookPathToAsset.ReplaceInline(*GameName, *GameNamePadded, ESearchCase::CaseSensitive);
			}

			AbsoluteCookPathToAssetLength = AbsoluteBuildMachineCookPathToAsset.Len();
		}
		else
		{
			// Test that the package can be cooked based on the current project path
			FString AbsoluteCookPathToAsset = AbsoluteCookPath / AssetPathWithinCookDir;

			// only add game name padding if it is not an engine asset, otherwise it is considered portable already
			if (!bIsEngineAsset)
			{
				AbsoluteCookPathToAsset.ReplaceInline(*GameName, *GameNamePadded, ESearchCase::CaseSensitive);
			}

			AbsoluteCookPathToAssetLength = AbsoluteCookPathToAsset.Len();
		}
	}
	else
	{
//		UE_LOG(LogContentBrowser, Error, TEXT("Package Name '%' is not a valid path and cannot be converted to a filename"), *PackageName);
	}
	return AbsoluteCookPathToAssetLength;
}

bool ExtContentBrowserUtils::IsValidPackageForCooking(const FString& PackageName, FText& OutErrorMessage)
{
	int32 AbsoluteCookPathToAssetLength = GetPackageLengthForCooking(PackageName, FEngineBuildSettings::IsInternalBuild());

	int32 MaxCookPathLen = GetMaxCookPathLen();
	if (AbsoluteCookPathToAssetLength > MaxCookPathLen)
	{
		// See TTP# 332328:
		// The following checks are done mostly to prevent / alleviate the problems that "long" paths are causing with the BuildFarm and cooked builds.
		// The BuildFarm uses a verbose path to encode extra information to provide more information when things fail, however this makes the path limitation a problem.
		//	- We assume a base path of D:/BuildFarm/buildmachine_++depot+UE4-Releases+4.10/
		//	- We assume the game name is 20 characters (the maximum allowed) to make sure that content can be ported between projects
		//	- We calculate the cooked game path relative to the game root (eg, Showcases/Infiltrator/Saved/Cooked/WindowsNoEditor/Infiltrator)
		//	- We calculate the asset path relative to (and including) the Content directory (eg, Content/Environment/Infil1/Infil1_Underground/Infrastructure/Model/SM_Infil1_Tunnel_Ceiling_Pipes_1xEntryCurveOuter_Double.uasset)
		if (FEngineBuildSettings::IsInternalBuild())
		{
			// The projected length of the path for cooking is too long
			OutErrorMessage = FText::Format(LOCTEXT("AssetCookingPathTooLongForBuildMachine", "The path to the asset is too long '{0}' for cooking by the build machines, the maximum is '{1}'\nPlease choose a shorter name for the asset or create it in a shallower folder structure with shorter folder names."), FText::AsNumber(AbsoluteCookPathToAssetLength), FText::AsNumber(MaxCookPathLen));
		}
		else
		{
			// The projected length of the path for cooking is too long
			OutErrorMessage = FText::Format(LOCTEXT("AssetCookingPathTooLong", "The path to the asset is too long '{0}', the maximum for cooking is '{1}'\nPlease choose a shorter name for the asset or create it in a shallower folder structure with shorter folder names."), FText::AsNumber(AbsoluteCookPathToAssetLength), FText::AsNumber(MaxCookPathLen));
		}

		// Return false to indicate that the user should enter a new name
		return false;
	}

	return true;
}

/** Given an set of packages that will be synced by a SCC operation, report any dependencies that are out-of-date and aren't in the list of packages to be synced */
void GetOutOfDatePackageDependencies(const TArray<FString>& InPackagesThatWillBeSynced, TArray<FString>& OutDependenciesThatAreOutOfDate)
{
#if ECB_LEGACY
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Build up the initial list of known packages
	// We add to these as we find new dependencies to process
	TSet<FName> AllPackages;
	TArray<FName> AllPackagesArray;
	{
		AllPackages.Reserve(InPackagesThatWillBeSynced.Num());
		AllPackagesArray.Reserve(InPackagesThatWillBeSynced.Num());

		for (const FString& PackageName : InPackagesThatWillBeSynced)
		{
			const FName PackageFName = *PackageName;
			AllPackages.Emplace(PackageFName);
			AllPackagesArray.Emplace(PackageFName);
		}
	}

	// Build up the complete set of package dependencies
	TArray<FString> AllDependencies;
	{
		for (int32 PackageIndex = 0; PackageIndex < AllPackagesArray.Num(); ++PackageIndex)
		{
			const FName PackageName = AllPackagesArray[PackageIndex];

			TArray<FName> PackageDependencies;
			AssetRegistryModule.GetDependencies(PackageName, PackageDependencies, EAssetRegistryDependencyType::Packages);

			for (const FName PackageDependency : PackageDependencies)
			{
				if (!AllPackages.Contains(PackageDependency))
				{
					AllPackages.Emplace(PackageDependency);
					AllPackagesArray.Emplace(PackageDependency);

					FString PackageDependencyStr = PackageDependency.ToString();
					if (!FPackageName::IsScriptPackage(PackageDependencyStr) && FPackageName::IsValidLongPackageName(PackageDependencyStr))
					{
						AllDependencies.Emplace(MoveTemp(PackageDependencyStr));
					}
				}
			}
		}
	}

	// Query SCC to see which dependencies are out-of-date
	if (AllDependencies.Num() > 0)
	{
		ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
		
		TArray<FString> DependencyFilenames = SourceControlHelpers::PackageFilenames(AllDependencies);
		for (int32 DependencyIndex = 0; DependencyIndex < AllDependencies.Num(); ++DependencyIndex)
		{
			// Dependency data may contain files that no longer exist on disk; strip those from the list now
			if (!FPaths::FileExists(DependencyFilenames[DependencyIndex]))
			{
				AllDependencies.RemoveAt(DependencyIndex, 1, false);
				DependencyFilenames.RemoveAt(DependencyIndex, 1, false);
				--DependencyIndex;
			}
		}

		SCCProvider.Execute(ISourceControlOperation::Create<FUpdateStatus>(), DependencyFilenames);
		for (int32 DependencyIndex = 0; DependencyIndex < AllDependencies.Num(); ++DependencyIndex)
		{
			const FString& DependencyName = AllDependencies[DependencyIndex];
			const FString& DependencyFilename = DependencyFilenames[DependencyIndex];

			FSourceControlStatePtr SCCState = SCCProvider.GetState(DependencyFilename, EStateCacheUsage::Use);
			if (SCCState.IsValid() && !SCCState->IsCurrent())
			{
				OutDependenciesThatAreOutOfDate.Emplace(DependencyName);
			}
		}
	}
#endif
}

void ShowSyncDependenciesDialog(const TArray<FString>& InDependencies, TArray<FString>& OutExtraPackagesToSync)
{
#if ECB_LEGACY
	if (InDependencies.Num() > 0)
	{
		FPackagesDialogModule& PackagesDialogModule = FModuleManager::LoadModuleChecked<FPackagesDialogModule>(TEXT("PackagesDialog"));

		PackagesDialogModule.CreatePackagesDialog(
			LOCTEXT("SyncAssetDependenciesTitle", "Sync Asset Dependencies"), 
			LOCTEXT("SyncAssetDependenciesMessage", "The following assets have newer versions available, but aren't selected to be synced.\nSelect any additional dependencies you would like to sync in order to avoid potential issues loading the updated packages.")
			);

		PackagesDialogModule.AddButton(
			DRT_CheckOut, 
			LOCTEXT("SyncDependenciesButton", "Sync"),
			LOCTEXT("SyncDependenciesButtonTip", "Sync with the selected dependencies included")
			);

		for (const FString& DependencyName : InDependencies)
		{
			UPackage* Package = FindPackage(nullptr, *DependencyName);
			PackagesDialogModule.AddPackageItem(Package, DependencyName, ECheckBoxState::Checked);
		}

		const EDialogReturnType UserResponse = PackagesDialogModule.ShowPackagesDialog();

		if (UserResponse == DRT_CheckOut)
		{
			TArray<UPackage*> SelectedPackages;
			PackagesDialogModule.GetResults(SelectedPackages, ECheckBoxState::Checked);

			for (UPackage* SelectedPackage : SelectedPackages)
			{
				if (SelectedPackage)
				{
					OutExtraPackagesToSync.Emplace(SelectedPackage->GetName());
				}
			}
		}
	}
#endif
}

bool ExtContentBrowserUtils::IsFavoriteFolder(const FString& FolderPath)
{
	return FExtContentBrowserSingleton::Get().FavoriteFolderPaths.Contains(FolderPath);
}

void ExtContentBrowserUtils::AddFavoriteFolder(const FString& FolderPath, bool bFlushConfig /*= true*/)
{
	FExtContentBrowserSingleton::Get().FavoriteFolderPaths.AddUnique(FolderPath);
}

void ExtContentBrowserUtils::RemoveFavoriteFolder(const FString& FolderPath, bool bFlushConfig /*= true*/)
{
	TArray<FString> FoldersToRemove;
	FoldersToRemove.Add(FolderPath);
	
	// Find and remove any subfolders
	for (const FString& FavoritePath : FExtContentBrowserSingleton::Get().FavoriteFolderPaths)
	{
		if (FavoritePath.StartsWith(FolderPath + TEXT("/")))
		{
			FoldersToRemove.Add(FavoritePath);
		}
	}
	for (const FString& FolderToRemove : FoldersToRemove)
	{
		FExtContentBrowserSingleton::Get().FavoriteFolderPaths.Remove(FolderToRemove);
	}
	if (bFlushConfig)
	{
		GConfig->Flush(false, GEditorPerProjectIni);
	}
}

const TArray<FString>& ExtContentBrowserUtils::GetFavoriteFolders()
{
	return FExtContentBrowserSingleton::Get().FavoriteFolderPaths;
}

int32 ExtContentBrowserUtils::GetMaxCookPathLen()
{
	if (GetDefault<UEditorExperimentalSettings>()->bEnableLongPathsSupport)
	{
		// Allow the longest path allowed by the system
		return FPlatformMisc::GetMaxPathLength();
	}
	else
	{
		// 260 characters is the limit on Windows, which is the shortest max path of any platforms that support cooking
		return 260;
	}
}

void ExtContentBrowserUtils::BeginAdvancedCopyPackages(TArray<FAssetData>& AssetList, TArray<FString>& AssetPaths, FString& DestinationPath)
{
	TMap<FString, TArray<UObject*> > SourcePathToLoadedAssets;
	FString DestPathWithTrailingSlash = DestinationPath / "";
	// Get a list of package names for input into Advanced Copy
	TArray<FName> InputNames;
	// Do not allow parent directories to be moved to themselves or children.
	TArray<FString> SourcePathNames = AssetPaths;

	for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
	{
		InputNames.Add(AssetList[AssetIdx].PackageName);
	}

	// Add any paths from selected folders
	for (const FString AssetPath : AssetPaths)
	{
		InputNames.Add(FName(*AssetPath));

	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().BeginAdvancedCopyPackages(InputNames, DestPathWithTrailingSlash);
}

#undef LOCTEXT_NAMESPACE

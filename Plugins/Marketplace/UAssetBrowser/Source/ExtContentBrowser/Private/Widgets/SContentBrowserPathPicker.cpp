// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SContentBrowserPathPicker.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "EditorStyleSet.h"
#include "GameFramework/Actor.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"

#define LOCTEXT_NAMESPACE "SContentBrowserPathPicker"

FString SContentBrowserPathPicker::LastAssetPath = FString("/Game");
void SContentBrowserPathPicker::Construct(const FArguments& InArgs, TSharedPtr<SWindow> InParentWindow)
{
	AssetFilenameSuffix = InArgs._AssetFilenameSuffix;
	HeadingText = InArgs._HeadingText;
	CreateButtonText = InArgs._CreateButtonText;
	OnCreateAssetAction = InArgs._OnCreateAssetAction;

	bIsReportingError = false;
	AssetPath = LastAssetPath;
	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = AssetPath;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateRaw(this, &SContentBrowserPathPicker::OnSelectAssetPath);

	SelectionDelegateHandle = USelection::SelectionChangedEvent.AddSP(this, &SContentBrowserPathPicker::OnLevelSelectionChanged);

	// Set up PathPickerConfig.
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	ParentWindow = InParentWindow;

	FString PackageName;
	ActorInstanceLabel.Empty();

	if( InArgs._DefaultNameOverride.IsEmpty() )
	{
		USelection* SelectedActors = GEditor->GetSelectedActors();
		for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
		{
			AActor* Actor = Cast<AActor>(*Iter);
			if(Actor)
			{
				ActorInstanceLabel += Actor->GetActorLabel();
				ActorInstanceLabel += TEXT("_");
				break;
			}
		}
	}
	else
	{
		ActorInstanceLabel = InArgs._DefaultNameOverride.ToString();
	}

	ActorInstanceLabel = UPackageTools::SanitizePackageName(ActorInstanceLabel + AssetFilenameSuffix);

	FString AssetName = ActorInstanceLabel;
	FString BasePath = AssetPath / AssetName;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(BasePath, TEXT(""), PackageName, AssetName);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
		]
#if 0
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(HeadingText)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(FileNameWidget, SEditableTextBox)
				.Text(FText::FromString(AssetName))
				.OnTextChanged(this, &SContentBrowserPathPicker::OnFilenameChanged)
			]
		]
#endif
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Right)
		.Padding(0, 20, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0, 2, 6, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.VAlign(VAlign_Bottom)
				.ContentPadding(FMargin(8, 2, 8, 2))
				.OnClicked(this, &SContentBrowserPathPicker::OnCreateAssetFromActorClicked)
				.IsEnabled(this, &SContentBrowserPathPicker::ISContentBrowserPathPickerEnabled)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.TextStyle(FAppStyle::Get(), "FlatButton.DefaultTextStyle")
				.Text(CreateButtonText)
			]
			+ SHorizontalBox::Slot()
			.Padding(0, 2, 0, 0)
			.AutoWidth()
			[
				SNew(SButton)
				.VAlign(VAlign_Bottom)
				.ContentPadding(FMargin(8, 2, 8, 2))
				.OnClicked(this, &SContentBrowserPathPicker::OnCancelCreateAssetFromActor)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
				.TextStyle(FAppStyle::Get(), "FlatButton.DefaultTextStyle")
				.Text(LOCTEXT("CancelButtonText", "Cancel"))
			]
		]
	];

	//OnFilenameChanged(FText::FromString(AssetName));
}

void SContentBrowserPathPicker::RequestDestroyParentWindow()
{
	USelection::SelectionChangedEvent.Remove(SelectionDelegateHandle);

	if (ParentWindow.IsValid())
	{
		ParentWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SContentBrowserPathPicker::OnCreateAssetFromActorClicked()
{
	RequestDestroyParentWindow();
	//OnCreateAssetAction.ExecuteIfBound(AssetPath / FileNameWidget->GetText().ToString());
	OnCreateAssetAction.ExecuteIfBound(AssetPath);// / FileNameWidget->GetText().ToString());
	return FReply::Handled();
}

FReply SContentBrowserPathPicker::OnCancelCreateAssetFromActor()
{
	RequestDestroyParentWindow();
	return FReply::Handled();
}

void SContentBrowserPathPicker::OnSelectAssetPath(const FString& Path)
{
	AssetPath = Path;
	LastAssetPath = AssetPath;
	//OnFilenameChanged(FileNameWidget->GetText());
}

void SContentBrowserPathPicker::OnLevelSelectionChanged(UObject* InObjectSelected)
{
	// When actor selection changes, this window should be destroyed.
	RequestDestroyParentWindow();
}

void SContentBrowserPathPicker::OnFilenameChanged(const FText& InNewName)
{
	TArray<FAssetData> AssetData;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(FName(*AssetPath), AssetData);

	FText ErrorText;
	if (!FFileHelper::IsFilenameValidForSaving(InNewName.ToString(), ErrorText) || !FName(*InNewName.ToString()).IsValidObjectName(ErrorText))
	{
		FileNameWidget->SetError(ErrorText);
		bIsReportingError = true;
		return;
	}
	else
	{
		// Check to see if the name conflicts
		for (auto Iter = AssetData.CreateConstIterator(); Iter; ++Iter)
		{
			if (Iter->AssetName.ToString() == InNewName.ToString())
			{
				FileNameWidget->SetError(LOCTEXT("AssetInUseError", "Asset name already in use!"));
				bIsReportingError = true;
				return;
			}
		}
	}

	FileNameWidget->SetError(FText::FromString(TEXT("")));
	bIsReportingError = false;
}

bool SContentBrowserPathPicker::ISContentBrowserPathPickerEnabled() const
{
	return !bIsReportingError;
}

#undef LOCTEXT_NAMESPACE

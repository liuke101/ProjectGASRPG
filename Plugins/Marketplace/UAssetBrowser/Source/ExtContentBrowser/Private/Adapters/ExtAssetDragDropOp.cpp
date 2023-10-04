// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ExtAssetDragDropOp.h"
#include "ExtAssetThumbnail.h"
#include "ExtContentBrowser.h"

#include "Engine/Level.h"
#include "ActorFactories/ActorFactory.h"
#include "GameFramework/Actor.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "ClassIconFinder.h"

TSharedRef<FExtAssetDragDropOp> FExtAssetDragDropOp::New(const FExtAssetData& InAssetData, UActorFactory* ActorFactory)
{
	TArray<FExtAssetData> AssetDataArray;
	AssetDataArray.Emplace(InAssetData);
	return New(MoveTemp(AssetDataArray), TArray<FString>(), ActorFactory);
}

TSharedRef<FExtAssetDragDropOp> FExtAssetDragDropOp::New(TArray<FExtAssetData> InAssetData, UActorFactory* ActorFactory)
{
	return New(MoveTemp(InAssetData), TArray<FString>(), ActorFactory);
}

TSharedRef<FExtAssetDragDropOp> FExtAssetDragDropOp::New(FString InAssetPath)
{
	TArray<FString> AssetPathsArray;
	AssetPathsArray.Emplace(MoveTemp(InAssetPath));
	return New(TArray<FExtAssetData>(), MoveTemp(AssetPathsArray), nullptr);
}

TSharedRef<FExtAssetDragDropOp> FExtAssetDragDropOp::New(TArray<FString> InAssetPaths)
{
	return New(TArray<FExtAssetData>(), MoveTemp(InAssetPaths), nullptr);
}

TSharedRef<FExtAssetDragDropOp> FExtAssetDragDropOp::New(TArray<FExtAssetData> InAssetData, TArray<FString> InAssetPaths, UActorFactory* ActorFactory)
{
	TSharedRef<FExtAssetDragDropOp> Operation = MakeShared<FExtAssetDragDropOp>();

	Operation->MouseCursor = EMouseCursor::GrabHandClosed;

	Operation->ThumbnailSize = 64;

	Operation->AssetData = MoveTemp(InAssetData);
	Operation->AssetPaths = MoveTemp(InAssetPaths);
	Operation->ActorFactory = ActorFactory;

	Operation->Init();

	Operation->Construct();
	return Operation;
}

FExtAssetDragDropOp::~FExtAssetDragDropOp()
{
	ThumbnailPool.Reset();
}

TSharedPtr<SWidget> FExtAssetDragDropOp::GetDefaultDecorator() const
{
	const int32 TotalCount = AssetData.Num() + AssetPaths.Num();

	TSharedPtr<SWidget> ThumbnailWidget;
	if (AssetThumbnail.IsValid())
	{
		ThumbnailWidget = AssetThumbnail->MakeThumbnailWidget();
	}
	else if (AssetPaths.Num() > 0)
	{
		ThumbnailWidget = 
			SNew(SOverlay)

			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Base"))
				.ColorAndOpacity(FLinearColor::Gray)
			]
		
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Mask"))
			];
	}
	else
	{
		ThumbnailWidget = 
			SNew(SImage)
			.Image(FAppStyle::GetDefaultBrush());
	}
	
	const FSlateBrush* SubTypeBrush = FAppStyle::GetDefaultBrush();
	FLinearColor SubTypeColor = FLinearColor::White;
	if (AssetThumbnail.IsValid() && AssetPaths.Num() > 0)
	{
		SubTypeBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
		SubTypeColor = FLinearColor::Gray;
	}
	else if (ActorFactory.IsValid() && AssetData.Num() > 0)
	{
#if ECB_LEGACY
		AActor* DefaultActor = ActorFactory->GetDefaultActor(AssetData[0]);
		SubTypeBrush = FClassIconFinder::FindIconForActor(DefaultActor);
#endif
	}

	return 
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.AssetDragDropTooltipBackground"))
		.Content()
		[
			SNew(SHorizontalBox)

			// Left slot is for the thumbnail
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(SBox) 
				.WidthOverride(ThumbnailSize) 
				.HeightOverride(ThumbnailSize)
				.Content()
				[
					SNew(SOverlay)

					+SOverlay::Slot()
					[
						ThumbnailWidget.ToSharedRef()
					]

					+SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					.Padding(FMargin(0, 4, 0, 0))
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("Menu.Background"))
						.Visibility(TotalCount > 1 ? EVisibility::Visible : EVisibility::Collapsed)
						.Content()
						[
							SNew(STextBlock)
							.Text(FText::AsNumber(TotalCount))
						]
					]

					+SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(FMargin(4, 4))
					[
						SNew(SImage)
						.Image(SubTypeBrush)
						.Visibility(SubTypeBrush != FAppStyle::GetDefaultBrush() ? EVisibility::Visible : EVisibility::Collapsed)
						.ColorAndOpacity(SubTypeColor)
					]
				]
			]

			// Right slot is for optional tooltip
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(80)
				.Content()
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(3.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SImage) 
						.Image(this, &FExtAssetDragDropOp::GetIcon)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0,0,3,0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock) 
						.Text(this, &FExtAssetDragDropOp::GetDecoratorText)
					]
				]
			]
		];
}

FText FExtAssetDragDropOp::GetDecoratorText() const
{
	if (CurrentHoverText.IsEmpty())
	{
		const int32 TotalCount = AssetData.Num() + AssetPaths.Num();
		if (TotalCount > 0)
		{
			const FText FirstItemText = AssetData.Num() > 0 ? FText::FromName(AssetData[0].AssetName) : FText::FromString(AssetPaths[0]);
			return (TotalCount == 1)
				? FirstItemText
				: FText::Format(NSLOCTEXT("ContentBrowser", "AssetDragDropOpDescriptionMulti", "'{0}' and {1} {1}|plural(one=other,other=others)"), FirstItemText, TotalCount - 1);
		}
	}
	
	return CurrentHoverText;
}

void FExtAssetDragDropOp::Init()
{
	if (AssetData.Num() > 0 && ThumbnailSize > 0)
	{
#if ECB_LEGACY
		// Load all assets first so that there is no loading going on while attempting to drag
		// Can cause unsafe frame reentry 
		for (FExtAssetData& Data : AssetData)
		{
			Data.GetAsset();
		}
#endif

		// Create a thumbnail pool to hold the single thumbnail rendered
		ThumbnailPool = MakeShared<FExtAssetThumbnailPool>(1, /*InAreRealTileThumbnailsAllowed=*/false);

		// Create the thumbnail handle
		AssetThumbnail = MakeShared<FExtAssetThumbnail>(AssetData[0], ThumbnailSize, ThumbnailSize, ThumbnailPool);

		// Request the texture then tick the pool once to render the thumbnail
		AssetThumbnail->GetViewportRenderTargetTexture();
		ThumbnailPool->Tick(0);
	}
}

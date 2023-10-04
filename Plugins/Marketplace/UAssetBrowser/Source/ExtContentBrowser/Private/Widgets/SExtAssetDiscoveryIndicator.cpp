// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SExtAssetDiscoveryIndicator.h"
#include "ExtContentBrowserSingleton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AssetDiscoveryIndicator"

namespace AssetDiscoveryIndicatorConstants
{
	static const FMargin Padding(4, 0);
	static const FMargin SubStatusTextPadding(2, 0, 4, 0);
}

SExtAssetDiscoveryIndicator::~SExtAssetDiscoveryIndicator()
{
	if ( FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")) )
	{
		FExtAssetRegistry& AssetRegistry = FExtContentBrowserSingleton::GetAssetRegistry();
		AssetRegistry.OnAssetGatherProgressUpdated().RemoveAll( this );
		//AssetRegistryModule.Get().OnFilesLoaded().RemoveAll( this );
	}
}

void SExtAssetDiscoveryIndicator::Construct( const FArguments& InArgs )
{
	FExtAssetRegistry& AssetRegistry = FExtContentBrowserSingleton::GetAssetRegistry();
	AssetRegistry.OnAssetGatherProgressUpdated().AddSP( this, &SExtAssetDiscoveryIndicator::OnAssetRegistryFileLoadProgress );
	//AssetRegistryModule.Get().OnFilesLoaded().AddSP( this, &SExtAssetDiscoveryIndicator::OnAssetRegistryFilesLoaded );

	ScaleMode = InArgs._ScaleMode;

	FadeAnimation = FCurveSequence();
	FadeAnimation.AddCurve(0.f, 4.f); // Add some space at the beginning to delay before fading in
	ScaleCurve = FadeAnimation.AddCurve(4.f, 0.75f);
	FadeCurve = FadeAnimation.AddCurve(4.75f, 0.75f);
	FadeAnimation.AddCurve(5.5f, 1.f); // Add some space at the end to cause a short delay before fading out

	MainStatusText = LOCTEXT("InitializingAssetDiscovery", "Initializing Asset Discovery");
	StatusTextWrapWidth = 0.0f;

	if (AssetRegistry.IsGatheringAssets() )
	{
		// Loading assets, marquee while discovering package files
		Progress = TOptional<float>();

		if ( InArgs._FadeIn )
		{
			FadeAnimation.Play( this->AsShared() );
		}
		else
		{
			FadeAnimation.JumpToEnd();
		}
	}
	else
	{
		// Already done loading assets, set to complete and don't play the complete animation
		Progress = 1.f;
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(InArgs._Padding)
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0)
			//.BorderBackgroundColor(this, &SExtAssetDiscoveryIndicator::GetBorderBackgroundColor)
			//.ColorAndOpacity(this, &SExtAssetDiscoveryIndicator::GetIndicatorColorAndOpacity)
			.DesiredSizeScale(this, &SExtAssetDiscoveryIndicator::GetIndicatorDesiredSizeScale)
			.Visibility(this, &SExtAssetDiscoveryIndicator::GetIndicatorVisibility)
			.VAlign(VAlign_Center)
			[
#if 0
				SNew(SVerticalBox)

				// Text
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(AssetDiscoveryIndicatorConstants::Padding)
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(FAppStyle::GetFontStyle("AssetDiscoveryIndicator.MainStatusFont"))
						.Text(this, &SExtAssetDiscoveryIndicator::GetMainStatusText)
						.WrapTextAt(this, &SExtAssetDiscoveryIndicator::GetStatusTextWrapWidth)
						.Justification(ETextJustify::Center)
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.Padding(AssetDiscoveryIndicatorConstants::SubStatusTextPadding)
						.Visibility(this, &SExtAssetDiscoveryIndicator::GetSubStatusTextVisibility)
						[
							SNew(STextBlock)
							.Font(FAppStyle::GetFontStyle("AssetDiscoveryIndicator.SubStatusFont"))
							.Text(this, &SExtAssetDiscoveryIndicator::GetSubStatusText)
							.WrapTextAt(this, &SExtAssetDiscoveryIndicator::GetStatusTextWrapWidth)
							.Justification(ETextJustify::Center)
						]
					]
				]

				// Progress bar
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(AssetDiscoveryIndicatorConstants::Padding)
				[
					SNew(SProgressBar)
					.Percent(this, &SExtAssetDiscoveryIndicator::GetProgress)
				]
#endif
				SNew(SHorizontalBox)

				// Progress bar
				+ SHorizontalBox::Slot()
				//.Padding(AssetDiscoveryIndicatorConstants::Padding)
				.Padding(AssetDiscoveryIndicatorConstants::Padding)
				[
					SNew(SBox)
					.Padding(0)
					.WidthOverride(100.f)
					.MinDesiredWidth(100.f)
					[
						SNew(SProgressBar)
						.Percent(this, &SExtAssetDiscoveryIndicator::GetProgress)
					]
				]

				// Text
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(AssetDiscoveryIndicatorConstants::Padding)
				[
					SNew(SBox)
					//.Padding(AssetDiscoveryIndicatorConstants::SubStatusTextPadding)
					.Padding(0)
					.Visibility(this, &SExtAssetDiscoveryIndicator::GetSubStatusTextVisibility)
					[
						SNew(STextBlock)
						.Font(FAppStyle::GetFontStyle("AssetDiscoveryIndicator.SubStatusFont"))
						.Text(this, &SExtAssetDiscoveryIndicator::GetSubStatusText)
						.WrapTextAt(this, &SExtAssetDiscoveryIndicator::GetStatusTextWrapWidth)
						.Justification(ETextJustify::Center)
					]
				]
			]
		]
	];
}

void SExtAssetDiscoveryIndicator::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// Calculate the wrapping width based on our complete alloted width
	// We do this rather than auto-wrap because the size of the text changes, and auto-wrapping prevents the text block from being able to grow if the text shrinks
	StatusTextWrapWidth = AllottedGeometry.GetLocalSize().X - (AssetDiscoveryIndicatorConstants::Padding.Left + AssetDiscoveryIndicatorConstants::Padding.Right);
}

void SExtAssetDiscoveryIndicator::OnAssetRegistryFileLoadProgress(const FExtAssetRegistry::FAssetGatherProgressUpdateData& ProgressUpdateData)
{
	if (ProgressUpdateData.bIsDiscoveringAssetFiles)
	{
		// Marquee while we're discovering asset files as we can't yet show an accurate percentage
		Progress = TOptional<float>();
		MainStatusText = LOCTEXT("DiscoveringAssetFiles", "Discovering Asset Files");
		SubStatusText = FText::Format(LOCTEXT("XFilesFoundFmt", "{0} files found"), FText::AsNumber(ProgressUpdateData.NumTotalAssets));
	}
	else
	{
		if (ProgressUpdateData.NumTotalAssets > 0)
		{
			Progress = ProgressUpdateData.NumAssetsProcessedByAssetRegistry / (float)ProgressUpdateData.NumTotalAssets;
		}
		else
		{
			Progress = 0.0f;
		}

		if (ProgressUpdateData.NumAssetsPendingDataLoad > 0)
		{
			MainStatusText = LOCTEXT("DiscoveringAssetData", "Discovering Asset Data");
			SubStatusText = FText::Format(LOCTEXT("XAssetsRemainingFmt", "{0} assets remaining"), FText::AsNumber(ProgressUpdateData.NumAssetsPendingDataLoad));
		}
		else
		{
			const int32 NumAssetsLeftToProcess = ProgressUpdateData.NumTotalAssets - ProgressUpdateData.NumAssetsProcessedByAssetRegistry;
			if (NumAssetsLeftToProcess == 0)
			{
				OnAssetRegistryFilesLoaded();
			}
			else
			{
				//if (FadeAnimation.IsAtStart())
				{
					FadeAnimation.JumpToEnd();
				}

				MainStatusText = LOCTEXT("ProcessingAssetData", "Processing Asset Data");
				SubStatusText = FText::Format(LOCTEXT("XAssetsRemainingFmt", "{0} assets remaining"), FText::AsNumber(NumAssetsLeftToProcess));
			}
		}
	}
}

void SExtAssetDiscoveryIndicator::OnAssetRegistryFilesLoaded()
{
	if (!FadeAnimation.IsAtStart())
	{
		MainStatusText = LOCTEXT("FinishedAssetDiscovery", "Finished Asset Discovery");
		SubStatusText = FText::GetEmpty();

		if (FadeAnimation.IsPlaying())
		{
			if (FadeAnimation.IsForward())
			{
				// Still fading in - reverse so we fade back out
				FadeAnimation.Reverse();
			}
			else
			{
				// Do nothing - already fading out
			}
		}
		else
		{
			// Play the fade out animation
			FadeAnimation.PlayReverse(this->AsShared());
		}
	}
}

FText SExtAssetDiscoveryIndicator::GetMainStatusText() const
{
	return MainStatusText;
}

FText SExtAssetDiscoveryIndicator::GetSubStatusText() const
{
	return SubStatusText;
}

TOptional<float> SExtAssetDiscoveryIndicator::GetProgress() const
{
	return Progress;
}

EVisibility SExtAssetDiscoveryIndicator::GetSubStatusTextVisibility() const
{
	return (SubStatusText.IsEmpty()) ? EVisibility::Collapsed : EVisibility::Visible;
}

float SExtAssetDiscoveryIndicator::GetStatusTextWrapWidth() const
{
	return StatusTextWrapWidth;
}

FSlateColor SExtAssetDiscoveryIndicator::GetBorderBackgroundColor() const
{
	return FSlateColor(FLinearColor(1, 1, 1, 0.8f * FadeCurve.GetLerp()));
}

FLinearColor SExtAssetDiscoveryIndicator::GetIndicatorColorAndOpacity() const
{
	return FLinearColor(1, 1, 1, FadeCurve.GetLerp());
}

FVector2D SExtAssetDiscoveryIndicator::GetIndicatorDesiredSizeScale() const
{
	const float Lerp = ScaleCurve.GetLerp();

	switch (ScaleMode)
	{
	case EAssetDiscoveryIndicatorScaleMode::Scale_Horizontal: return FVector2D(Lerp, 1);
	case EAssetDiscoveryIndicatorScaleMode::Scale_Vertical: return FVector2D(1, Lerp);
	case EAssetDiscoveryIndicatorScaleMode::Scale_Both: return FVector2D(Lerp, Lerp);
	default:
		return FVector2D(1, 1);
	}
}

EVisibility SExtAssetDiscoveryIndicator::GetIndicatorVisibility() const
{
	//return FadeAnimation.IsAtStart() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
	return FExtContentBrowserSingleton::GetAssetRegistry().IsGatheringAssets() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE

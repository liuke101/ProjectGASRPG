// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtAssetThumbnail.h"
#include "ExtContentBrowser.h"
#include "ExtPackageUtils.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtContentBrowserSettings.h"
#include "ExtContentBrowserStyle.h"

#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Layout/Margin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameViewportClient.h"
#include "Interfaces/Interface_AsyncCompilation.h"
#include "Modules/ModuleManager.h"
#include "Animation/CurveHandle.h"
#include "Animation/CurveSequence.h"
#include "Textures/SlateTextureData.h"
#include "Fonts/SlateFontInfo.h"
#include "Application/ThrottleManager.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SViewport.h"
#include "EditorStyleSet.h"
#include "RenderingThread.h"
#include "Settings/ContentBrowserSettings.h"
#include "RenderUtils.h"
#include "Editor/UnrealEdEngine.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Slate/SlateTextures.h"
#include "ObjectTools.h"
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#include "IAssetTools.h"
#include "AssetTypeActions_Base.h"
#include "AssetToolsModule.h"
#include "Styling/SlateIconFinder.h"
#include "ClassIconFinder.h"
#include "IVREditorModule.h"
#include "Framework/Application/SlateApplication.h"

//-------------------------------------------------------------------------------------
// SExtAssetThumbnail - Thumbnail widget for display thumbnail of .uasset file
//

class SExtAssetThumbnail : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SExtAssetThumbnail )
		: _Style("AssetThumbnail")
		, _ThumbnailPool(NULL)
		, _AllowFadeIn(false)
		, _ForceGenericThumbnail(false)
		, _AllowHintText(true)
		, _AllowAssetSpecificThumbnailOverlay(false)
		, _Label(EThumbnailLabel::ClassName)
		, _HighlightedText(FText::GetEmpty())
		, _HintColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
		, _ClassThumbnailBrushOverride(NAME_None)
		, _AssetTypeColorOverride()
		, _Padding(0)
		, _GenericThumbnailSize(64)
		{}

		SLATE_ARGUMENT(FName, Style)
		SLATE_ARGUMENT(TSharedPtr<FExtAssetThumbnail>, AssetThumbnail)
		SLATE_ARGUMENT(TSharedPtr<FExtAssetThumbnailPool>, ThumbnailPool)
		SLATE_ARGUMENT(bool, AllowFadeIn)
		SLATE_ARGUMENT(bool, ForceGenericThumbnail)
		SLATE_ARGUMENT(bool, AllowHintText)
		SLATE_ARGUMENT(bool, AllowAssetSpecificThumbnailOverlay)
		//SLATE_ARGUMENT(bool, AllowRealTimeOnHovered)
		SLATE_ARGUMENT(EThumbnailLabel::Type, Label)
		SLATE_ATTRIBUTE(FText, HighlightedText)
		SLATE_ATTRIBUTE(FLinearColor, HintColorAndOpacity)
		SLATE_ARGUMENT(FName, ClassThumbnailBrushOverride)
		SLATE_ARGUMENT(TOptional<FLinearColor>, AssetTypeColorOverride)
		SLATE_ARGUMENT(FMargin, Padding)
		SLATE_ATTRIBUTE(int32, GenericThumbnailSize)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs )
	{
		Style = InArgs._Style;
		HighlightedText = InArgs._HighlightedText;
		Label = InArgs._Label;
		HintColorAndOpacity = InArgs._HintColorAndOpacity;
		bAllowHintText = InArgs._AllowHintText;
		ThumbnailBrush = nullptr;
		ClassIconBrush = nullptr;
		AssetThumbnail = InArgs._AssetThumbnail;
		bHasRenderedThumbnail = false;
		WidthLastFrame = 0;
		GenericThumbnailBorderPadding = 2.f;
		GenericThumbnailSize = InArgs._GenericThumbnailSize;

		AssetThumbnail->OnAssetDataChanged().AddSP(this, &SExtAssetThumbnail::OnAssetDataChanged);

		const FExtAssetData& AssetData = AssetThumbnail->GetAssetData();
		const bool bInvalidUAsset = !AssetData.IsValid() && AssetData.IsUAsset();

		UClass* Class = bInvalidUAsset ? nullptr : FindObjectSafe<UClass>(nullptr, *AssetData.AssetClass.ToString());
		static FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TSharedPtr<IAssetTypeActions> AssetTypeActions;
		if ( Class != NULL )
		{
			AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Class).Pin();
		}

		AssetColor = bInvalidUAsset ? FLinearColor::Red : FLinearColor::White;
		if( InArgs._AssetTypeColorOverride.IsSet() )
		{
			AssetColor = InArgs._AssetTypeColorOverride.GetValue();
		}
		else if ( AssetTypeActions.IsValid() )
		{
			AssetColor = AssetTypeActions->GetTypeColor();
		}

		TSharedRef<SOverlay> OverlayWidget = SNew(SOverlay);

		UpdateThumbnailClass();

		ClassThumbnailBrushOverride = InArgs._ClassThumbnailBrushOverride;

		AssetBackgroundBrushName = *(Style.ToString() + TEXT(".AssetBackground"));
		ClassBackgroundBrushName = *(Style.ToString() + TEXT(".ClassBackground"));

		// The generic representation of the thumbnail, for use before the rendered version, if it exists
		OverlayWidget->AddSlot()
		.Padding(InArgs._Padding)
		[
			SAssignNew(AssetBackgroundWidget, SBorder)
			.BorderImage(GetAssetBackgroundBrush())
			.Padding(GenericThumbnailBorderPadding)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Visibility(this, &SExtAssetThumbnail::GetGenericThumbnailVisibility)
			[
				SNew(SOverlay)

				+SOverlay::Slot()
				[
					SAssignNew(GenericLabelTextBlock, STextBlock)
					.Text(GetLabelText())
					.Font(GetTextFont())
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FAppStyle::GetColor(Style, ".ColorAndOpacity"))
					.HighlightText(HighlightedText)
				]

				+SOverlay::Slot()
				[
					SAssignNew(GenericThumbnailImage, SImage)
					.DesiredSizeOverride(this, &SExtAssetThumbnail::GetGenericThumbnailDesiredSize)
					.Image(this, &SExtAssetThumbnail::GetClassThumbnailBrush)
				]
			]
		];

		if ( InArgs._ThumbnailPool.IsValid() && !InArgs._ForceGenericThumbnail )
		{
			ViewportFadeAnimation = FCurveSequence();
			ViewportFadeCurve = ViewportFadeAnimation.AddCurve(0.f, 0.25f, ECurveEaseFunction::QuadOut);

			TSharedPtr<SViewport> Viewport = 
				SNew( SViewport )
				.EnableGammaCorrection(false)
				// In VR editor every widget is in the world and gamma corrected by the scene renderer.  Thumbnails will have already been gamma
				// corrected and so they need to be reversed
				.ReverseGammaCorrection(IVREditorModule::Get().IsVREditorModeActive())
				.EnableBlending(true);

			Viewport->SetViewportInterface( AssetThumbnail.ToSharedRef() );
			AssetThumbnail->GetViewportRenderTargetTexture(); // Access the render texture to push it on the stack if it isn't already rendered

			InArgs._ThumbnailPool->OnThumbnailRendered().AddSP(this, &SExtAssetThumbnail::OnThumbnailRendered);
			InArgs._ThumbnailPool->OnThumbnailRenderFailed().AddSP(this, &SExtAssetThumbnail::OnThumbnailRenderFailed);

			if ( ShouldRender() && (!InArgs._AllowFadeIn || InArgs._ThumbnailPool->IsRendered(AssetThumbnail)) )
			{
				bHasRenderedThumbnail = true;
				ViewportFadeAnimation.JumpToEnd();
			}

			// The viewport for the rendered thumbnail, if it exists
			OverlayWidget->AddSlot()
			[
				SAssignNew(RenderedThumbnailWidget, SBorder)
				.Padding(InArgs._Padding)
				.BorderImage(FStyleDefaults::GetNoBrush())
				.ColorAndOpacity(this, &SExtAssetThumbnail::GetViewportColorAndOpacity)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					Viewport.ToSharedRef()
				]
			];
		}

		if( ThumbnailClass.Get() && bIsClassType)
		{
			OverlayWidget->AddSlot()
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Right)
			.Padding(TAttribute<FMargin>(this, &SExtAssetThumbnail::GetClassIconPadding))
			[
				SAssignNew(ClassIconWidget, SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				[
					SNew(SImage)
					.Image(this, &SExtAssetThumbnail::GetClassIconBrush)
				]
			];
		}

		if( bAllowHintText )
		{
			OverlayWidget->AddSlot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				.Padding(FMargin(2, 2, 2, 2))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush(Style, ".HintBackground"))
					.BorderBackgroundColor(this, &SExtAssetThumbnail::GetHintBackgroundColor) //Adjust the opacity of the border itself
					.ColorAndOpacity(HintColorAndOpacity) //adjusts the opacity of the contents of the border
					.Visibility(this, &SExtAssetThumbnail::GetHintTextVisibility)
					.Padding(0)
					[
						SAssignNew(HintTextBlock, STextBlock)
						.Text(GetLabelText())
						.Font(GetHintTextFont())
						.ColorAndOpacity(FAppStyle::GetColor(Style, ".HintColorAndOpacity"))
						.HighlightText(HighlightedText)
					]
				];
		}

		// The asset color strip
		OverlayWidget->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SAssignNew(AssetColorStripWidget, SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(AssetColor)
			.Padding(this, &SExtAssetThumbnail::GetAssetColorStripPadding)
		];

#if ECB_LEGACY
		if( InArgs._AllowAssetSpecificThumbnailOverlay && AssetTypeActions.IsValid() )
		{
			// Does the asset provide an additional thumbnail overlay?
			TSharedPtr<SWidget> AssetSpecificThumbnailOverlay = AssetTypeActions->GetThumbnailOverlay(AssetData.AssetData);
			if( AssetSpecificThumbnailOverlay.IsValid() )
			{
				OverlayWidget->AddSlot()
				[
					AssetSpecificThumbnailOverlay.ToSharedRef()
				];
			}
		}
#endif
		if (InArgs._AllowAssetSpecificThumbnailOverlay)
		{
			// Does the asset provide an additional thumbnail overlay?
			TSharedPtr<SWidget> AssetSpecificThumbnailOverlay = AssetThumbnail->GetSpecificThumbnailOverlay();
			if (AssetSpecificThumbnailOverlay.IsValid())
			{
				OverlayWidget->AddSlot()
				[
					AssetSpecificThumbnailOverlay.ToSharedRef()
				];
			}
		}

		ChildSlot
		[
			OverlayWidget
		];

		UpdateThumbnailVisibilities();

	}

	void UpdateThumbnailClass()
	{
		const FExtAssetData& AssetData = AssetThumbnail->GetAssetData();
		ThumbnailClass = MakeWeakObjectPtr(const_cast<UClass*>(AssetData.GetIconClass(&bIsClassType)));

		// For non-class types, use the default based upon the actual asset class
		// This has the side effect of not showing a class icon for assets that don't have a proper thumbnail image available
		const FName DefaultThumbnail = (bIsClassType) ? NAME_None : FName(*FString::Printf(TEXT("ClassThumbnail.%s"), *AssetThumbnail->GetAssetData().AssetClass.ToString()));
		ThumbnailBrush = FClassIconFinder::FindThumbnailForClass(ThumbnailClass.Get(), DefaultThumbnail);

		ClassIconBrush = FSlateIconFinder::FindIconBrushForClass(ThumbnailClass.Get());

	}

	FSlateColor GetHintBackgroundColor() const
	{
		const FLinearColor Color = HintColorAndOpacity.Get();
		return FSlateColor( FLinearColor( Color.R, Color.G, Color.B, FMath::Lerp( 0.0f, 0.5f, Color.A ) ) );
	}

	// SWidget implementation
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
#if ECB_LEGACY
		if (!GetDefault<UExtContentBrowserSettings>()->RealTimeThumbnails )
		{
			// Update hovered thumbnails if we are not already updating them in real-time
			AssetThumbnail->RefreshThumbnail();
		}
#endif		
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		if ( WidthLastFrame != AllottedGeometry.Size.X )
		{
			WidthLastFrame = AllottedGeometry.Size.X;

			// The width changed, update the font
			if ( GenericLabelTextBlock.IsValid() )
			{
				GenericLabelTextBlock->SetFont( GetTextFont() );
				GenericLabelTextBlock->SetWrapTextAt( GetTextWrapWidth() );
			}

			if ( HintTextBlock.IsValid() )
			{
				HintTextBlock->SetFont( GetHintTextFont() );
				HintTextBlock->SetWrapTextAt( GetTextWrapWidth() );
			}
		}
	}

private:
	void OnAssetDataChanged()
	{
		if ( GenericLabelTextBlock.IsValid() )
		{
			GenericLabelTextBlock->SetText( GetLabelText() );
		}

		if ( HintTextBlock.IsValid() )
		{
			HintTextBlock->SetText( GetLabelText() );
		}

		// Check if the asset has a thumbnail.
		const FObjectThumbnail* ObjectThumbnail = NULL;
		FThumbnailMap ThumbnailMap;
		if( AssetThumbnail->GetAsset() )
		{
			FName FullAssetName = FName( *(AssetThumbnail->GetAssetData().GetFullName()) );
			TArray<FName> ObjectNames;
			ObjectNames.Add( FullAssetName );
			ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectNames, ThumbnailMap);
			ObjectThumbnail = ThumbnailMap.Find( FullAssetName );
		}

		bHasRenderedThumbnail = ObjectThumbnail && !ObjectThumbnail->IsEmpty();
		ViewportFadeAnimation.JumpToEnd();
		AssetThumbnail->GetViewportRenderTargetTexture(); // Access the render texture to push it on the stack if it isnt already rendered

		const FExtAssetData& AssetData = AssetThumbnail->GetAssetData();

		UClass* Class = FindObject<UClass>(nullptr, *AssetData.AssetClass.ToString());
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TWeakPtr<IAssetTypeActions> AssetTypeActions;
		if ( Class != NULL )
		{
			AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Class);
		}

		UpdateThumbnailClass();

		AssetColor = FLinearColor::White;
		if ( AssetTypeActions.IsValid() )
		{
			AssetColor = AssetTypeActions.Pin()->GetTypeColor();
			AssetBackgroundWidget->SetBorderBackgroundColor(AssetColor.CopyWithNewOpacity(0.3f));
			AssetColorStripWidget->SetBorderBackgroundColor(AssetColor);
		}

		UpdateThumbnailVisibilities();
	}

	FSlateFontInfo GetTextFont() const
	{
		return FAppStyle::GetFontStyle( WidthLastFrame <= 64 ? FAppStyle::Join(Style, ".FontSmall") : FAppStyle::Join(Style, ".Font") );
	}

	FSlateFontInfo GetHintTextFont() const
	{
		return FAppStyle::GetFontStyle( WidthLastFrame <= 64 ? FAppStyle::Join(Style, ".HintFontSmall") : FAppStyle::Join(Style, ".HintFont") );
	}

	float GetTextWrapWidth() const
	{
		return WidthLastFrame - GenericThumbnailBorderPadding * 2.f;
	}

	const FSlateBrush* GetAssetBackgroundBrush() const
	{
		return FAppStyle::GetBrush(AssetBackgroundBrushName);
	}

	const FSlateBrush* GetClassBackgroundBrush() const
	{

		return FAppStyle::GetBrush(ClassBackgroundBrushName);
	}

	FSlateColor GetViewportBorderColorAndOpacity() const
	{
		return FLinearColor(AssetColor.R, AssetColor.G, AssetColor.B, ViewportFadeCurve.GetLerp());
	}

	FLinearColor GetViewportColorAndOpacity() const
	{
		return FLinearColor(1, 1, 1, ViewportFadeCurve.GetLerp());
	}
	
	EVisibility GetViewportVisibility() const
	{
		return bHasRenderedThumbnail ? EVisibility::Visible : EVisibility::Collapsed;
	}

	float GetAssetColorStripHeight() const
	{
		return 2.0f;
	}

	FMargin GetAssetColorStripPadding() const
	{
		const float Height = GetAssetColorStripHeight();
		return FMargin(0,Height,0,0);
	}

	const FSlateBrush* GetClassThumbnailBrush() const
	{
		if (ClassThumbnailBrushOverride.IsNone())
		{
			return ThumbnailBrush;
		}
		else
		{
			// Instead of getting the override thumbnail directly from the editor style here get it from the
			// ClassIconFinder since it may have additional styles registered which can be searched by passing
			// it as a default with no class to search for.
			return FClassIconFinder::FindThumbnailForClass(nullptr, ClassThumbnailBrushOverride);
		}
	}

	EVisibility GetClassThumbnailVisibility() const
	{
		if(!bHasRenderedThumbnail)
		{
			const FSlateBrush* ClassThumbnailBrush = GetClassThumbnailBrush();
			if( ClassThumbnailBrush && ThumbnailClass.Get() )
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	EVisibility GetGenericThumbnailVisibility() const
	{
		return (bHasRenderedThumbnail && ViewportFadeAnimation.IsAtEnd()) ? EVisibility::Collapsed : EVisibility::Visible;
	}

	const FSlateBrush* GetClassIconBrush() const
	{
		return ClassIconBrush;
	}

	FMargin GetClassIconPadding() const
	{
		const float Height = GetAssetColorStripHeight();
		return FMargin(0,0,0,Height);
	}

	EVisibility GetHintTextVisibility() const
	{
		if ( bAllowHintText && ( bHasRenderedThumbnail || !GenericLabelTextBlock.IsValid() ) && HintColorAndOpacity.Get().A > 0 )
		{
			return EVisibility::Visible;
		}

		return EVisibility::Collapsed;
	}

	void OnThumbnailRendered(const FExtAssetData& AssetData)
	{
		if ( !bHasRenderedThumbnail && AssetData == AssetThumbnail->GetAssetData() && ShouldRender() )
		{
			OnRenderedThumbnailChanged( true );
			ViewportFadeAnimation.Play( this->AsShared() );
		}
	}

	void OnThumbnailRenderFailed(const FExtAssetData& AssetData)
	{
		if ( bHasRenderedThumbnail && AssetData == AssetThumbnail->GetAssetData() )
		{
			OnRenderedThumbnailChanged( false );
		}
	}

	bool ShouldRender() const
	{
		const FExtAssetData& AssetData = AssetThumbnail->GetAssetData();

#if ECB_FEA_SHOW_INVALID
		if ( !AssetData.IsValid() )
		{
			return false;
		}
#endif

		if( AssetData.IsAssetLoaded() )
		{
			// Loaded asset, return true if there is a rendering info for it
			UObject* Asset = AssetData.GetAsset();
			FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Asset );
			if ( RenderInfo != NULL && RenderInfo->Renderer != NULL )
			{
				return true;
			}
		}

		const FObjectThumbnail* CachedThumbnail = ThumbnailTools::FindCachedThumbnail(AssetData.GetFullName());
		if ( CachedThumbnail != NULL )
		{
			// There is a cached thumbnail for this asset, we should render it
			return !CachedThumbnail->IsEmpty();
		}

		// Unloaded blueprint or asset that may have a custom thumbnail, check to see if there is a thumbnail in the package to render
		if (AssetData.HasThumbnail())
		{
			return true;
		}

		if ( AssetData.AssetClass != UBlueprint::StaticClass()->GetFName() )
		{
			// If we are not a blueprint, see if the CDO of the asset's class has a rendering info
			// Blueprints can't do this because the rendering info is based on the generated class
			UClass* AssetClass = FindObject<UClass>(nullptr, *AssetData.AssetClass.ToString());

			if ( AssetClass )
			{
				FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( AssetClass->GetDefaultObject() );
				if ( RenderInfo != NULL && RenderInfo->Renderer != NULL )
				{
					return true;
				}
			}
		}

		// Unloaded blueprint or asset that may have a custom thumbnail, check to see if there is a thumbnail in the package to render
		FString PackageFilename;
		if ( FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), &PackageFilename) )
		{
			TSet<FName> ObjectFullNames;
			FThumbnailMap ThumbnailMap;

			FName ObjectFullName = FName(*AssetData.GetFullName());
			ObjectFullNames.Add(ObjectFullName);

			ThumbnailTools::LoadThumbnailsFromPackage(PackageFilename, ObjectFullNames, ThumbnailMap);

			const FObjectThumbnail* ThumbnailPtr = ThumbnailMap.Find(ObjectFullName);
			if (ThumbnailPtr)
			{
				const FObjectThumbnail& ObjectThumbnail = *ThumbnailPtr;
				return ObjectThumbnail.GetImageWidth() > 0 && ObjectThumbnail.GetImageHeight() > 0 && ObjectThumbnail.GetCompressedDataSize() > 0;
			}
		}

		return false;
	}

	FText GetLabelText() const
	{
		if( Label != EThumbnailLabel::NoLabel )
		{
			if ( Label == EThumbnailLabel::ClassName )
			{
				return GetAssetClassDisplayName();
			}
			else if ( Label == EThumbnailLabel::AssetName ) 
			{
				return GetAssetDisplayName();
			}
		}
		return FText::GetEmpty();
	}

	FText GetDisplayNameForClass( UClass* Class, const FExtAssetData* InExtAssetData = nullptr) const
	{
		FText ClassDisplayName;
		if ( Class )
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Class);
			
			if ( AssetTypeActions.IsValid() )
			{
				if (InExtAssetData != nullptr)
				{
					FAssetTypeActions_Base* BaseAssetTypeAction = static_cast<FAssetTypeActions_Base*>(AssetTypeActions.Pin().Get());
					if (BaseAssetTypeAction != nullptr)
					{
						ClassDisplayName = BaseAssetTypeAction->GetDisplayNameFromAssetData(InExtAssetData->AssetData);
					}
				}

				if (ClassDisplayName.IsEmpty())
				{
					ClassDisplayName = AssetTypeActions.Pin()->GetName();
				}
			}

			if ( ClassDisplayName.IsEmpty() )
			{
				ClassDisplayName = FText::FromString( FName::NameToDisplayString(*Class->GetName(), false) );
			}
		}

		return ClassDisplayName;
	}

	FText GetAssetClassDisplayName() const
	{
		const FExtAssetData& AssetData = AssetThumbnail->GetAssetData();

		/*
		FName(TEXT("Code"));
		FName(TEXT("Code Missing"));
		FName(TEXT("Map Missing"));
		FName(TEXT("Package Missing"));
		*/

		FString AssetClass = AssetData.AssetClass.ToString();
		
		const bool bInvalidUAsset = !AssetData.IsValid() && AssetData.IsUAsset();
		if (bInvalidUAsset)
		{
			AssetClass = TEXT("");
		}
		
		UClass* Class = FindObjectSafe<UClass>(nullptr, *AssetClass);

		if ( Class )
		{
			return GetDisplayNameForClass( Class, &AssetData );
		}

		if (bInvalidUAsset)
		{
			return FText::FromString(AssetData.GetInvalidReason()); 
		}
		else
		{
			return FText::FromString(AssetClass);
		}
	}

	FText GetAssetDisplayName() const
	{
		const FExtAssetData& ExtAssetData = AssetThumbnail->GetAssetData();

		if ( ExtAssetData.GetClass() == UClass::StaticClass() )
		{
			UClass* Class = Cast<UClass>( ExtAssetData.GetAsset() );
			return GetDisplayNameForClass( Class );
		}

		return FText::FromName(ExtAssetData.AssetName);
	}

	void OnRenderedThumbnailChanged( bool bInHasRenderedThumbnail )
	{
		bHasRenderedThumbnail = bInHasRenderedThumbnail;

		UpdateThumbnailVisibilities();
	}

	void UpdateThumbnailVisibilities()
	{
		// Either the generic label or thumbnail should be shown, but not both at once
		const EVisibility ClassThumbnailVisibility = GetClassThumbnailVisibility();
		if( GenericThumbnailImage.IsValid() )
		{
			GenericThumbnailImage->SetVisibility( ClassThumbnailVisibility );
		}
		if( GenericLabelTextBlock.IsValid() )
		{
			GenericLabelTextBlock->SetVisibility( (ClassThumbnailVisibility == EVisibility::Visible) ? EVisibility::Collapsed : EVisibility::Visible );
		}

		const EVisibility ViewportVisibility = GetViewportVisibility();
		if( RenderedThumbnailWidget.IsValid() )
		{
			RenderedThumbnailWidget->SetVisibility( ViewportVisibility );
			if( ClassIconWidget.IsValid() )
			{
				ClassIconWidget->SetVisibility( ViewportVisibility );
			}
		}
	}


	TOptional<FVector2D> GetGenericThumbnailDesiredSize() const
	{
		const int32 Size = GenericThumbnailSize.Get();

		return FVector2D(Size, Size);
	}

private:
	TSharedPtr<STextBlock> GenericLabelTextBlock;
	TSharedPtr<STextBlock> HintTextBlock;
	TSharedPtr<SImage> GenericThumbnailImage;
	TSharedPtr<SBorder> ClassIconWidget;
	TSharedPtr<SBorder> RenderedThumbnailWidget;
	TSharedPtr<SBorder> AssetBackgroundWidget;
	TSharedPtr<SBorder> AssetColorStripWidget;
	TSharedPtr<FExtAssetThumbnail> AssetThumbnail;
	FCurveSequence ViewportFadeAnimation;
	FCurveHandle ViewportFadeCurve;

	FLinearColor AssetColor;
	TOptional<FLinearColor> AssetTypeColorOverride;
	float WidthLastFrame;
	float GenericThumbnailBorderPadding;
	bool bHasRenderedThumbnail;
	FName Style;
	TAttribute< FText > HighlightedText;
	EThumbnailLabel::Type Label;

	TAttribute< FLinearColor > HintColorAndOpacity;
	TAttribute<int32> GenericThumbnailSize;
	bool bAllowHintText;

	/** The name of the thumbnail which should be used instead of the class thumbnail. */
	FName ClassThumbnailBrushOverride;

	FName AssetBackgroundBrushName;
	FName ClassBackgroundBrushName;

	const FSlateBrush* ThumbnailBrush;

	const FSlateBrush* ClassIconBrush;

	/** The class to use when finding the thumbnail. */
	TWeakObjectPtr<UClass> ThumbnailClass;
	/** Are we showing a class type? (UClass, UBlueprint) */
	bool bIsClassType;
};

//-------------------------------------------------------------------------------------
// FExtAssetThumbnail implementation
//

FExtAssetThumbnail::FExtAssetThumbnail( UObject* InAsset, uint32 InWidth, uint32 InHeight, const TSharedPtr<class FExtAssetThumbnailPool>& InThumbnailPool )
	: ThumbnailPool(InThumbnailPool)
	, AssetData(InAsset ? FExtAssetData(InAsset) : FExtAssetData())
	, Width( InWidth )
	, Height( InHeight )
{
	if ( InThumbnailPool.IsValid() )
	{
		InThumbnailPool->AddReferencer(*this);
	}
}

FExtAssetThumbnail::FExtAssetThumbnail( const FExtAssetData& InAssetData , uint32 InWidth, uint32 InHeight, const TSharedPtr<class FExtAssetThumbnailPool>& InThumbnailPool )
	: ThumbnailPool( InThumbnailPool )
	, AssetData ( InAssetData )
	, Width( InWidth )
	, Height( InHeight )
{
	if ( InThumbnailPool.IsValid() )
	{
		InThumbnailPool->AddReferencer(*this);
	}
}

FExtAssetThumbnail::~FExtAssetThumbnail()
{
	if ( ThumbnailPool.IsValid() )
	{
		ThumbnailPool.Pin()->RemoveReferencer(*this);
	}
}

FIntPoint FExtAssetThumbnail::GetSize() const
{
	return FIntPoint( Width, Height );
}

FSlateShaderResource* FExtAssetThumbnail::GetViewportRenderTargetTexture() const
{
	FSlateTexture2DRHIRef* Texture = NULL;
	if ( ThumbnailPool.IsValid() )
	{
		Texture = ThumbnailPool.Pin()->AccessTexture( AssetData, Width, Height );
	}
	if( !Texture || !Texture->IsValid() )
	{
		return NULL;
	}

	return Texture;
}

UObject* FExtAssetThumbnail::GetAsset() const
{
	if ( AssetData.ObjectPath != NAME_None )
	{
		return FindObject<UObject>(NULL, *AssetData.ObjectPath.ToString());
	}
	else
	{
		return NULL;
	}
}

const FExtAssetData& FExtAssetThumbnail::GetAssetData() const
{
	return AssetData;
}

void FExtAssetThumbnail::SetAsset( const UObject* InAsset )
{
	SetAsset( FExtAssetData(InAsset) );
}

void FExtAssetThumbnail::SetAsset( const FExtAssetData& InAssetData )
{
	if ( ThumbnailPool.IsValid() )
	{
		ThumbnailPool.Pin()->RemoveReferencer(*this);
	}

	// if ( InAssetData.IsValid() ) // Even Invalid Asset can provide value information
	{
		AssetData = InAssetData;
		if ( ThumbnailPool.IsValid() )
		{
			ThumbnailPool.Pin()->AddReferencer(*this);
		}
	}
// 	else
// 	{
// 		AssetData = FExtAssetData();
// 	}

	AssetDataChangedEvent.Broadcast();
}

TSharedRef<SWidget> FExtAssetThumbnail::MakeThumbnailWidget( const FExtAssetThumbnailConfig& InConfig )
{
	return
		SNew(SExtAssetThumbnail)
		.AssetThumbnail( SharedThis(this) )
		.ThumbnailPool( ThumbnailPool.Pin() )
		.AllowFadeIn( InConfig.bAllowFadeIn )
		.ForceGenericThumbnail( InConfig.bForceGenericThumbnail )
		.Label( InConfig.ThumbnailLabel )
		.HighlightedText( InConfig.HighlightedText )
		.HintColorAndOpacity( InConfig.HintColorAndOpacity )
		.AllowHintText( InConfig.bAllowHintText )
		.ClassThumbnailBrushOverride( InConfig.ClassThumbnailBrushOverride )
		.AllowAssetSpecificThumbnailOverlay( InConfig.bAllowAssetSpecificThumbnailOverlay )
		.AssetTypeColorOverride( InConfig.AssetTypeColorOverride )
		.Padding(InConfig.Padding)
		.GenericThumbnailSize(InConfig.GenericThumbnailSize);
}

void FExtAssetThumbnail::RefreshThumbnail()
{
	if ( ThumbnailPool.IsValid() && /*AssetData.IsValid()*/AssetData.HasThumbnail() )
	{
		ThumbnailPool.Pin()->RefreshThumbnail( SharedThis(this) );
	}
}

TSharedPtr<SWidget> FExtAssetThumbnail::GetSpecificThumbnailOverlay() const
{
	TSharedRef<SOverlay> ThumbnailOverlayWidget = SNew(SOverlay);

	const bool bDisplayEngineVersionOverlay = GetDefault<UExtContentBrowserSettings>()->DisplayEngineVersionOverlay;
	if (bDisplayEngineVersionOverlay)
	{
		//FText OverlayText = FText::FromString(AssetData.GetSavedEngineVersionForDisplay());

		ThumbnailOverlayWidget->AddSlot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetNoBrush())
			//.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FExtAssetThumbnailOverlayManager::GetSpecificThumbnailOverlayVisibility, InAssetData.AssetClass)))
			.Padding(FMargin(1.0f, 1.0f, 1.0f, 1.0f))
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				//.Text(OverlayText)
				.Text_Lambda([this]()
				{
					return FText::FromString(this->GetAssetData().GetSavedEngineVersionForDisplay());
				})
				.TextStyle(FExtContentBrowserStyle::Get(), "UAssetBrowser.AssetThumbnail.EngineOverlay")
				.Justification(ETextJustify::Center)
			]
		];
	}

#if ECB_FEA_VALIDATE_OVERLAY
	const bool bDisplayValidationOverlay = GetDefault<UExtContentBrowserSettings>()->DisplayValidationStatusOverlay;
	if (bDisplayValidationOverlay)
	{
		ThumbnailOverlayWidget->AddSlot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetNoBrush())
			.Padding(FMargin(2.0f, 1.0f, 1.0f, 8.0f))
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SNew(SImage)
				.Image(this, &FExtAssetThumbnail::GetValidationIconBrush)
			]
		];
	}
#endif

#if ECB_WIP_CONTENT_TYPE_OVERLAY
	const bool bDisplayContentTypeOverlay = GetDefault<UExtContentBrowserSettings>()->DisplayContentTypeOverlay;
	if (bDisplayContentTypeOverlay)
	{
		ThumbnailOverlayWidget->AddSlot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility(this, &FExtAssetThumbnail::GetContentTypeOverlayVisibility)
				.Padding(FMargin(1.0f, 1.0f, 1.0f, 1.0f))
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					//.Text(OverlayText)
					.Text_Lambda([this]()
					{
						return FText::FromString(this->GetAssetData().GetAssetContentTypeForDisplay());
					})
					.TextStyle(FExtContentBrowserStyle::Get(), "UAssetBrowser.AssetThumbnail.EngineOverlay")
					.Justification(ETextJustify::Center)
				]
			];
	}
#endif

	return ThumbnailOverlayWidget;

}

const FSlateBrush* FExtAssetThumbnail::GetValidationIconBrush() const
{
#if ECB_FEA_VALIDATE_OVERLAY

	if (!AssetData.IsValid())
	{
		return FExtContentBrowserStyle::Get().GetBrush("UAssetBrowser.ValidationInValid");
	}

	if (const FExtAssetDependencyInfo* DependencyInfoPtr = FExtContentBrowserSingleton::GetAssetRegistry().GetCachedAssetDependencyInfo(AssetData))
	{
		const FExtAssetDependencyInfo& DependencyInfo = *DependencyInfoPtr;

		if (DependencyInfo.AssetStatus == EDependencyNodeStatus::Invalid || DependencyInfo.AssetStatus == EDependencyNodeStatus::Missing)
		{
			return FExtContentBrowserStyle::Get().GetBrush("UAssetBrowser.ValidationInValid");
		}
		else if (DependencyInfo.AssetStatus == EDependencyNodeStatus::Valid)
		{
			return FExtContentBrowserStyle::Get().GetBrush("UAssetBrowser.ValidationValid");
		}
		else if (DependencyInfo.AssetStatus == EDependencyNodeStatus::ValidWithSoftReferenceIssue)
		{
			return FExtContentBrowserStyle::Get().GetBrush("UAssetBrowser.ValidationIssue");
		}
		else
		{
			return FExtContentBrowserStyle::Get().GetBrush("UAssetBrowser.ValidationUknown");
		}
	}

	return FExtContentBrowserStyle::Get().GetBrush("UAssetBrowser.ValidationUknown");
	
#endif
	return nullptr;
}

EVisibility FExtAssetThumbnail::GetContentTypeOverlayVisibility() const
{
	//return EVisibility::Collapsed;
	return EVisibility::Visible;
}

//-------------------------------------------------------------------------------------
// FExtAssetThumbnailPool implementation
//

FExtAssetThumbnailPool::FExtAssetThumbnailPool( uint32 InNumInPool, const TAttribute<bool>& InAreRealTimeThumbnailsAllowed, double InMaxFrameTimeAllowance, uint32 InMaxRealTimeThumbnailsPerFrame )
	: AreRealTimeThumbnailsAllowed( InAreRealTimeThumbnailsAllowed )
	, NumInPool( InNumInPool )
	, MaxRealTimeThumbnailsPerFrame( InMaxRealTimeThumbnailsPerFrame )
	, MaxFrameTimeAllowance( InMaxFrameTimeAllowance )
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FExtAssetThumbnailPool::OnObjectPropertyChanged);
	FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &FExtAssetThumbnailPool::OnAssetLoaded);
	if ( GEditor )
	{
		GEditor->OnActorMoved().AddRaw( this, &FExtAssetThumbnailPool::OnActorPostEditMove );
	}
}

FExtAssetThumbnailPool::~FExtAssetThumbnailPool()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	FCoreUObjectDelegates::OnAssetLoaded.RemoveAll(this);
	if ( GEditor )
	{
		GEditor->OnActorMoved().RemoveAll(this);
	}

	// Release all the texture resources
	ReleaseResources();
}

FExtAssetThumbnailPool::FThumbnailInfo::~FThumbnailInfo()
{
	if( ThumbnailTexture )
	{
		delete ThumbnailTexture;
		ThumbnailTexture = NULL;
	}

	if( ThumbnailRenderTarget )
	{
		delete ThumbnailRenderTarget;
		ThumbnailRenderTarget = NULL;
	}
}

void FExtAssetThumbnailPool::ReleaseResources()
{
	// Clear all pending render requests
	ThumbnailsToRenderStack.Empty();
	RealTimeThumbnails.Empty();
	RealTimeThumbnailsToRender.Empty();

	TArray< TSharedRef<FThumbnailInfo> > ThumbnailsToRelease;

	for( auto ThumbIt = ThumbnailToTextureMap.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		ThumbnailsToRelease.Add(ThumbIt.Value());
	}
	ThumbnailToTextureMap.Empty();

	for( auto ThumbIt = FreeThumbnails.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		ThumbnailsToRelease.Add(*ThumbIt);
	}
	FreeThumbnails.Empty();

	for ( auto ThumbIt = ThumbnailsToRelease.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		const TSharedRef<FThumbnailInfo>& Thumb = *ThumbIt;

			// Release rendering resources
			FThumbnailInfo_RenderThread ThumbInfo = Thumb.Get();
			ENQUEUE_RENDER_COMMAND(ReleaseThumbnailResources)(
				[ThumbInfo](FRHICommandListImmediate& RHICmdList)
				{
					ThumbInfo.ThumbnailTexture->ClearTextureData();
					ThumbInfo.ThumbnailTexture->ReleaseResource();
					ThumbInfo.ThumbnailRenderTarget->ReleaseResource();
				});
		}

	// Wait for all resources to be released
	FlushRenderingCommands();

	// Make sure there are no more references to any of our thumbnails now that rendering commands have been flushed
	for ( auto ThumbIt = ThumbnailsToRelease.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		const TSharedRef<FThumbnailInfo>& Thumb = *ThumbIt;
		if ( !Thumb.IsUnique() )
		{
			ensureMsgf(0, TEXT("Thumbnail info for '%s' is still referenced by '%d' other objects"), *Thumb->AssetData.ObjectPath.ToString(), Thumb.GetSharedReferenceCount());
		}
	}
}

TStatId FExtAssetThumbnailPool::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT( FExtAssetThumbnailPool, STATGROUP_Tickables );
}

bool FExtAssetThumbnailPool::IsTickable() const
{
	return RecentlyLoadedAssets.Num() > 0 || ThumbnailsToRenderStack.Num() > 0 || RealTimeThumbnails.Num() > 0 || bWereShadersCompilingLastFrame || (GShaderCompilingManager && GShaderCompilingManager->IsCompiling());
}

void FExtAssetThumbnailPool::Tick( float DeltaTime )
{
	// If throttling do not tick unless drag dropping which could have a thumbnail as the cursor decorator
	if (!FSlateApplication::Get().IsDragDropping() && !FSlateThrottleManager::Get().IsAllowingExpensiveTasks() && !FSlateApplication::Get().AnyMenusVisible())
	{
		return;
	}

	const bool bAreShadersCompiling = (GShaderCompilingManager && GShaderCompilingManager->IsCompiling());
	if (bWereShadersCompilingLastFrame && !bAreShadersCompiling)
	{
		ThumbnailsToRenderStack.Reset();
		// Reschedule visible thumbnails to be rerendered now that shaders are finished compiling
		for (auto ThumbIt = ThumbnailToTextureMap.CreateIterator(); ThumbIt; ++ThumbIt)
		{
			ThumbnailsToRenderStack.Push(ThumbIt.Value());
		}
	}
	bWereShadersCompilingLastFrame = bAreShadersCompiling;

	// If there were any assets loaded since last frame that we are currently displaying thumbnails for, push them on the render stack now.
	if ( RecentlyLoadedAssets.Num() > 0 )
	{
		for ( int32 LoadedAssetIdx = 0; LoadedAssetIdx < RecentlyLoadedAssets.Num(); ++LoadedAssetIdx )
		{
			RefreshThumbnailsFor(RecentlyLoadedAssets[LoadedAssetIdx]);
		}

		RecentlyLoadedAssets.Empty();
	}

	// If we have dynamic thumbnails are we are done rendering the last batch of dynamic thumbnails, start a new batch as long as real-time thumbnails are enabled
	const bool bIsInPIEOrSimulate = GEditor->PlayWorld != NULL || GEditor->bIsSimulatingInEditor;
#if ECB_LEGACY
	const bool bShouldUseRealtimeThumbnails = AreRealTimeThumbnailsAllowed.Get() && GetDefault<UExtContentBrowserSettings>()->RealTimeThumbnails && !bIsInPIEOrSimulate;
#else
	const bool bShouldUseRealtimeThumbnails = false;
#endif
	if ( bShouldUseRealtimeThumbnails && RealTimeThumbnails.Num() > 0 && RealTimeThumbnailsToRender.Num() == 0 )
	{
		double CurrentTime = FPlatformTime::Seconds();
		for ( int32 ThumbIdx = RealTimeThumbnails.Num() - 1; ThumbIdx >= 0; --ThumbIdx )
		{
			const TSharedRef<FThumbnailInfo>& Thumb = RealTimeThumbnails[ThumbIdx];
			if ( Thumb->AssetData.IsAssetLoaded() )
			{
				// Only render thumbnails that have been requested recently
				if ( (CurrentTime - Thumb->LastAccessTime) < 1.f )
				{
					RealTimeThumbnailsToRender.Add(Thumb);
				}
			}
			else
			{
				RealTimeThumbnails.RemoveAt(ThumbIdx);
			}
		}
	}

	uint32 NumRealTimeThumbnailsRenderedThisFrame = 0;
	// If there are any thumbnails to render, pop one off the stack and render it.
	if( ThumbnailsToRenderStack.Num() + RealTimeThumbnailsToRender.Num() > 0 )
	{
		double FrameStartTime = FPlatformTime::Seconds();
		// Render as many thumbnails as we are allowed to
		while( ThumbnailsToRenderStack.Num() + RealTimeThumbnailsToRender.Num() > 0 && FPlatformTime::Seconds() - FrameStartTime < MaxFrameTimeAllowance )
		{
			TSharedPtr<FThumbnailInfo> Info;
			if ( ThumbnailsToRenderStack.Num() > 0 )
			{
				Info = ThumbnailsToRenderStack.Pop();
			}
			else if (RealTimeThumbnailsToRender.Num() > 0 && NumRealTimeThumbnailsRenderedThisFrame < MaxRealTimeThumbnailsPerFrame )
			{
				Info = RealTimeThumbnailsToRender.Pop();
				NumRealTimeThumbnailsRenderedThisFrame++;
			}
			else
			{
				// No thumbnails left to render or we don't want to render any more
				break;
			}

			if( Info.IsValid() )
			{
				TSharedRef<FThumbnailInfo> InfoRef = Info.ToSharedRef();

				/*if ( InfoRef->AssetData.IsValid() )*/
				if (InfoRef->AssetData.HasThumbnail())
				{
					bool bLoadedThumbnail = false;

					// If this is a loaded asset and we have a rendering info for it, render a fresh thumbnail here
					if( InfoRef->AssetData.IsAssetLoaded() )
					{
						UObject* Asset = InfoRef->AssetData.GetAsset();
						FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Asset );
						if ( RenderInfo != NULL && RenderInfo->Renderer != NULL )
						{
							FThumbnailInfo_RenderThread ThumbInfo = InfoRef.Get();
							ENQUEUE_RENDER_COMMAND(SyncSlateTextureCommand)(
								[ThumbInfo](FRHICommandListImmediate& RHICmdList)
								{
									if ( ThumbInfo.ThumbnailTexture->GetTypedResource() != ThumbInfo.ThumbnailRenderTarget->GetTextureRHI() )
									{
										ThumbInfo.ThumbnailTexture->ClearTextureData();
										ThumbInfo.ThumbnailTexture->ReleaseDynamicRHI();
										ThumbInfo.ThumbnailTexture->SetRHIRef(ThumbInfo.ThumbnailRenderTarget->GetTextureRHI(), ThumbInfo.Width, ThumbInfo.Height);
									}
								});

							if (InfoRef->LastUpdateTime <= 0.0f/* || RenderInfo->Renderer->AllowsRealtimeThumbnails(Asset)*/)
							{
								//@todo: this should be done on the GPU only but it is not supported by thumbnail tools yet
								ThumbnailTools::RenderThumbnail(
									Asset,
									InfoRef->Width,
									InfoRef->Height,
									ThumbnailTools::EThumbnailTextureFlushMode::NeverFlush,
									InfoRef->ThumbnailRenderTarget
									);
							}

							bLoadedThumbnail = true;

							// Since this was rendered, add it to the list of thumbnails that can be rendered in real-time
							RealTimeThumbnails.AddUnique(InfoRef);
						}
					}
				
					const FObjectThumbnail* ObjectThumbnailPtr = NULL;
					FObjectThumbnail ObjectThumbnail;
					// If we could not render a fresh thumbnail, see if we already have a cached one to load
					if ( !bLoadedThumbnail )
					{
						// Unloaded asset

						const bool bUseThumbnailPool = GetDefault<UExtContentBrowserSettings>()->bUseThumbnailPool;

#if ECB_WIP_THUMB_CACHE
						const FObjectThumbnail* FoundThumbnail = FExtContentBrowserSingleton::GetAssetRegistry().FindCachedThumbnail(InfoRef->AssetData.PackageFilePath);
#else

	#if ECB_WIP_OBJECT_THUMB_POOL
						const FObjectThumbnail* FoundThumbnail = bUseThumbnailPool 
							? FExtContentBrowserSingleton::GetThumbnailPool().FindCachedThumbnail(InfoRef->AssetData.GetFullName())
							: ThumbnailTools::FindCachedThumbnail(InfoRef->AssetData.GetFullName());
	#else
						const FObjectThumbnail* FoundThumbnail = ThumbnailTools::FindCachedThumbnail(InfoRef->AssetData.GetFullName());
	#endif
#endif
						if ( FoundThumbnail )
						{
							//ECB_LOG(Display, TEXT("Found cached thumbnail for %s"), *InfoRef->AssetData.PackageFilePath.ToString());
							ObjectThumbnailPtr = FoundThumbnail;
						}
						else
						{
#if ECB_WIP_OBJECT_THUMB_POOL
							FObjectThumbnail& ObjectThumbnailRef = bUseThumbnailPool 
								? FExtContentBrowserSingleton::GetThumbnailPool().AddToPool(InfoRef->AssetData.GetFullName())
								: ObjectThumbnail;

							if (InfoRef->AssetData.LoadThumbnail(ObjectThumbnailRef))
							{
								ObjectThumbnailPtr = &ObjectThumbnailRef;
							}
#else
							// If we don't have a cached thumbnail, try to find it on disk
							if (InfoRef->AssetData.LoadThumbnail(ObjectThumbnail))
							{
								ObjectThumbnailPtr = &ObjectThumbnail;
							}
#endif
						}
					}

					if ( ObjectThumbnailPtr )
					{
						if ( ObjectThumbnailPtr->GetImageWidth() > 0 && ObjectThumbnailPtr->GetImageHeight() > 0 && ObjectThumbnailPtr->GetUncompressedImageData().Num() > 0 )
						{
							// Make bulk data for updating the texture memory later
							FSlateTextureData* BulkData = new FSlateTextureData(ObjectThumbnailPtr->GetImageWidth(),ObjectThumbnailPtr->GetImageHeight(),GPixelFormats[PF_B8G8R8A8].BlockBytes, ObjectThumbnailPtr->AccessImageData() );

							// Update the texture RHI
							FThumbnailInfo_RenderThread ThumbInfo = InfoRef.Get();
							ENQUEUE_RENDER_COMMAND(ClearSlateTextureCommand)(
								[ThumbInfo, BulkData](FRHICommandListImmediate& RHICmdList)
							{
								if (ThumbInfo.ThumbnailTexture->GetTypedResource() == ThumbInfo.ThumbnailRenderTarget->GetTextureRHI())
								{
									ThumbInfo.ThumbnailTexture->SetRHIRef(NULL, ThumbInfo.Width, ThumbInfo.Height);
								}

								ThumbInfo.ThumbnailTexture->SetTextureData(MakeShareable(BulkData));
								ThumbInfo.ThumbnailTexture->UpdateRHI();
							});

							bLoadedThumbnail = true;
						}
						else
						{
							bLoadedThumbnail = false;
						}
					}

					if ( bLoadedThumbnail )
					{
						// Mark it as updated
						InfoRef->LastUpdateTime = FPlatformTime::Seconds();

						// Notify listeners that a thumbnail has been rendered
						ThumbnailRenderedEvent.Broadcast(InfoRef->AssetData);
					}
					else
					{
						// Notify listeners that a thumbnail has been rendered
						ThumbnailRenderFailedEvent.Broadcast(InfoRef->AssetData);
					}
				}
			}
		}
	}
}

FSlateTexture2DRHIRef* FExtAssetThumbnailPool::AccessTexture( const FExtAssetData& AssetData, uint32 Width, uint32 Height )
{
	if(AssetData.ObjectPath == NAME_None || Width == 0 || Height == 0)
	{
		return NULL;
	}
	else
	{
		FThumbId ThumbId( AssetData.ObjectPath, Width, Height ) ;
		// Check to see if a thumbnail for this asset exists.  If so we don't need to render it
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find( ThumbId );
		TSharedPtr<FThumbnailInfo> ThumbnailInfo;
		if( ThumbnailInfoPtr )
		{
			ThumbnailInfo = *ThumbnailInfoPtr;
		}
		else
		{
			// If a the max number of thumbnails allowed by the pool exists then reuse its rendering resource for the new thumbnail
			if( FreeThumbnails.Num() == 0 && ThumbnailToTextureMap.Num() == NumInPool )
			{
				// Find the thumbnail which was accessed last and use it for the new thumbnail
				float LastAccessTime = FLT_MAX;
				const FThumbId* AssetToRemove = NULL;
				for( TMap< FThumbId, TSharedRef<FThumbnailInfo> >::TConstIterator It(ThumbnailToTextureMap); It; ++It )
				{
					if( It.Value()->LastAccessTime < LastAccessTime )
					{
						LastAccessTime = It.Value()->LastAccessTime;
						AssetToRemove = &It.Key();
					}
				}

				check( AssetToRemove );

				// Remove the old mapping 
				ThumbnailInfo = ThumbnailToTextureMap.FindAndRemoveChecked( *AssetToRemove );
			}
			else if( FreeThumbnails.Num() > 0 )
			{
				ThumbnailInfo = FreeThumbnails.Pop();

				FSlateTextureRenderTarget2DResource* ThumbnailRenderTarget = ThumbnailInfo->ThumbnailRenderTarget;
				ENQUEUE_RENDER_COMMAND(SlateUpdateThumbSizeCommand)(
					[ThumbnailRenderTarget, Width, Height](FRHICommandListImmediate& RHICmdList)
					{
						ThumbnailRenderTarget->SetSize(Width, Height);
					});
			}
			else
			{
				// There are no free thumbnail resources
				check( (uint32)ThumbnailToTextureMap.Num() <= NumInPool );
				check( !ThumbnailInfo.IsValid() );
				// The pool isn't used up so just make a new texture 

				// Make new thumbnail info if it doesn't exist
				// This happens when the pool is not yet full
				ThumbnailInfo = MakeShareable( new FThumbnailInfo );
				
				// Set the thumbnail and asset on the info. It is NOT safe to change or NULL these pointers until ReleaseResources.
				ThumbnailInfo->ThumbnailTexture = new FSlateTexture2DRHIRef( Width, Height, PF_B8G8R8A8, NULL, TexCreate_Dynamic );
				ThumbnailInfo->ThumbnailRenderTarget = new FSlateTextureRenderTarget2DResource(FLinearColor::Black, Width, Height, PF_B8G8R8A8, SF_Point, TA_Wrap, TA_Wrap, 0.0f);

				BeginInitResource( ThumbnailInfo->ThumbnailTexture );
				BeginInitResource( ThumbnailInfo->ThumbnailRenderTarget );
			}


			check( ThumbnailInfo.IsValid() );
			TSharedRef<FThumbnailInfo> ThumbnailRef = ThumbnailInfo.ToSharedRef();

			// Map the object to its thumbnail info
			ThumbnailToTextureMap.Add( ThumbId, ThumbnailRef );

			ThumbnailInfo->AssetData = AssetData;
			ThumbnailInfo->Width = Width;
			ThumbnailInfo->Height = Height;
		
			// Mark this thumbnail as needing to be updated
			ThumbnailInfo->LastUpdateTime = -1.f;

			// Request that the thumbnail be rendered as soon as possible
			ThumbnailsToRenderStack.Push( ThumbnailRef );
		}

		// This thumbnail was accessed, update its last time to the current time
		// We'll use LastAccessTime to determine the order to recycle thumbnails if the pool is full
		ThumbnailInfo->LastAccessTime = FPlatformTime::Seconds();

		return ThumbnailInfo->ThumbnailTexture;
	}
}

void FExtAssetThumbnailPool::AddReferencer( const FExtAssetThumbnail& AssetThumbnail )
{
	FIntPoint Size = AssetThumbnail.GetSize();
	if ( AssetThumbnail.GetAssetData().ObjectPath == NAME_None || Size.X == 0 || Size.Y == 0 )
	{
		// Invalid referencer
		return;
	}

	// Generate a key and look up the number of references in the RefCountMap
	FThumbId ThumbId( AssetThumbnail.GetAssetData().ObjectPath, Size.X, Size.Y ) ;
	int32* RefCountPtr = RefCountMap.Find(ThumbId);

	if ( RefCountPtr )
	{
		// Already in the map, increment a reference
		(*RefCountPtr)++;
	}
	else
	{
		// New referencer, add it to the map with a RefCount of 1
		RefCountMap.Add(ThumbId, 1);
	}
}

void FExtAssetThumbnailPool::RemoveReferencer( const FExtAssetThumbnail& AssetThumbnail )
{
	FIntPoint Size = AssetThumbnail.GetSize();
	const FName ObjectPath = AssetThumbnail.GetAssetData().ObjectPath;
	if ( ObjectPath == NAME_None || Size.X == 0 || Size.Y == 0 )
	{
		// Invalid referencer
		return;
	}

	// Generate a key and look up the number of references in the RefCountMap
	FThumbId ThumbId( ObjectPath, Size.X, Size.Y ) ;
	int32* RefCountPtr = RefCountMap.Find(ThumbId);

	// This should complement an AddReferencer so this entry should be in the map
	if ( RefCountPtr )
	{
		// Decrement the RefCount
		(*RefCountPtr)--;

		// If we reached zero, free the thumbnail and remove it from the map.
		if ( (*RefCountPtr) <= 0 )
		{
			RefCountMap.Remove(ThumbId);
			FreeThumbnail(ObjectPath, Size.X, Size.Y);
		}
	}
	else
	{
		// This FExtAssetThumbnail did not reference anything or was deleted after the pool was deleted.
	}
}

bool FExtAssetThumbnailPool::IsInRenderStack( const TSharedPtr<FExtAssetThumbnail>& Thumbnail ) const
{
	const FExtAssetData& AssetData = Thumbnail->GetAssetData();
	const uint32 Width = Thumbnail->GetSize().X;
	const uint32 Height = Thumbnail->GetSize().Y;

	if ( ensure(AssetData.ObjectPath != NAME_None) && ensure(Width > 0) && ensure(Height > 0) )
	{
		FThumbId ThumbId( AssetData.ObjectPath, Width, Height ) ;
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find( ThumbId );
		if ( ThumbnailInfoPtr )
		{
			return ThumbnailsToRenderStack.Contains(*ThumbnailInfoPtr);
		}
	}

	return false;
}

bool FExtAssetThumbnailPool::IsRendered(const TSharedPtr<FExtAssetThumbnail>& Thumbnail) const
{
	const FExtAssetData& AssetData = Thumbnail->GetAssetData();
	const uint32 Width = Thumbnail->GetSize().X;
	const uint32 Height = Thumbnail->GetSize().Y;

	if (ensure(AssetData.ObjectPath != NAME_None) && ensure(Width > 0) && ensure(Height > 0))
	{
		FThumbId ThumbId(AssetData.ObjectPath, Width, Height);
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find(ThumbId);
		if (ThumbnailInfoPtr)
		{
			return (*ThumbnailInfoPtr).Get().LastUpdateTime >= 0.f;
		}
	}

	return false;
}

void FExtAssetThumbnailPool::PrioritizeThumbnails( const TArray< TSharedPtr<FExtAssetThumbnail> >& ThumbnailsToPrioritize, uint32 Width, uint32 Height )
{
	if ( ensure(Width > 0) && ensure(Height > 0) )
	{
		TSet<FName> ObjectPathList;
		for ( int32 ThumbIdx = 0; ThumbIdx < ThumbnailsToPrioritize.Num(); ++ThumbIdx )
		{
			ObjectPathList.Add(ThumbnailsToPrioritize[ThumbIdx]->GetAssetData().ObjectPath);
		}

		TArray< TSharedRef<FThumbnailInfo> > FoundThumbnails;
		for ( int32 ThumbIdx = ThumbnailsToRenderStack.Num() - 1; ThumbIdx >= 0; --ThumbIdx )
		{
			const TSharedRef<FThumbnailInfo>& ThumbnailInfo = ThumbnailsToRenderStack[ThumbIdx];
			if ( ThumbnailInfo->Width == Width && ThumbnailInfo->Height == Height && ObjectPathList.Contains(ThumbnailInfo->AssetData.ObjectPath) )
			{
				FoundThumbnails.Add(ThumbnailInfo);
				ThumbnailsToRenderStack.RemoveAt(ThumbIdx);
			}
		}

		for ( int32 ThumbIdx = 0; ThumbIdx < FoundThumbnails.Num(); ++ThumbIdx )
		{
			ThumbnailsToRenderStack.Push(FoundThumbnails[ThumbIdx]);
		}
	}
}

void FExtAssetThumbnailPool::RefreshThumbnail( const TSharedPtr<FExtAssetThumbnail>& ThumbnailToRefresh )
{
	const FExtAssetData& AssetData = ThumbnailToRefresh->GetAssetData();
	const uint32 Width = ThumbnailToRefresh->GetSize().X;
	const uint32 Height = ThumbnailToRefresh->GetSize().Y;

	if ( ensure(AssetData.ObjectPath != NAME_None) && ensure(Width > 0) && ensure(Height > 0) )
	{
		FThumbId ThumbId( AssetData.ObjectPath, Width, Height ) ;
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find( ThumbId );
		if ( ThumbnailInfoPtr )
		{
			ThumbnailsToRenderStack.AddUnique(*ThumbnailInfoPtr);
		}
	}
}

void FExtAssetThumbnailPool::FreeThumbnail( const FName& ObjectPath, uint32 Width, uint32 Height )
{
	if(ObjectPath != NAME_None && Width != 0 && Height != 0)
	{
		FThumbId ThumbId( ObjectPath, Width, Height ) ;

		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find(ThumbId);
		if( ThumbnailInfoPtr )
		{
			TSharedRef<FThumbnailInfo> ThumbnailInfo = *ThumbnailInfoPtr;
			ThumbnailToTextureMap.Remove(ThumbId);
			ThumbnailsToRenderStack.Remove(ThumbnailInfo);
			RealTimeThumbnails.Remove(ThumbnailInfo);
			RealTimeThumbnailsToRender.Remove(ThumbnailInfo);

			FSlateTexture2DRHIRef* ThumbnailTexture = ThumbnailInfo->ThumbnailTexture;
			ENQUEUE_RENDER_COMMAND(ReleaseThumbnailTextureData)(
				[ThumbnailTexture](FRHICommandListImmediate& RHICmdList)
				{
					ThumbnailTexture->ClearTextureData();
				});

			FreeThumbnails.Add( ThumbnailInfo );
		}
	}
			
}

void FExtAssetThumbnailPool::RefreshThumbnailsFor( FName ObjectPath )
{
	for ( auto ThumbIt = ThumbnailToTextureMap.CreateIterator(); ThumbIt; ++ThumbIt)
	{
		if ( ThumbIt.Key().ObjectPath == ObjectPath )
		{
			ThumbnailsToRenderStack.Push( ThumbIt.Value() );
		}
	}
}

void FExtAssetThumbnailPool::OnAssetLoaded( UObject* Asset )
{
	if ( Asset != NULL )
	{
		RecentlyLoadedAssets.Add( FName(*Asset->GetPathName()) );
	}
}

void FExtAssetThumbnailPool::OnActorPostEditMove( AActor* Actor )
{
	DirtyThumbnailForObject(Actor);
}

void FExtAssetThumbnailPool::OnObjectPropertyChanged( UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent )
{
	DirtyThumbnailForObject(ObjectBeingModified);
}

void FExtAssetThumbnailPool::DirtyThumbnailForObject(UObject* ObjectBeingModified)
{
	if (!ObjectBeingModified)
	{
		return;
	}

	if (ObjectBeingModified->HasAnyFlags(RF_ClassDefaultObject))
	{
		if (ObjectBeingModified->GetClass()->ClassGeneratedBy != NULL)
		{
			// This is a blueprint modification. Check to see if this thumbnail is the blueprint of the modified CDO
			ObjectBeingModified = ObjectBeingModified->GetClass()->ClassGeneratedBy;
		}
	}
	else if (AActor* ActorBeingModified = Cast<AActor>(ObjectBeingModified))
	{
		// This is a non CDO actor getting modified. Update the actor's world's thumbnail.
		ObjectBeingModified = ActorBeingModified->GetWorld();
	}

	if (ObjectBeingModified && ObjectBeingModified->IsAsset())
	{
		// An object in memory was modified.  We'll mark it's thumbnail as dirty so that it'll be
		// regenerated on demand later. (Before being displayed in the browser, or package saves, etc.)
		FObjectThumbnail* Thumbnail = ThumbnailTools::GetThumbnailForObject(ObjectBeingModified);

		// Don't try loading thumbnails for package that have never been saved
		if (Thumbnail == NULL && !IsGarbageCollecting() && !ObjectBeingModified->GetOutermost()->HasAnyPackageFlags(PKG_NewlyCreated))
		{
			// If we don't yet have a thumbnail map, load one from disk if possible
			// Don't attempt to do this while garbage collecting since loading or finding objects during GC is illegal
			FName ObjectFullName = FName(*ObjectBeingModified->GetFullName());
			TArray<FName> ObjectFullNames;
			FThumbnailMap LoadedThumbnails;
			ObjectFullNames.Add(ObjectFullName);
			if (ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectFullNames, LoadedThumbnails))
			{
				Thumbnail = LoadedThumbnails.Find(ObjectFullName);

				if (Thumbnail != NULL)
				{
					Thumbnail = ThumbnailTools::CacheThumbnail(ObjectBeingModified->GetFullName(), Thumbnail, ObjectBeingModified->GetOutermost());
				}
			}
		}

		if (Thumbnail != NULL)
		{
			// Mark the thumbnail as dirty
			Thumbnail->MarkAsDirty();
		}

		RefreshThumbnailsFor( FName(*ObjectBeingModified->GetPathName()) );
	}
}

/////////////////////////////////////////////////////////////////////

const FObjectThumbnail* FExtObjectThumbnailPool::FindCachedThumbnail(const FString& InFullName)
{
	FObjectThumbnail* FoundThumbnail = Pool.Find(InFullName);
	if (FoundThumbnail)
	{
		ECB_LOG(Display, TEXT("Found ThumbPool: %s"), *InFullName);
	}
	return FoundThumbnail;
}

FObjectThumbnail& FExtObjectThumbnailPool::AddToPool(const FString& InFullName)
{
	if (Pool.Num() > NumInPool)
	{
		FString FirstKey;
		for (auto ThumbIt = Pool.CreateConstIterator(); ThumbIt; ++ThumbIt)
		{
			FirstKey = ThumbIt->Key;
			break;
		}
		Pool.Remove(FirstKey);
	}

	// Todo: Free if Pool is Full

	ECB_LOG(Warning, TEXT("Addto ThumbPool: %s"), *InFullName);
	return Pool.FindOrAdd(InFullName);
}

void FExtObjectThumbnailPool::Reserve(int32 InSize)
{
	NumInPool = InSize;
	Pool.Reserve(InSize);
}

void FExtObjectThumbnailPool::Resize(int32 InSize)
{
	NumInPool = InSize;
	if (NumInPool <= 0)
	{
		Free();
		return;
	}

	if (NumInPool < Pool.Num())
	{
		int32 NumToRemove = Pool.Num() - NumInPool;
		int32 Index = 0;
		for (auto ThumbIt = Pool.CreateIterator(); ThumbIt; ++ThumbIt)
		{
			if (Index >= NumToRemove)
				break;

			ThumbIt.RemoveCurrent();

			++Index;
		}
	}
	else
	{
		Pool.Reserve(NumInPool);
	}
}

void FExtObjectThumbnailPool::Free()
{
	Pool.Empty();
}

uint32 FExtObjectThumbnailPool::GetAllocatedSize() const
{
	uint32 MapMemory = Pool.GetAllocatedSize();

	uint32 MapArrayMemory = 0;
	auto SubArray =
		[&MapArrayMemory](const auto& A)
	{
		for (auto& Pair : A)
		{
			MapArrayMemory += Pair.Value.GetCompressedDataSize();
		}
	};
	SubArray(Pool);

	return MapMemory + MapArrayMemory;
}


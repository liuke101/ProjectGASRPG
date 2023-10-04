// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtAssetViewWidgets.h"
#include "ExtAssetThumbnail.h"
#include "ExtContentBrowser.h"
#include "ExtContentBrowserSettings.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtAssetViewTypes.h"
#include "DragDropHandler.h"
#include "ExtContentBrowserUtils.h"

// Engine - Editor
#include "UObject/UnrealType.h"
#include "ObjectTools.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "EditorFramework/AssetImportData.h"
#include "Internationalization/BreakIterator.h"
#include "Misc/EngineBuildSettings.h"

// Asset
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AutoReimport/AssetSourceFilenameCache.h"
#include "DragAndDrop/AssetDragDropOp.h"

// Slate
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Images/SLayeredImage.h"

#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"

// Collection
#include "CollectionManagerTypes.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"

//
#include "ActorFolder.h"

#define LOCTEXT_NAMESPACE "ExtContentBrowser"


///////////////////////////////
// FAssetViewModeUtils
///////////////////////////////

FReply FAssetViewModeUtils::OnViewModeKeyDown( const TSet< TSharedPtr<FExtAssetViewItem> >& SelectedItems, const FKeyEvent& InKeyEvent )
{
	// All asset views use Ctrl-C to copy references to assets
	if ( InKeyEvent.IsControlDown() && InKeyEvent.GetCharacter() == 'C' 
		&& !InKeyEvent.IsShiftDown() && !InKeyEvent.IsAltDown()
		)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		TArray<FExtAssetData> SelectedAssets;
		for ( auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt)
		{
			const TSharedPtr<FExtAssetViewItem>& Item = *ItemIt;
			if(Item.IsValid())
			{
				if(Item->GetType() == EExtAssetItemType::Folder)
				{
					// we need to recurse & copy references to all folder contents
					FARFilter Filter;
					Filter.PackagePaths.Add(*StaticCastSharedPtr<FExtAssetViewFolder>(Item)->FolderPath);

					// Add assets found in the asset registry
					//AssetRegistryModule.Get().GetAssets(Filter, SelectedAssets);
				}
				else
				{
					SelectedAssets.Add(StaticCastSharedPtr<FExtAssetViewAsset>(Item)->Data);
				}
			}
		}

		//ContentBrowserUtils::CopyAssetReferencesToClipboard(SelectedAssets);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}


///////////////////////////////
// FAssetViewModeUtils
///////////////////////////////

TSharedRef<SWidget> FExtAssetViewItemHelper::CreateListItemContents(SExtAssetListItem* const InListItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	return CreateListTileItemContents(InListItem, InThumbnail, OutItemShadowBorder);
}

TSharedRef<SWidget> FExtAssetViewItemHelper::CreateTileItemContents(SExtAssetTileItem* const InTileItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	return CreateListTileItemContents(InTileItem, InThumbnail, OutItemShadowBorder);
}

template <typename T>
TSharedRef<SWidget> FExtAssetViewItemHelper::CreateListTileItemContents(T* const InTileOrListItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	TSharedRef<SOverlay> ItemContentsOverlay = SNew(SOverlay);

	OutItemShadowBorder = FName("ContentBrowser.AssetTileItem.DropShadow");

	if (InTileOrListItem->IsFolder())
	{
		// TODO: Allow items to customize their widget

		TSharedPtr<FExtAssetViewFolder> AssetFolderItem = StaticCastSharedPtr<FExtAssetViewFolder>(InTileOrListItem->AssetItem);

		const FSlateBrush* FolderBaseImage = FAppStyle::GetBrush("ContentBrowser.ListViewFolderIcon");

		// Folder base
		ItemContentsOverlay->AddSlot()
		.Padding(FMargin(5))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ContentBrowser.FolderItem.DropShadow"))
			.Padding(FMargin(0,0,2.0f,2.0f))
			[
				SNew(SImage)
				.Image(FolderBaseImage)
				.ColorAndOpacity(InTileOrListItem, &T::GetAssetColor)
			]
		];
	}
	else
	{


		// The actual thumbnail
		ItemContentsOverlay->AddSlot()
		[
			InThumbnail
		];

		// Source control state
		ItemContentsOverlay->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 3.0f, 3.0f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(InTileOrListItem, &T::GetStateIconImageSize)
			.HeightOverride(InTileOrListItem, &T::GetStateIconImageSize)
			[
				InTileOrListItem->GenerateSourceControlIconWidget()
			]
		];

		// Extra external state hook
		ItemContentsOverlay->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.MaxDesiredWidth(InTileOrListItem, &T::GetExtraStateIconMaxWidth)
			.MaxDesiredHeight(InTileOrListItem, &T::GetStateIconImageSize)
			[
				InTileOrListItem->GenerateExtraStateIconWidget(TAttribute<float>(InTileOrListItem, &T::GetExtraStateIconWidth))
			]
		];

		// Dirty state
		ItemContentsOverlay->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		[
			SNew(SBox)
			.MaxDesiredWidth(InTileOrListItem, &T::GetStateIconImageSize)
			.MaxDesiredHeight(InTileOrListItem, &T::GetStateIconImageSize)
			[
				SNew(SImage)
				.Image(InTileOrListItem, &T::GetDirtyImage)
			]
		];
	}

	return ItemContentsOverlay;
}


///////////////////////////////
// Asset view item tool tip
///////////////////////////////

class SExtAssetViewItemToolTip : public SToolTip
{
public:
	SLATE_BEGIN_ARGS(SExtAssetViewItemToolTip)
		: _AssetViewItem()
	{ }

		SLATE_ARGUMENT(TSharedPtr<SExtAssetViewItem>, AssetViewItem)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		AssetViewItem = InArgs._AssetViewItem;

		SToolTip::Construct(
			SToolTip::FArguments()
			.TextMargin(1.0f)
			.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
			);
	}

	// IToolTip interface
	virtual bool IsEmpty() const override
	{
		return !AssetViewItem.IsValid();
	}

	virtual void OnOpening() override
	{
		TSharedPtr<SExtAssetViewItem> AssetViewItemPin = AssetViewItem.Pin();
		if (AssetViewItemPin.IsValid())
		{
			SetContentWidget(AssetViewItemPin->CreateToolTipWidget());
		}
	}

	virtual void OnClosed() override
	{
		SetContentWidget(SNullWidget::NullWidget);
	}

private:
	TWeakPtr<SExtAssetViewItem> AssetViewItem;
};


///////////////////////////////
// Asset view modes
///////////////////////////////

FReply SExtAssetTileView::OnKeyDown( const FGeometry& InGeometry, const FKeyEvent& InKeyEvent )
{
	FReply Reply = FAssetViewModeUtils::OnViewModeKeyDown(SelectedItems, InKeyEvent);

	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}
	else
	{
		return STileView<TSharedPtr<FExtAssetViewItem>>::OnKeyDown(InGeometry, InKeyEvent);
	}
}

void SExtAssetTileView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Regreshing an asset view is an intensive task. Do not do this while a user
	// is dragging arround content for maximum responsiveness.
	// Also prevents a re-entrancy crash caused by potentially complex thumbnail generators.
	if (!FSlateApplication::Get().IsDragDropping())
	{
		STileView<TSharedPtr<FExtAssetViewItem>>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}
}

bool SExtAssetTileView::HasWidgetForAsset(const FName& AssetPathName)
{
	for (const TSharedPtr<FExtAssetViewItem>& Item : WidgetGenerator.ItemsWithGeneratedWidgets)
	{
		if (Item.IsValid() && Item->GetType() == EExtAssetItemType::Normal)
		{
			const FName& ObjectPath = StaticCastSharedPtr<FExtAssetViewAsset>(Item)->Data.ObjectPath;
			if (ObjectPath == AssetPathName)
			{
				return true;
			}
		}
	}

	return false;
}

FReply SExtAssetListView::OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FAssetViewModeUtils::OnViewModeKeyDown(SelectedItems, InKeyEvent);

	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}
	else
	{
		return SListView<TSharedPtr<FExtAssetViewItem>>::OnKeyDown(InGeometry, InKeyEvent);
	}
}

void SExtAssetListView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Regreshing an asset view is an intensive task. Do not do this while a user
	// is dragging arround content for maximum responsiveness.
	// Also prevents a re-entrancy crash caused by potentially complex thumbnail generators.
	if (!FSlateApplication::Get().IsDragDropping())
	{
		SListView<TSharedPtr<FExtAssetViewItem>>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}
}

bool SExtAssetListView::HasWidgetForAsset(const FName& AssetPathName)
{
	for (const TSharedPtr<FExtAssetViewItem>& Item : WidgetGenerator.ItemsWithGeneratedWidgets)
	{
		if (Item.IsValid() && Item->GetType() == EExtAssetItemType::Normal)
		{
			const FName& ObjectPath = StaticCastSharedPtr<FExtAssetViewAsset>(Item)->Data.ObjectPath;
			if (ObjectPath == AssetPathName)
			{
				return true;
			}
		}
	}

	return false;
}

FReply SExtAssetColumnView::OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FAssetViewModeUtils::OnViewModeKeyDown(SelectedItems, InKeyEvent);

	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}
	else
	{
		return SListView<TSharedPtr<FExtAssetViewItem>>::OnKeyDown(InGeometry, InKeyEvent);
	}
}


void SExtAssetColumnView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Regreshing an asset view is an intensive task. Do not do this while a user
	// is dragging arround content for maximum responsiveness.
	// Also prevents a re-entrancy crash caused by potentially complex thumbnail generators.
	if (!FSlateApplication::Get().IsDragDropping())
	{
		return SListView<TSharedPtr<FExtAssetViewItem>>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}
}

///////////////////////////////
// SExtAssetViewItem
///////////////////////////////

SExtAssetViewItem::~SExtAssetViewItem()
{
	if (AssetItem.IsValid())
	{
		AssetItem->OnAssetDataChanged.RemoveAll(this);
	}

	OnItemDestroyed.ExecuteIfBound(AssetItem);

	if (!GExitPurge)
	{
		const bool bIsGarbageCollectingOnGameThread = (IsInGameThread() && IsGarbageCollecting());
		if (!bIsGarbageCollectingOnGameThread)
		{
			// This hack is here to make abnormal shutdowns less frequent.  Crashes here are the result UI's being shut down as a result of GC at edtior shutdown.  This code attemps to call FindObject which is not allowed during GC.
			SetForceMipLevelsToBeResident(false);
		}
	}
}

void SExtAssetViewItem::Construct( const FArguments& InArgs )
{
	AssetItem = InArgs._AssetItem;
	OnRenameBegin = InArgs._OnRenameBegin;
	OnRenameCommit = InArgs._OnRenameCommit;
	OnVerifyRenameCommit = InArgs._OnVerifyRenameCommit;
	OnItemDestroyed = InArgs._OnItemDestroyed;
	ShouldAllowToolTip = InArgs._ShouldAllowToolTip;
	ThumbnailEditMode = InArgs._ThumbnailEditMode;
	HighlightText = InArgs._HighlightText;
	OnAssetsOrPathsDragDropped = InArgs._OnAssetsOrPathsDragDropped;
	OnFilesDragDropped = InArgs._OnFilesDragDropped;
	OnIsAssetValidForCustomToolTip = InArgs._OnIsAssetValidForCustomToolTip;
	OnGetCustomAssetToolTip = InArgs._OnGetCustomAssetToolTip;
	OnVisualizeAssetToolTip = InArgs._OnVisualizeAssetToolTip;
	OnAssetToolTipClosing = InArgs._OnAssetToolTipClosing;

	bDraggedOver = false;

	bPackageDirty = false;
	OnAssetDataChanged();

	AssetItem->OnAssetDataChanged.AddSP(this, &SExtAssetViewItem::OnAssetDataChanged);

	AssetDirtyBrush = FAppStyle::GetBrush("ContentBrowser.ContentDirty");

	// Set our tooltip - this will refresh each time it's opened to make sure it's up-to-date
	SetToolTip(SNew(SExtAssetViewItemToolTip).AssetViewItem(SharedThis(this)));

	SourceControlStateDelay = 0.0f;
	bSourceControlStateRequested = false;

	ISourceControlModule::Get().RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateSP(this, &SExtAssetViewItem::HandleSourceControlProviderChanged));
	SourceControlStateChangedDelegateHandle = ISourceControlModule::Get().GetProvider().RegisterSourceControlStateChanged_Handle(FSourceControlStateChanged::FDelegate::CreateSP(this, &SExtAssetViewItem::HandleSourceControlStateChanged));

	// Source control state may have already been cached, make sure the control is in sync with 
	// cached state as the delegate is not going to be invoked again until source control state 
	// changes. This will be necessary any time the widget is destroyed and recreated after source 
	// control state has been cached; for instance when the widget is killed via FWidgetGenerator::OnEndGenerationPass 
	// or a view is refreshed due to user filtering/navigating):
	HandleSourceControlStateChanged();
}

void SExtAssetViewItem::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	const float PrevSizeX = LastGeometry.Size.X;

	LastGeometry = AllottedGeometry;

	// Set cached wrap text width based on new "LastGeometry" value. 
	// We set this only when changed because binding a delegate to text wrapping attributes is expensive
	if( PrevSizeX != AllottedGeometry.Size.X && InlineRenameWidget.IsValid() )
	{
		InlineRenameWidget->SetWrapTextAt( GetNameTextWrapWidth() );
	}

	UpdatePackageDirtyState();

	UpdateSourceControlState((float)InDeltaTime);
}

TSharedPtr<IToolTip> SExtAssetViewItem::GetToolTip()
{
	return ShouldAllowToolTip.Get() ? SCompoundWidget::GetToolTip() : NULL;
}

bool SExtAssetViewItem::ValidateDragDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool& OutIsKnownDragOperation ) const
{
	OutIsKnownDragOperation = false;
	return IsFolder() && DragDropHandler::ValidateDragDropOnAssetFolder(MyGeometry, DragDropEvent, StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath, OutIsKnownDragOperation);
}

void SExtAssetViewItem::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	ValidateDragDrop(MyGeometry, DragDropEvent, bDraggedOver); // updates bDraggedOver
}
	
void SExtAssetViewItem::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if(IsFolder())
	{
		TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
		if (Operation.IsValid())
		{
			Operation->SetCursorOverride(TOptional<EMouseCursor::Type>());

			if (Operation->IsOfType<FAssetDragDropOp>())
			{
				TSharedPtr<FAssetDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(Operation);
				DragDropOp->ResetToDefaultToolTip();
			}
		}
	}

	bDraggedOver = false;
}

FReply SExtAssetViewItem::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	ValidateDragDrop(MyGeometry, DragDropEvent, bDraggedOver); // updates bDraggedOver
	return (bDraggedOver) ? FReply::Handled() : FReply::Unhandled();
}

FReply SExtAssetViewItem::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if (ValidateDragDrop(MyGeometry, DragDropEvent, bDraggedOver)) // updates bDraggedOver
	{
		bDraggedOver = false;

		check(AssetItem->GetType() == EExtAssetItemType::Folder);

		TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
		if (!Operation.IsValid())
		{
			return FReply::Unhandled();
		}

		if (Operation->IsOfType<FExternalDragOperation>())
		{
			TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>(Operation);
			OnFilesDragDropped.ExecuteIfBound(DragDropOp->GetFiles(), StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath);
			return FReply::Handled();
		}
		
		if (Operation->IsOfType<FAssetDragDropOp>())
		{
			TSharedPtr<FAssetDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(Operation);
			OnAssetsOrPathsDragDropped.ExecuteIfBound(DragDropOp->GetAssets(), DragDropOp->GetAssetPaths(), StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath);
			return FReply::Handled();
		}
	}

	if (bDraggedOver)
	{
		// We were able to handle this operation, but could not due to another error - still report this drop as handled so it doesn't fall through to other widgets
		bDraggedOver = false;
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

bool SExtAssetViewItem::IsNameReadOnly() const
{
	return true;
}

void SExtAssetViewItem::HandleBeginNameChange( const FText& OriginalText )
{
	OnRenameBegin.ExecuteIfBound(AssetItem, OriginalText.ToString(), LastGeometry.GetLayoutBoundingRect());
}

void SExtAssetViewItem::HandleNameCommitted( const FText& NewText, ETextCommit::Type CommitInfo )
{
	OnRenameCommit.ExecuteIfBound(AssetItem, NewText.ToString(), LastGeometry.GetLayoutBoundingRect(), CommitInfo);
}

bool  SExtAssetViewItem::HandleVerifyNameChanged( const FText& NewText, FText& OutErrorMessage )
{
	return !OnVerifyRenameCommit.IsBound() || OnVerifyRenameCommit.Execute(AssetItem, NewText, LastGeometry.GetLayoutBoundingRect(), OutErrorMessage);
}

void SExtAssetViewItem::OnAssetDataChanged()
{
	CachePackageName();
	AssetPackage = FindObjectSafe<UPackage>(NULL, *CachedPackageName);
	UpdatePackageDirtyState();

	AssetTypeActions.Reset();
	if ( AssetItem->GetType() != EExtAssetItemType::Folder )
	{
		UClass* AssetClass = FindObject<UClass>(nullptr, *StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.AssetClass.ToString());
		if (AssetClass)
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetClass).Pin();
		}
	}

	if ( InlineRenameWidget.IsValid() )
	{
		InlineRenameWidget->SetText( GetNameText() );
	}

	CacheDisplayTags();
}

void SExtAssetViewItem::DirtyStateChanged()
{
}

FText SExtAssetViewItem::GetAssetClassText() const
{
	if (!AssetItem.IsValid())
	{
		return FText();
	}
	
	if (AssetItem->GetType() == EExtAssetItemType::Folder)
	{
		return LOCTEXT("FolderName", "Folder");
	}

	if (AssetTypeActions.IsValid())
	{
		FText Name = AssetTypeActions.Pin()->GetName();

		if (!Name.IsEmpty())
		{
			return Name;
		}
	}

	return FText::FromName(StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.AssetClass);
}


void SExtAssetViewItem::HandleSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider)
{
	OldProvider.UnregisterSourceControlStateChanged_Handle(SourceControlStateChangedDelegateHandle);
	SourceControlStateChangedDelegateHandle = NewProvider.RegisterSourceControlStateChanged_Handle(FSourceControlStateChanged::FDelegate::CreateSP(this, &SExtAssetViewItem::HandleSourceControlStateChanged));
	
	// Reset this so the state will be queried from the new provider on the next Tick
	SourceControlStateDelay = 0.0f;
	bSourceControlStateRequested = false;
	
	HandleSourceControlStateChanged();
}

void SExtAssetViewItem::HandleSourceControlStateChanged()
{
	if ( ISourceControlModule::Get().IsEnabled() && AssetItem.IsValid() && (AssetItem->GetType() == EExtAssetItemType::Normal) && !AssetItem->IsTemporaryItem() && !FPackageName::IsScriptPackage(CachedPackageName) )
	{
		FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(CachedPackageFileName, EStateCacheUsage::Use);
		if(SourceControlState.IsValid())
		{
			if (SCCStateWidget.IsValid())
			{
				SCCStateWidget->SetFromSlateIcon(SourceControlState->GetIcon());
			}
		}
	}
}

const FSlateBrush* SExtAssetViewItem::GetDirtyImage() const
{
	return IsDirty() ? AssetDirtyBrush : NULL;
}


TSharedRef<SWidget> SExtAssetViewItem::GenerateSourceControlIconWidget()
{
	TSharedRef<SLayeredImage> Image = SNew(SLayeredImage).Image(FStyleDefaults::GetNoBrush());
	SCCStateWidget = Image;

	return Image;
}

TSharedRef<SWidget> SExtAssetViewItem::GenerateExtraStateIconWidget(TAttribute<float> InMaxExtraStateIconWidth) const
{
#if ECB_TODO // Extra icon support
	TArray<FOnGenerateAssetViewExtraStateIndicators>& ExtraStateIconGenerators = FModuleManager::GetModuleChecked<FExtContentBrowserModule>(TEXT("ExtContentBrowser")).GetAllAssetViewExtraStateIconGenerators();
	if (AssetItem->GetType() != EAssetItemType::Folder && ExtraStateIconGenerators.Num() > 0)
	{
		FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
		TSharedPtr<SHorizontalBox> Content = SNew(SHorizontalBox);
		for (const auto& Generator : ExtraStateIconGenerators)
		{
			if (Generator.IsBound())
			{
				Content->AddSlot()
					.HAlign(HAlign_Left)
					.MaxWidth(InMaxExtraStateIconWidth)
					[
						Generator.Execute(AssetData)
					];
			}
		}
		return Content.ToSharedRef();
	}
#endif
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SExtAssetViewItem::GenerateExtraStateTooltipWidget() const
{
#if ECB_TODO // Extra icon support
	TArray<FOnGenerateAssetViewExtraStateIndicators>& ExtraStateTooltipGenerators = FModuleManager::GetModuleChecked<FExtContentBrowserModule>(TEXT("ContentBrowser")).GetAllAssetViewExtraStateTooltipGenerators();
	if (AssetItem->GetType() != EAssetItemType::Folder && ExtraStateTooltipGenerators.Num() > 0)
	{
		FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
		TSharedPtr<SVerticalBox> Content = SNew(SVerticalBox);
		for (const auto& Generator : ExtraStateTooltipGenerators)
		{
			if (Generator.IsBound())
			{
				Content->AddSlot()
					[
						Generator.Execute(AssetData)
					];
			}
		}
		return Content.ToSharedRef();
	}
#endif
	return SNullWidget::NullWidget;
}

EVisibility SExtAssetViewItem::GetThumbnailEditModeUIVisibility() const
{
	return !IsFolder() && ThumbnailEditMode.Get() ? EVisibility::Visible : EVisibility::Collapsed;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SWidget> SExtAssetViewItem::CreateToolTipWidget() const
{
	if ( AssetItem.IsValid() )
	{
#if ECB_TODO // Custom tooltip
		bool bTryCustomAssetToolTip = true;
		if (OnIsAssetValidForCustomToolTip.IsBound() && AssetItem->GetType() != EAssetItemType::Folder)
		{
			FExtAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			bTryCustomAssetToolTip = OnIsAssetValidForCustomToolTip.Execute(AssetData);
		}

		if(bTryCustomAssetToolTip && OnGetCustomAssetToolTip.IsBound() && AssetItem->GetType() != EAssetItemType::Folder)
		{
			FExtAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			return OnGetCustomAssetToolTip.Execute(AssetData);
		}
		else 
#endif
		if(AssetItem->GetType() != EExtAssetItemType::Folder)
		{
			const FExtAssetData& AssetData = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data;

			// The tooltip contains the name, class, path, and asset registry tags
			const FText NameText = FText::FromName( AssetData.AssetName );
			const FText ClassText = FText::Format( LOCTEXT("ClassName", "({0})"), GetAssetClassText() );

			// Create a box to hold every line of info in the body of the tooltip
			TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

			// Add File Path
			AddToToolTipInfoBox( InfoBox, LOCTEXT("TileViewTooltipPath", "Path"), FText::FromName(AssetData.PackageFilePath), false);

			// Saved By Engine Version
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipEngine", "Engine"), FText::FromString(AssetData.SavedByEngineVersion.ToString(EVersionComponent::Patch)), false);

#if ECB_DEBUG
			// File Version
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipFileVersion", "File Version"), FText::AsNumber(AssetData.FileVersionUE4), false);

			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipSavedByEngineDetail", "Saved By Engine"), FText::FromString(AssetData.SavedByEngineVersion.ToString(EVersionComponent::Branch)), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipCompatibleEngineDetail", "Compatible Engine"), FText::FromString(AssetData.CompatibleWithEngineVersion.ToString(EVersionComponent::Branch)), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipPackageName", "PackageName"), FText::FromName(AssetData.PackageName), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipPackagePath", "PackagePath"), FText::FromName(AssetData.PackagePath), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipObjectPath", "ObjectPath"), FText::FromName(AssetData.ObjectPath), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipAssetName", "AssetName"), FText::FromName(AssetData.AssetName), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipAssetClass", "AssetClass"), FText::FromName(AssetData.AssetClass), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipAssetContentRootDir", "AssetContentRootDir"), FText::FromName(AssetData.AssetContentRoot), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipAssetRelativePath", "AssetRelativePath"), FText::FromName(AssetData.AssetRelativePath), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipHasThumbnail", "HasThumbnail"), FText::AsNumber(AssetData.HasThumbnail()), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipNumTagsAndValues", "Num of TagsAndValues"), FText::AsNumber(AssetData.TagsAndValues.Num()), false);

			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipHardDependentPackages", "Num of Hard Dependencies"), FText::AsNumber(AssetData.HardDependentPackages.Num()), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipSoftReferencesList", "Num of Soft References"), FText::AsNumber(AssetData.SoftReferencesList.Num()), false);

			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipAssetCount", "Num of Assets"), FText::AsNumber(AssetData.AssetCount), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipThumbCount", "Num of Thumbnails"), FText::AsNumber(AssetData.ThumbCount), false);
			AddToToolTipInfoBox(InfoBox, LOCTEXT("TileViewTooltipFileSize", "File Size"), FText::AsNumber(AssetData.FileSize), false);
#endif

			// Add Collections
#if ECB_WIP_COLLECTION
			{
				FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
				const FString CollectionNames = CollectionManagerModule.Get().GetCollectionsStringForObject(AssetData.ObjectPath, ECollectionShareType::CST_All, ECollectionRecursionFlags::Self, GetDefault<UExtContentBrowserSettings>()->bShowFullCollectionNameInToolTip);
				if (!CollectionNames.IsEmpty())
				{
					AddToToolTipInfoBox(InfoBox, LOCTEXT("AssetToolTipKey_Collections", "Collections"), FText::FromString(CollectionNames), false);
				}
			}
#endif

			// Add tags
			for (const auto& DisplayTagItem : CachedDisplayTags)
			{
				AddToToolTipInfoBox(InfoBox, DisplayTagItem.DisplayKey, DisplayTagItem.DisplayValue, DisplayTagItem.bImportant);
			}

			// Add asset source files
			TOptional<FAssetImportInfo> ImportInfo = FExtContentBrowserSingleton::GetAssetRegistry().ExtractAssetImportInfo(AssetData);
			if (ImportInfo.IsSet())
			{
				for (const auto& File : ImportInfo->SourceFiles)
				{
					FText SourceLabel = LOCTEXT("TileViewTooltipSourceFile", "Source File");
					if (File.DisplayLabelName.Len() > 0)
					{
						SourceLabel = FText::FromString(FText(LOCTEXT("TileViewTooltipSourceFile", "Source File")).ToString() + TEXT(" (") + File.DisplayLabelName + TEXT(")"));
					}
					AddToToolTipInfoBox( InfoBox, SourceLabel, FText::FromString(File.RelativeFilename), false );
				}
			}

			TSharedRef<SVerticalBox> OverallTooltipVBox = SNew(SVerticalBox);

			// Top section (asset name, type, is checked out)
			OverallTooltipVBox->AddSlot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(SBorder)
					.Padding(6)
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0, 0, 4, 0)
							[
								SNew(STextBlock)
								.Text(NameText)
								.Font(FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(ClassText)
								.HighlightText(HighlightText)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							GenerateExtraStateTooltipWidget()
						]
					]
				];

			// Middle section (user description, if present)
			FText UserDescription = GetAssetUserDescription();
			if (!UserDescription.IsEmpty())
			{
				OverallTooltipVBox->AddSlot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(SBorder)
						.Padding(6)
						.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
						[
							SNew(STextBlock)
							.WrapTextAt(300.0f)
							.Font(FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.AssetUserDescriptionFont"))
							.Text(UserDescription)
						]
					];
			}

			// Bottom section (asset registry tags)
			OverallTooltipVBox->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.Padding(6)
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						InfoBox
					]
				];


			return SNew(SBorder)
				.Padding(6)
				.BorderImage( FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder") )
				[
					OverallTooltipVBox
				];
		}
		else
		{
			const FText& FolderName = StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderName;
			const FString& FolderPath = StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath;

			// Create a box to hold every line of info in the body of the tooltip
			TSharedRef<SVerticalBox> InfoBox = SNew(SVerticalBox);

			AddToToolTipInfoBox( InfoBox, LOCTEXT("TileViewTooltipPath", "Path"), FText::FromString(FolderPath), false );

			return SNew(SBorder)
				.Padding(6)
				.BorderImage( FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder") )
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(SBorder)
						.Padding(6)
						.BorderImage( FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder") )
						[
							SNew(SVerticalBox)

							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0, 0, 4, 0)
								[
									SNew(STextBlock)
									.Text( FolderName )
									.Font( FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont") )
								]

								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock) 
									.Text( LOCTEXT("FolderNameBracketed", "(Folder)") )
								]
							]
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.Padding(6)
						.BorderImage( FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder") )
						[
							InfoBox
						]
					]
				];
		}

	}
	else
	{
		// Return an empty tooltip since the asset item wasn't valid
		return SNullWidget::NullWidget;
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


FText SExtAssetViewItem::GetAssetUserDescription() const
{
#if ECB_TODO // Get Asset User Description
	if (AssetItem.IsValid() && AssetTypeActions.IsValid())
	{
		if (AssetItem->GetType() != EAssetItemType::Folder)
		{
			const FAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
			return AssetTypeActions.Pin()->GetAssetDescription(AssetData);
		}
	}
#endif
	return FText::GetEmpty();
}

void SExtAssetViewItem::AddToToolTipInfoBox(const TSharedRef<SVerticalBox>& InfoBox, const FText& Key, const FText& Value, bool bImportant) const
{
	FWidgetStyle ImportantStyle;
	ImportantStyle.SetForegroundColor(FLinearColor(1, 0.5, 0, 1));

	InfoBox->AddSlot()
	.AutoHeight()
	.Padding(0, 1)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 4, 0)
		[
			SNew(STextBlock) .Text( FText::Format(LOCTEXT("AssetViewTooltipFormat", "{0}:"), Key ) )
			.ColorAndOpacity(bImportant ? ImportantStyle.GetSubduedForegroundColor() : FSlateColor::UseSubduedForeground())
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock) .Text( Value )
			.ColorAndOpacity(bImportant ? ImportantStyle.GetForegroundColor() : FSlateColor::UseForeground())
			.HighlightText((Key.ToString() == TEXT("Path")) ? HighlightText : FText())
			.WrapTextAt(700.0f)
		]
	];
}

void SExtAssetViewItem::UpdatePackageDirtyState()
{
	bool bNewIsDirty = false;

	// Only update the dirty state for non-temporary asset items that aren't a built in script
	if ( AssetItem.IsValid() && !AssetItem->IsTemporaryItem() && AssetItem->GetType() != EExtAssetItemType::Folder && !FPackageName::IsScriptPackage(CachedPackageName) )
	{
		if ( AssetPackage.IsValid() )
		{
			bNewIsDirty = AssetPackage->IsDirty();
		}
	}

	if ( bNewIsDirty != bPackageDirty )
	{
		bPackageDirty = bNewIsDirty;
		DirtyStateChanged();
	}
}

bool SExtAssetViewItem::IsDirty() const
{
	return bPackageDirty;
}

void SExtAssetViewItem::UpdateSourceControlState(float InDeltaTime)
{
	SourceControlStateDelay += InDeltaTime;

	static ISourceControlModule& SourceControlModule = ISourceControlModule::Get();
	if ( !bSourceControlStateRequested && SourceControlStateDelay > 1.0f && SourceControlModule.IsEnabled() && AssetItem.IsValid() )
	{
		// Only update the SCC state for non-temporary asset items that aren't a built in script
		if ( !AssetItem->IsTemporaryItem() && AssetItem->GetType() != EExtAssetItemType::Folder && !FPackageName::IsScriptPackage(CachedPackageName) && !FPackageName::IsMemoryPackage(CachedPackageName) && !FPackageName::IsTempPackage(CachedPackageName))
		{
			// Request the most recent SCC state for this asset
			SourceControlModule.QueueStatusUpdate(CachedPackageFileName);
		}

		bSourceControlStateRequested = true;
	}
}

void SExtAssetViewItem::CachePackageName()
{
#if ECB_LEGACY
	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() != EAssetItemType::Folder)
		{
			CachedPackageName = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.PackageName.ToString();
			CachedPackageFileName = SourceControlHelpers::PackageFilename(CachedPackageName);
		}
		else
		{
			CachedPackageName = StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderName.ToString();
		}
	}
#endif
}

void SExtAssetViewItem::CacheDisplayTags()
{
	CachedDisplayTags.Reset();

	if (AssetItem->GetType() == EExtAssetItemType::Folder)
	{
		return;
	}

	const FExtAssetData& AssetData = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data;

	// Find the asset CDO so we can get the meta-data for the tags
	UClass* AssetClass = FindObject<UClass>(nullptr, *AssetData.AssetClass.ToString());
	const UObject* AssetCDO = AssetClass ? GetDefault<UObject>(AssetClass) : nullptr;

	struct FTagDisplayMetaData
	{
		FTagDisplayMetaData()
			: MetaData()
			, Type(UObject::FAssetRegistryTag::TT_Hidden)
			, DisplayFlags(UObject::FAssetRegistryTag::TD_None)
		{
		}

		UObject::FAssetRegistryTagMetadata MetaData;
		UObject::FAssetRegistryTag::ETagType Type;
		uint32 DisplayFlags;
	};

	bool bHasTagMetaDataMap = false;

	// Build up the meta-data needed to correctly process the tags for display
	TMap<FName, FTagDisplayMetaData> TagMetaDataMap;
	if (AssetCDO)
	{
		bHasTagMetaDataMap = true;

		// Add the internal meta-data
		{
			TMap<FName, UObject::FAssetRegistryTagMetadata> TmpMetaData;
			AssetCDO->GetAssetRegistryTagMetadata(TmpMetaData);

			for (const auto& TmpMetaDataPair : TmpMetaData)
			{
				FTagDisplayMetaData& TagMetaData = TagMetaDataMap.FindOrAdd(TmpMetaDataPair.Key);
				TagMetaData.MetaData = TmpMetaDataPair.Value;
			}
		}

		// Add the type and display flags
		{
			TArray<UObject::FAssetRegistryTag> TmpTags;
			if (AssetCDO->GetClass()->IsChildOf(UActorFolder::StaticClass()))
			{
			}
			else
			{
				AssetCDO->GetAssetRegistryTags(TmpTags);
			}

			for (const UObject::FAssetRegistryTag& TmpTag : TmpTags)
			{
				FTagDisplayMetaData& TagMetaData = TagMetaDataMap.FindOrAdd(TmpTag.Name);
				TagMetaData.Type = TmpTag.Type;
				TagMetaData.DisplayFlags = TmpTag.DisplayFlags;
			}
		}
	}

	// Add all asset registry tags and values
	for (const auto& TagAndValuePair : AssetData.TagsAndValues)
	{
		// Skip tags that are not registered
#if !ECB_DEBUG
		if (bHasTagMetaDataMap && !TagMetaDataMap.Contains(TagAndValuePair.Key))
		{
			continue;
		}
#endif

		FTagDisplayMetaData* TagMetaData = NULL;
		if (bHasTagMetaDataMap && TagMetaDataMap.Contains(TagAndValuePair.Key))
		{
			TagMetaData = TagMetaDataMap.Find(TagAndValuePair.Key);

			// Skip tags that are set to be hidden
			if (TagMetaData && TagMetaData->Type == UObject::FAssetRegistryTag::TT_Hidden)
			{
				continue;
			}
		}
		const bool bHasTagMetaData = TagMetaData != NULL;

		FProperty* TagField = AssetClass != NULL ? FindFProperty<FProperty>(AssetClass, TagAndValuePair.Key) : NULL;

		// Build the display name for this tag
		FText DisplayName;
		if (bHasTagMetaData && !TagMetaData->MetaData.DisplayName.IsEmpty())
		{
			DisplayName = TagMetaData->MetaData.DisplayName;
		}
		else if (TagField)
		{
			DisplayName = TagField->GetDisplayNameText();
		}
		else
		{
			// We have no type information by this point, so no idea if it's a bool :(
			const bool bIsBool = false;
			DisplayName = FText::FromString(FName::NameToDisplayString(TagAndValuePair.Key.ToString(), bIsBool));
		}

		// Build the display value for this tag
		FText DisplayValue;
		{
			auto ReformatNumberStringForDisplay = [](const FString& InNumberString) -> FText
			{
				// Respect the number of decimal places in the source string when converting for display
				int32 NumDecimalPlaces = 0;
				{
					int32 DotIndex = INDEX_NONE;
					if (InNumberString.FindChar(TEXT('.'), DotIndex))
					{
						NumDecimalPlaces = InNumberString.Len() - DotIndex - 1;
					}
				}

				if (NumDecimalPlaces > 0)
				{
					// Convert the number as a double
					double Num = 0.0;
					LexFromString(Num, *InNumberString);

					const FNumberFormattingOptions NumFormatOpts = FNumberFormattingOptions()
						.SetMinimumFractionalDigits(NumDecimalPlaces)
						.SetMaximumFractionalDigits(NumDecimalPlaces);

					return FText::AsNumber(Num, &NumFormatOpts);
				}
				else
				{
					const bool bIsSigned = InNumberString.Len() > 0 && (InNumberString[0] == TEXT('-') || InNumberString[0] == TEXT('+'));

					if (bIsSigned)
					{
						// Convert the number as a signed int
						int64 Num = 0;
						LexFromString(Num, *InNumberString);

						return FText::AsNumber(Num);
					}
					else
					{
						// Convert the number as an unsigned int
						uint64 Num = 0;
						LexFromString(Num, *InNumberString);

						return FText::AsNumber(Num);
					}
				}

				return FText::GetEmpty();
			};

			bool bHasSetDisplayValue = false;

			const FString AttributeValueStr = TagAndValuePair.Value.GetValue();

			// Numerical tags need to format the specified number based on the display flags
			if (!bHasSetDisplayValue && bHasTagMetaData && TagMetaData->Type == UObject::FAssetRegistryTag::TT_Numerical && AttributeValueStr.IsNumeric())
			{
				bHasSetDisplayValue = true;

				const bool bAsMemory = !!(TagMetaData->DisplayFlags & UObject::FAssetRegistryTag::TD_Memory);

				if (bAsMemory)
				{
					// Memory should be a 64-bit unsigned number of bytes
					uint64 NumBytes = 0;
					LexFromString(NumBytes, *AttributeValueStr);

					DisplayValue = FText::AsMemory(NumBytes);
				}
				else
				{
					DisplayValue = ReformatNumberStringForDisplay(AttributeValueStr);
				}
			}

			// Dimensional tags need to be split into their component numbers, with each component number re-format
			if (!bHasSetDisplayValue && bHasTagMetaData && TagMetaData->Type == UObject::FAssetRegistryTag::TT_Dimensional)
			{
				TArray<FString> NumberStrTokens;
				AttributeValueStr.ParseIntoArray(NumberStrTokens, TEXT("x"), true);

				if (NumberStrTokens.Num() > 0 && NumberStrTokens.Num() <= 3)
				{
					bHasSetDisplayValue = true;

					switch (NumberStrTokens.Num())
					{
					case 1:
						DisplayValue = ReformatNumberStringForDisplay(NumberStrTokens[0]);
						break;

					case 2:
						DisplayValue = FText::Format(LOCTEXT("DisplayTag2xFmt", "{0} \u00D7 {1}"), ReformatNumberStringForDisplay(NumberStrTokens[0]), ReformatNumberStringForDisplay(NumberStrTokens[1]));
						break;

					case 3:
						DisplayValue = FText::Format(LOCTEXT("DisplayTag3xFmt", "{0} \u00D7 {1} \u00D7 {2}"), ReformatNumberStringForDisplay(NumberStrTokens[0]), ReformatNumberStringForDisplay(NumberStrTokens[1]), ReformatNumberStringForDisplay(NumberStrTokens[2]));
						break;

					default:
						break;
					}
				}
			}

			// Chronological tags need to format the specified timestamp based on the display flags
			if (!bHasSetDisplayValue && bHasTagMetaData && TagMetaData->Type == UObject::FAssetRegistryTag::TT_Chronological)
			{
				bHasSetDisplayValue = true;

				FDateTime Timestamp;
				if (FDateTime::Parse(AttributeValueStr, Timestamp))
				{
					const bool bDisplayDate = !!(TagMetaData->DisplayFlags & UObject::FAssetRegistryTag::TD_Date);
					const bool bDisplayTime = !!(TagMetaData->DisplayFlags & UObject::FAssetRegistryTag::TD_Time);
					const FString TimeZone = (TagMetaData->DisplayFlags & UObject::FAssetRegistryTag::TD_InvariantTz) ? FText::GetInvariantTimeZone() : FString();

					if (bDisplayDate && bDisplayTime)
					{
						DisplayValue = FText::AsDateTime(Timestamp, EDateTimeStyle::Short, EDateTimeStyle::Short, TimeZone);
					}
					else if (bDisplayDate)
					{
						DisplayValue = FText::AsDate(Timestamp, EDateTimeStyle::Short, TimeZone);
					}
					else if (bDisplayTime)
					{
						DisplayValue = FText::AsTime(Timestamp, EDateTimeStyle::Short, TimeZone);
					}
				}
			}

			// The tag value might be localized text, so we need to parse it for display
			if (!bHasSetDisplayValue && FTextStringHelper::IsComplexText(*AttributeValueStr))
			{
				bHasSetDisplayValue = FTextStringHelper::ReadFromBuffer(*AttributeValueStr, DisplayValue) != nullptr;
			}

			// Do our best to build something valid from the string value
			if (!bHasSetDisplayValue)
			{
				bHasSetDisplayValue = true;

				// Since all we have at this point is a string, we can't be very smart here.
				// We need to strip some noise off class paths in some cases, but can't load the asset to inspect its UPROPERTYs manually due to performance concerns.
				FString ValueString = FPackageName::ExportTextPathToObjectPath(AttributeValueStr);

				const TCHAR StringToRemove[] = TEXT("/Script/");
				if (ValueString.StartsWith(StringToRemove))
				{
					// Remove the class path for native classes, and also remove Engine. for engine classes
					const int32 SizeOfPrefix = UE_ARRAY_COUNT(StringToRemove) - 1;
					ValueString = ValueString.Mid(SizeOfPrefix, ValueString.Len() - SizeOfPrefix).Replace(TEXT("Engine."), TEXT(""));
				}

				if (TagField)
				{
					FProperty* TagProp = nullptr;
					UEnum* TagEnum = nullptr;
					if (FByteProperty* ByteProp = CastField<FByteProperty>(TagField))
					{
						TagProp = ByteProp;
						TagEnum = ByteProp->Enum;
					}
					else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(TagField))
					{
						TagProp = EnumProp;
						TagEnum = EnumProp->GetEnum();
					}

					// Strip off enum prefixes if they exist
					if (TagProp)
					{
						if (TagEnum)
						{
							const FString EnumPrefix = TagEnum->GenerateEnumPrefix();
							if (EnumPrefix.Len() && ValueString.StartsWith(EnumPrefix))
							{
								ValueString = ValueString.RightChop(EnumPrefix.Len() + 1);	// +1 to skip over the underscore
							}
						}

						ValueString = FName::NameToDisplayString(ValueString, false);
					}
				}

				DisplayValue = FText::FromString(MoveTemp(ValueString));
			}
						
			// Add suffix to the value, if one is defined for this tag
			if (bHasTagMetaData && !TagMetaData->MetaData.Suffix.IsEmpty())
			{
				DisplayValue = FText::Format(LOCTEXT("DisplayTagSuffixFmt", "{0} {1}"), DisplayValue, TagMetaData->MetaData.Suffix);
			}
		}

		if (!DisplayValue.IsEmpty())
		{
			const bool bImportant = bHasTagMetaData && !TagMetaData->MetaData.ImportantValue.IsEmpty() && TagMetaData->MetaData.ImportantValue == TagAndValuePair.Value;
			CachedDisplayTags.Add(FTagDisplayItem(TagAndValuePair.Key, DisplayName, DisplayValue, bImportant));
		}
	}

}

const FSlateBrush* SExtAssetViewItem::GetBorderImage() const
{
	return bDraggedOver ? FAppStyle::GetBrush("Menu.Background") : FAppStyle::GetBrush("NoBorder");
}

bool SExtAssetViewItem::IsFolder() const
{
	return AssetItem.IsValid() && AssetItem->GetType() == EExtAssetItemType::Folder;
}

FText SExtAssetViewItem::GetNameText() const
{
	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() != EExtAssetItemType::Folder)
		{
			auto& AssetData = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data;

#if ECB_TODO // todo: AssetName should fallback to filename when parsing even invalid
			if (AssetData.IsValid() || AssetData.PackageFilePath == NAME_None)
			{
				return FText::FromName(AssetData.AssetName);
			}
			else
			{
				return FText::FromString(FPaths::GetBaseFilename(AssetData.PackageFilePath.ToString()));
			}
#endif
			return FText::FromName(AssetData.AssetName);
		}
		else
		{
			return StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderName;
		}
	}

	return FText();
}

FSlateColor SExtAssetViewItem::GetAssetColor() const
{
	if(AssetItem.IsValid())
	{
		if(AssetItem->GetType() == EExtAssetItemType::Folder)
		{
			TSharedPtr<FExtAssetViewFolder> AssetFolderItem = StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem);

			TSharedPtr<FLinearColor> Color;
#if ECB_WIP_COLLECTION
			if (AssetFolderItem->bCollectionFolder)
			{
				FName CollectionName;
				ECollectionShareType::Type CollectionFolderShareType = ECollectionShareType::CST_All;
				ContentBrowserUtils::IsCollectionPath(AssetFolderItem->FolderPath, &CollectionName, &CollectionFolderShareType);

				Color = CollectionViewUtils::LoadColor( CollectionName.ToString(), CollectionFolderShareType );
			}
			else
#endif
			{
#if ECB_LEGACY
				Color = ExtContentBrowserUtils::LoadColor( AssetFolderItem->FolderPath );
#endif
			}
#if ECB_LEGACY
			if ( Color.IsValid() )
			{
				return *Color.Get();
			}			
#endif
			FLinearColor FolderColor;
			if (ExtContentBrowserUtils::LoadFolderColor(AssetFolderItem->FolderPath, FolderColor))
			{
				return FolderColor;
			}
			else
			{
				return ExtContentBrowserUtils::GetDefaultColor();
			}
		}
		else if(AssetTypeActions.IsValid())
		{
			return AssetTypeActions.Pin()->GetTypeColor().ReinterpretAsLinear();
		}
	}
	return ExtContentBrowserUtils::GetDefaultColor();
}

void SExtAssetViewItem::SetForceMipLevelsToBeResident(bool bForce) const
{
#if ECB_LEGACY
	if(AssetItem.IsValid() && AssetItem->GetType() == EAssetItemType::Normal)
	{
		const FExtAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
		if(AssetData.IsValid() && AssetData.IsAssetLoaded())
		{
			UObject* Asset = AssetData.GetAsset();
			if(Asset != nullptr)
			{
				if(UTexture2D* Texture2D = Cast<UTexture2D>(Asset))
				{
					Texture2D->bForceMiplevelsToBeResident = bForce;
				}
				else if(UMaterial* Material = Cast<UMaterial>(Asset))
				{
					Material->SetForceMipLevelsToBeResident(bForce, bForce, -1.0f);
				}
			}
		}
	}
#endif
}

bool SExtAssetViewItem::OnVisualizeTooltip(const TSharedPtr<SWidget>& TooltipContent)
{
#if ECB_LEGACY
	if(OnVisualizeAssetToolTip.IsBound() && TooltipContent.IsValid() && AssetItem->GetType() != EAssetItemType::Folder)
	{
		FExtAssetData& AssetData = StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data;
		return OnVisualizeAssetToolTip.Execute(TooltipContent, AssetData);
	}
#endif
	// No custom behaviour, return false to allow slate to visualize the widget
	return false;
}

void SExtAssetViewItem::OnToolTipClosing()
{
	OnAssetToolTipClosing.ExecuteIfBound();
}

///////////////////////////////
// SExtAssetListItem
///////////////////////////////

SExtAssetListItem::~SExtAssetListItem()
{

}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SExtAssetListItem::Construct( const FArguments& InArgs )
{
	SExtAssetViewItem::Construct( SExtAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ShouldAllowToolTip(InArgs._ShouldAllowToolTip)
		.ThumbnailEditMode(InArgs._ThumbnailEditMode)
		.HighlightText(InArgs._HighlightText)
		.OnAssetsOrPathsDragDropped(InArgs._OnAssetsOrPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		.OnIsAssetValidForCustomToolTip(InArgs._OnIsAssetValidForCustomToolTip)
		.OnGetCustomAssetToolTip(InArgs._OnGetCustomAssetToolTip)
		.OnVisualizeAssetToolTip(InArgs._OnVisualizeAssetToolTip)
		.OnAssetToolTipClosing( InArgs._OnAssetToolTipClosing )
		);

	AssetThumbnail = InArgs._AssetThumbnail;
	ItemHeight = InArgs._ItemHeight;

	const float ThumbnailPadding = InArgs._ThumbnailPadding;

	TSharedPtr<SWidget> Thumbnail;
	if ( AssetItem.IsValid() && AssetThumbnail.IsValid() )
	{
		FExtAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.bAllowFadeIn = true;
		ThumbnailConfig.bAllowHintText = InArgs._AllowThumbnailHintLabel;
		ThumbnailConfig.bForceGenericThumbnail = (AssetItem->GetType() == EExtAssetItemType::Creation);
		//ThumbnailConfig.bAllowAssetSpecificThumbnailOverlay = (AssetItem->GetType() != EExtAssetItemType::Creation);
		ThumbnailConfig.bAllowAssetSpecificThumbnailOverlay = false;
		ThumbnailConfig.ThumbnailLabel = InArgs._ThumbnailLabel;
		ThumbnailConfig.HighlightedText = InArgs._HighlightText;
		ThumbnailConfig.HintColorAndOpacity = InArgs._ThumbnailHintColorAndOpacity;
		Thumbnail = AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig);
	}
	else
	{
		Thumbnail = SNew(SImage) .Image( FAppStyle::GetDefaultBrush() );
	}

	FName ItemShadowBorderName;
	TSharedRef<SWidget> ItemContents = FExtAssetViewItemHelper::CreateListItemContents(this, Thumbnail.ToSharedRef(), ItemShadowBorderName);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(this, &SExtAssetViewItem::GetBorderImage)
		.Padding(0)
		.AddMetaData<FTagMetaData>(FTagMetaData(AssetItem->GetType() == EExtAssetItemType::Normal ? StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.ObjectPath : NAME_None))
		[
			SNew(SHorizontalBox)

			// Viewport
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew( SBox )
				.Padding(ThumbnailPadding - 4.f)
				.WidthOverride( this, &SExtAssetListItem::GetThumbnailBoxSize )
				.HeightOverride( this, &SExtAssetListItem::GetThumbnailBoxSize )
				[
					// Drop shadow border
					SNew(SBorder)
					.Padding(4.f)
					.BorderImage(FAppStyle::GetBrush(ItemShadowBorderName))
					[
						ItemContents
					]
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 1)
				[
					SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
					.Font(FAppStyle::GetFontStyle("ContentBrowser.AssetTileViewNameFont"))
					.Text( GetNameText() )
					.OnBeginTextEdit(this, &SExtAssetListItem::HandleBeginNameChange)
					.OnTextCommitted(this, &SExtAssetListItem::HandleNameCommitted)
					.OnVerifyTextChanged(this, &SExtAssetListItem::HandleVerifyNameChanged)
					.HighlightText(InArgs._HighlightText)
					.IsSelected(InArgs._IsSelected)
					.IsReadOnly(this, &SExtAssetListItem::IsNameReadOnly)
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 1)
				[
					// Class
					SAssignNew(ClassText, STextBlock)
					.Font(FAppStyle::GetFontStyle("ContentBrowser.AssetListViewClassFont"))
					.Text(GetAssetClassText())
					.HighlightText(InArgs._HighlightText)
				]
			]
		]
	];

	if(AssetItem.IsValid())
	{
		AssetItem->RenamedRequestEvent.BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
		AssetItem->RenameCanceledEvent.BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::ExitEditingMode);
	}

	SetForceMipLevelsToBeResident(true);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SExtAssetListItem::OnAssetDataChanged()
{
	SExtAssetViewItem::OnAssetDataChanged();

	if (ClassText.IsValid())
	{
		ClassText->SetText(GetAssetClassText());
	}

	if (AssetItem->GetType() != EExtAssetItemType::Folder && AssetThumbnail.IsValid())
	{
		AssetThumbnail->SetAsset(StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data);
	}
}

float SExtAssetListItem::GetExtraStateIconWidth() const
{
	return GetStateIconImageSize().Get();
}

FOptionalSize SExtAssetListItem::GetExtraStateIconMaxWidth() const
{
	return GetThumbnailBoxSize().Get() * 0.7;
}

FOptionalSize SExtAssetListItem::GetStateIconImageSize() const
{
	float IconSize = GetThumbnailBoxSize().Get() * 0.3;
	return IconSize > 12 ? IconSize : 12;
}

FOptionalSize SExtAssetListItem::GetThumbnailBoxSize() const
{
	return FOptionalSize( ItemHeight.Get() );
}

///////////////////////////////
// SExtAssetTileItem
///////////////////////////////

SExtAssetTileItem::~SExtAssetTileItem()
{

}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SExtAssetTileItem::Construct( const FArguments& InArgs )
{
	SExtAssetViewItem::Construct( SExtAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ShouldAllowToolTip(InArgs._ShouldAllowToolTip)
		.ThumbnailEditMode(InArgs._ThumbnailEditMode)
		.HighlightText(InArgs._HighlightText)
		.OnAssetsOrPathsDragDropped(InArgs._OnAssetsOrPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		.OnIsAssetValidForCustomToolTip(InArgs._OnIsAssetValidForCustomToolTip)
		.OnGetCustomAssetToolTip(InArgs._OnGetCustomAssetToolTip)
		.OnVisualizeAssetToolTip(InArgs._OnVisualizeAssetToolTip)
		.OnAssetToolTipClosing( InArgs._OnAssetToolTipClosing )
		);

	AssetThumbnail = InArgs._AssetThumbnail;
	ItemWidth = InArgs._ItemWidth;
	ThumbnailPadding = IsFolder() ? InArgs._ThumbnailPadding + 5.0f : InArgs._ThumbnailPadding;

	TSharedPtr<SWidget> Thumbnail;
	if ( AssetItem.IsValid() && AssetThumbnail.IsValid() )
	{
		FExtAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.bAllowFadeIn = true;
		ThumbnailConfig.bAllowHintText = InArgs._AllowThumbnailHintLabel;
		ThumbnailConfig.bForceGenericThumbnail = (AssetItem->GetType() == EExtAssetItemType::Creation);
		ThumbnailConfig.bAllowAssetSpecificThumbnailOverlay = true;// (AssetItem->GetType() != EAssetItemType::Creation);
		ThumbnailConfig.ThumbnailLabel = InArgs._ThumbnailLabel;
		ThumbnailConfig.HighlightedText = InArgs._HighlightText;
		ThumbnailConfig.HintColorAndOpacity = InArgs._ThumbnailHintColorAndOpacity;
		Thumbnail = AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig);
	}
	else
	{
		Thumbnail = SNew(SImage) .Image( FAppStyle::GetDefaultBrush() );
	}

	FName ItemShadowBorderName;
	TSharedRef<SWidget> ItemContents = FExtAssetViewItemHelper::CreateTileItemContents(this, Thumbnail.ToSharedRef(), ItemShadowBorderName);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(this, &SExtAssetViewItem::GetBorderImage)
		.Padding(0)
		.AddMetaData<FTagMetaData>(FTagMetaData((AssetItem->GetType() == EExtAssetItemType::Normal) ? StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.ObjectPath : ((AssetItem->GetType() == EExtAssetItemType::Folder) ? FName(*StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath) : NAME_None)))
		[
			SNew(SVerticalBox)

			// Thumbnail
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				// The remainder of the space is reserved for the name.
				SNew(SBox)
				.Padding(ThumbnailPadding - 4.f)
				.WidthOverride(this, &SExtAssetTileItem::GetThumbnailBoxSize)
				.HeightOverride( this, &SExtAssetTileItem::GetThumbnailBoxSize )
				[
					// Drop shadow border
					SNew(SBorder)
					.Padding(4.f)
					.BorderImage(FAppStyle::GetBrush(ItemShadowBorderName))
					[
						ItemContents
					]
				]
			]

			+SVerticalBox::Slot()
			.Padding(FMargin(1.f, 0))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			[
				SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
					.Font( this, &SExtAssetTileItem::GetThumbnailFont )
					.Text( GetNameText() )
					.OnBeginTextEdit(this, &SExtAssetTileItem::HandleBeginNameChange)
					.OnTextCommitted(this, &SExtAssetTileItem::HandleNameCommitted)
					.OnVerifyTextChanged(this, &SExtAssetTileItem::HandleVerifyNameChanged)
					.HighlightText(InArgs._HighlightText)
					.IsSelected(InArgs._IsSelected)
					.IsReadOnly(this, &SExtAssetTileItem::IsNameReadOnly)
					.Justification(ETextJustify::Center)
					.LineBreakPolicy(FBreakIterator::CreateCamelCaseBreakIterator())
			]
		]
	
	];

	if(AssetItem.IsValid())
	{
		AssetItem->RenamedRequestEvent.BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
		AssetItem->RenameCanceledEvent.BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::ExitEditingMode);
	}

	SetForceMipLevelsToBeResident(true);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SExtAssetTileItem::OnAssetDataChanged()
{
	SExtAssetViewItem::OnAssetDataChanged();

	if (AssetItem->GetType() != EExtAssetItemType::Folder && AssetThumbnail.IsValid())
	{
		AssetThumbnail->SetAsset(StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data);
	}
}

float SExtAssetTileItem::GetExtraStateIconWidth() const
{
	return GetStateIconImageSize().Get();
}

FOptionalSize SExtAssetTileItem::GetExtraStateIconMaxWidth() const
{
	return GetThumbnailBoxSize().Get() * 0.8;
}

FOptionalSize SExtAssetTileItem::GetStateIconImageSize() const
{
	float IconSize = GetThumbnailBoxSize().Get() * 0.2;
	return IconSize > 12 ? IconSize : 12;
}

FOptionalSize SExtAssetTileItem::GetThumbnailBoxSize() const
{
	return FOptionalSize(ItemWidth.Get());
}

FSlateFontInfo SExtAssetTileItem::GetThumbnailFont() const
{
	FOptionalSize ThumbSize = GetThumbnailBoxSize();
	if ( ThumbSize.IsSet() )
	{
		float Size = ThumbSize.Get();
		if ( Size < 50 )
		{
			const static FName SmallFontName("ContentBrowser.AssetTileViewNameFontVerySmall");
			return FAppStyle::GetFontStyle(SmallFontName);
		}
		else if ( Size < 85 )
		{
			const static FName SmallFontName("ContentBrowser.AssetTileViewNameFontSmall");
			return FAppStyle::GetFontStyle(SmallFontName);
		}
	}

	const static FName RegularFont("ContentBrowser.AssetTileViewNameFont");
	return FAppStyle::GetFontStyle(RegularFont);
}



///////////////////////////////
// SAssetColumnItem
///////////////////////////////

/** Custom box for the Name column of an asset */
class SAssetColumnItemNameBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SAssetColumnItemNameBox ) {}

		/** The color of the asset  */
		SLATE_ATTRIBUTE( FMargin, Padding )

		/** The widget content presented in the box */
		SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	~SAssetColumnItemNameBox() {}

	void Construct( const FArguments& InArgs, const TSharedRef<SExtAssetColumnItem>& InOwnerAssetColumnItem )
	{
		OwnerAssetColumnItem = InOwnerAssetColumnItem;

		ChildSlot
		[
			SNew(SBox)
			.Padding(InArgs._Padding)
			[
				InArgs._Content.Widget
			]
		];
	}

	virtual TSharedPtr<IToolTip> GetToolTip() override
	{
		if ( OwnerAssetColumnItem.IsValid() )
		{
			return OwnerAssetColumnItem.Pin()->GetToolTip();
		}

		return nullptr;
	}

	/** Forward the event to the view item that this name box belongs to */
	virtual void OnToolTipClosing() override
	{
		if ( OwnerAssetColumnItem.IsValid() )
		{
			OwnerAssetColumnItem.Pin()->OnToolTipClosing();
		}
	}

private:
	TWeakPtr<SExtAssetViewItem> OwnerAssetColumnItem;
};

void SExtAssetColumnItem::Construct( const FArguments& InArgs )
{
	SExtAssetViewItem::Construct( SExtAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.HighlightText(InArgs._HighlightText)
		.OnAssetsOrPathsDragDropped(InArgs._OnAssetsOrPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		.OnIsAssetValidForCustomToolTip(InArgs._OnIsAssetValidForCustomToolTip)
		.OnGetCustomAssetToolTip(InArgs._OnGetCustomAssetToolTip)
		.OnVisualizeAssetToolTip(InArgs._OnVisualizeAssetToolTip)
		.OnAssetToolTipClosing(InArgs._OnAssetToolTipClosing)
		);
	
	HighlightText = InArgs._HighlightText;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SWidget> SExtAssetColumnItem::GenerateWidgetForColumn( const FName& ColumnName, FIsSelected InIsSelected )
{
	TSharedPtr<SWidget> Content;

	// A little right padding so text from this column does not run directly into text from the next.
	static const FMargin ColumnItemPadding( 5, 0, 5, 0 );

	if ( ColumnName == "Name" )
	{
		const FSlateBrush* IconBrush;
		if(IsFolder())
		{
			IconBrush = FAppStyle::GetBrush("ContentBrowser.ColumnViewFolderIcon");
		}
		else
		{
			IconBrush = FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon");
		}

		// Make icon overlays (eg, SCC and dirty status) a reasonable size in relation to the icon size (note: it is assumed this icon is square)
		const float IconOverlaySize = IconBrush->ImageSize.X * 0.6f;

		Content = SNew(SHorizontalBox)
			.AddMetaData<FTagMetaData>(FTagMetaData(AssetItem->GetType() == EExtAssetItemType::Normal ? StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.ObjectPath : NAME_None))
			// Icon
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SOverlay)
				
				// The actual icon
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Image( IconBrush )
					.ColorAndOpacity(this, &SExtAssetColumnItem::GetAssetColor)
				]

				// Source control state
				+SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.WidthOverride(IconOverlaySize)
					.HeightOverride(IconOverlaySize)
					[
						SAssignNew(SCCStateWidget, SLayeredImage)
						.Image(FStyleDefaults::GetNoBrush())
					]
				]

				// Extra external state hook
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(IconOverlaySize)
					.MaxDesiredWidth(IconOverlaySize)
					[
						GenerateExtraStateIconWidget(IconOverlaySize)
					]
				]

				// Dirty state
				+SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBox)
					.WidthOverride(IconOverlaySize)
					.HeightOverride(IconOverlaySize)
					[
						SNew(SImage)
						.Image(this, &SExtAssetColumnItem::GetDirtyImage)
					]
				]
			]

			// Editable Name
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
				.Text( GetNameText() )
				.OnBeginTextEdit(this, &SExtAssetColumnItem::HandleBeginNameChange)
				.OnTextCommitted(this, &SExtAssetColumnItem::HandleNameCommitted)
				.OnVerifyTextChanged(this, &SExtAssetColumnItem::HandleVerifyNameChanged)
				.HighlightText(HighlightText)
				.IsSelected(InIsSelected)
				.IsReadOnly(this, &SExtAssetColumnItem::IsNameReadOnly)
			];

		if(AssetItem.IsValid())
		{
			AssetItem->RenamedRequestEvent.BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
			AssetItem->RenameCanceledEvent.BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::ExitEditingMode);
		}

		return SNew(SBorder)
			.BorderImage(this, &SExtAssetViewItem::GetBorderImage)
			.Padding(0)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				SNew( SAssetColumnItemNameBox, SharedThis(this) )
				.Padding( ColumnItemPadding )
				[
					Content.ToSharedRef()
				]
			];
	}
	else if ( ColumnName == "Class" )
	{
		Content = SAssignNew(ClassText, STextBlock)
			.ToolTipText( this, &SExtAssetColumnItem::GetAssetClassText )
			.Text( GetAssetClassText() )
			.HighlightText( HighlightText );
	}
	else if ( ColumnName == "Path" )
	{
		Content = SAssignNew(PathText, STextBlock)
			.ToolTipText( this, &SExtAssetColumnItem::GetAssetPathText )
			.Text( GetAssetPathText() )
			.HighlightText( HighlightText );
	}
	else
	{
		Content = SNew(STextBlock)
			.ToolTipText( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP(this, &SExtAssetColumnItem::GetAssetTagText, ColumnName) ) )
			.Text( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP(this, &SExtAssetColumnItem::GetAssetTagText, ColumnName) ) );
	}

	return SNew(SBox)
		.Padding( ColumnItemPadding )
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			Content.ToSharedRef()
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SExtAssetColumnItem::OnAssetDataChanged()
{
	SExtAssetViewItem::OnAssetDataChanged();

	if ( ClassText.IsValid() )
	{
		ClassText->SetText( GetAssetClassText() );
	}

	if ( PathText.IsValid() )
	{
		PathText->SetText( GetAssetPathText() );
	}
}

FString SExtAssetColumnItem::GetAssetNameToolTipText() const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() == EExtAssetItemType::Folder)
		{
			FString Result = StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderName.ToString();
			Result += TEXT("\n");
			Result += LOCTEXT("FolderName", "Folder").ToString();

			return Result;
		}
		else
		{
			const FString AssetName = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.AssetName.ToString();
			const FString AssetType = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.AssetClass.ToString();

			FString Result = AssetName;
			Result += TEXT("\n");
			Result += AssetType;

			return Result;
		}
	}
	else
	{
		return FString();
	}
}

FText SExtAssetColumnItem::GetAssetPathText() const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() != EExtAssetItemType::Folder)
		{
			return FText::FromName(StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data.PackageFilePath);
		}
		else
		{
			return FText::FromString(StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath);
		}
	}
	else
	{
		return FText();
	}
}

FText SExtAssetColumnItem::GetAssetTagText(FName AssetTag) const
{
	if ( AssetItem.IsValid() )
	{
		if(AssetItem->GetType() != EExtAssetItemType::Folder)
		{
			const TSharedPtr<FExtAssetViewAsset>& ItemAsAsset = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem);

			// Check custom type
			{
				FText* FoundText = ItemAsAsset->CustomColumnDisplayText.Find(AssetTag);
				if (FoundText)
				{
					return *FoundText;
				}
			}

			// Check display tags
			{
				const FTagDisplayItem* FoundTagItem = CachedDisplayTags.FindByPredicate([AssetTag](const FTagDisplayItem& TagItem)
				{
					return TagItem.TagKey == AssetTag;
				});

				if (FoundTagItem)
				{
					return FoundTagItem->DisplayValue;
				}
			}
		}
	}
	
	return FText();
}

#undef LOCTEXT_NAMESPACE

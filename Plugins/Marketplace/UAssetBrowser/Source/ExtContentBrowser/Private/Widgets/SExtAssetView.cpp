// Copyright 2017-2021 marynate. All Rights Reserved.

#include "SExtAssetView.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtAssetThumbnail.h"
#include "ExtAssetViewWidgets.h"
#include "ExtAssetViewTypes.h"
#include "ExtContentBrowserUtils.h"
#include "ExtContentBrowserStyle.h"
#include "SExtAssetDiscoveryIndicator.h"
#include "ExtFrontendFilterBase.h"

#include "ExtAssetDragDropOp.h"
#include "Adapters/DragDropHandler.h"

#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameViewportClient.h"
#include "Factories/Factory.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Layout/SGridPanel.h"

#include "Framework/Docking/TabManager.h"
#include "EditorStyleSet.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "AssetSelection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "ObjectTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformApplicationMisc.h"

#include "Misc/FileHelper.h"
//#include "Misc/TextFilterUtils.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Materials/Material.h"

//#include "DragDropHandler.h"
#include "DesktopPlatformModule.h"
#include "EditorWidgetsModule.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "LevelEditor.h"


#define LOCTEXT_NAMESPACE "ExtContentBrowser"
#define MAX_THUMBNAIL_SIZE 4096

namespace
{
	/** Time delay between recently added items being added to the filtered asset items list */
	const double TimeBetweenAddingNewAssets = 0.3;//4.0;

	/** Time delay between performing the last jump, and the jump term being reset */
	const double JumpDelaySeconds = 2.0;
}

#if ECB_WIP_INITIAL_ASSET

namespace AssetViewUtils
{
	/** Higher performance version of FAssetData::IsUAsset()
	 * Returns true if this is the primary asset in a package, true for maps and assets but false for secondary objects like class redirectors
	 */
	bool IsUAsset(const FAssetData& Item)
	{
		TextFilterUtils::FNameBufferWithNumber AssetNameBuffer(Item.AssetName);
		TextFilterUtils::FNameBufferWithNumber PackageNameBuffer(Item.PackageName);

		if (PackageNameBuffer.IsWide())
		{
			// Skip to final slash
			const WIDECHAR* LongPackageAssetNameWide = PackageNameBuffer.GetWideNamePtr();
			for (const WIDECHAR* CharPtr = PackageNameBuffer.GetWideNamePtr(); *CharPtr; ++CharPtr)
			{
				if (*CharPtr == '/')
				{
					LongPackageAssetNameWide = CharPtr + 1;
				}
			}

			if (AssetNameBuffer.IsWide())
			{
				return !FCStringWide::Stricmp(LongPackageAssetNameWide, AssetNameBuffer.GetWideNamePtr());
			}
			else if (FCString::IsPureAnsi(LongPackageAssetNameWide))
			{
				// Convert PackageName to ANSI for comparison
				ANSICHAR LongPackageAssetNameAnsi[NAME_WITH_NUMBER_SIZE];
				FPlatformString::Convert(LongPackageAssetNameAnsi, NAME_WITH_NUMBER_SIZE, LongPackageAssetNameWide, FCStringWide::Strlen(LongPackageAssetNameWide) + 1);
				return !FCStringAnsi::Stricmp(LongPackageAssetNameAnsi, AssetNameBuffer.GetAnsiNamePtr());
			}
		}
		else if (!AssetNameBuffer.IsWide())
		{
			// Skip to final slash
			const ANSICHAR* LongPackageAssetNameAnsi = PackageNameBuffer.GetAnsiNamePtr();
			for (const ANSICHAR* CharPtr = PackageNameBuffer.GetAnsiNamePtr(); *CharPtr; ++CharPtr)
			{
				if (*CharPtr == '/')
				{
					LongPackageAssetNameAnsi = CharPtr + 1;
				}
			}

			return !FCStringAnsi::Stricmp(LongPackageAssetNameAnsi, AssetNameBuffer.GetAnsiNamePtr());
		}

		return false;
	}

	class FInitialAssetFilter
	{
	public:
		FInitialAssetFilter(bool InDisplayL10N, bool InDisplayEngine, bool InDisplayPlugins) :
			bDisplayL10N(InDisplayL10N),
			bDisplayEngine(InDisplayEngine),
			bDisplayPlugins(InDisplayPlugins)
		{
			Init(InDisplayL10N, InDisplayEngine, InDisplayPlugins);
		}

		void Init(bool InDisplayL10N, bool InDisplayEngine, bool InDisplayPlugins)
		{
			ObjectRedirectorClassName = UObjectRedirector::StaticClass()->GetFName();

			bDisplayL10N = InDisplayL10N;
			bDisplayEngine = InDisplayEngine;
			bDisplayPlugins = InDisplayPlugins;

			Plugins = IPluginManager::Get().GetEnabledPluginsWithContent();
			PluginNamesUpperWide.Reset(Plugins.Num());
			PluginNamesUpperAnsi.Reset(Plugins.Num());
			PluginLoadedFromEngine.Reset(Plugins.Num());
			for (const TSharedRef<IPlugin>& PluginRef : Plugins)
			{
				FString& PluginNameUpperWide = PluginNamesUpperWide.Add_GetRef(PluginRef->GetName().ToUpper());
				TextFilterUtils::TryConvertWideToAnsi(PluginNameUpperWide, PluginNamesUpperAnsi.AddDefaulted_GetRef());
				PluginLoadedFromEngine.Add(PluginRef->GetLoadedFrom() == EPluginLoadedFrom::Engine);
			}
		}

		FORCEINLINE bool PassesFilter(const FAssetData& Item) const
		{
			if (!PassesRedirectorMainAssetFilter(Item))
			{
				return false;
			}

			return PassesPackagePathFilter(Item.PackagePath);
		}

		FORCEINLINE bool PassesRedirectorMainAssetFilter(const FAssetData& Item) const
		{
			// Do not show redirectors if they are not the main asset in the uasset file.
			if (Item.AssetClass == ObjectRedirectorClassName && !AssetViewUtils::IsUAsset(Item))
			{
				return false;
			}

			return true;
		}

		FORCEINLINE bool PassesPackagePathFilter(const FName& PackagePath) const
		{
			TextFilterUtils::FNameBufferWithNumber ObjectPathBuffer(PackagePath);
			if (ObjectPathBuffer.IsWide())
			{
				return PassesPackagePathFilter(ObjectPathBuffer.GetWideNamePtr());
			}
			else
			{
				return PassesPackagePathFilter(ObjectPathBuffer.GetAnsiNamePtr());
			}
		}

		template <typename CharType>
		FORCEINLINE bool PassesPackagePathFilter(CharType* PackagePath) const
		{
			CharType* PathCh = PackagePath;
			if (*PathCh++ != '/')
			{
				return true;
			}

			for (; *PathCh && *PathCh != '/'; ++PathCh)
			{
				*PathCh = TChar<CharType>::ToUpper(*PathCh);
			}

			if (*PathCh)
			{
				if (!bDisplayL10N)
				{
					if ((PathCh[1] == 'L' || PathCh[1] == 'l') &&
						PathCh[2] == '1' &&
						PathCh[3] == '0' &&
						(PathCh[4] == 'N' || PathCh[4] == 'n') &&
						(PathCh[5] == '/' || PathCh[5] == 0))
					{
						return false;
					}
				}
				*PathCh = 0;
			}

			CharType* PackageRootFolderName = PackagePath + 1;
			int32 PackageRootFolderNameLength = PathCh - PackageRootFolderName;
			if (PackageRootFolderNameLength == 4 && TCString<CharType>::Strncmp(PackageRootFolderName, LITERAL(CharType, "GAME"), 4) == 0)
			{
				return true;
			}
			else if (PackageRootFolderNameLength == 6 && TCString<CharType>::Strncmp(PackageRootFolderName, LITERAL(CharType, "ENGINE"), 4) == 0)
			{
				return bDisplayEngine;
			}
			else
			{
				if (!bDisplayPlugins || !bDisplayEngine)
				{
					int32 PluginIndex = FindPluginNameUpper(PackageRootFolderName, PackageRootFolderNameLength);
					if (PluginIndex != INDEX_NONE)
					{
						if (!bDisplayPlugins)
						{
							return false;
						}
						else if (!bDisplayEngine && PluginLoadedFromEngine[PluginIndex])
						{
							return false;
						}
					}
				}
			}

			return true;
		}

	private:
		FName ObjectRedirectorClassName;
		bool bDisplayL10N;
		bool bDisplayEngine;
		bool bDisplayPlugins;
		TArray<TArray<ANSICHAR>> PluginNamesUpperAnsi;
		TArray<FString> PluginNamesUpperWide;
		TArray<bool> PluginLoadedFromEngine;
		TArray<TSharedRef<IPlugin>> Plugins;

		FORCEINLINE int32 FindPluginNameUpper(const WIDECHAR* PluginNameUpper, int32 Length) const
		{
			int32 i = 0;
			for (const FString& OtherPluginNameUpper : PluginNamesUpperWide)
			{
				if (OtherPluginNameUpper.Len() == Length && TCString<WIDECHAR>::Strcmp(PluginNameUpper, *OtherPluginNameUpper) == 0)
				{
					return i;
				}
				++i;
			}

			return INDEX_NONE;
		}

		FORCEINLINE int32 FindPluginNameUpper(const ANSICHAR* PluginNameUpper, int32 Length) const
		{
			const int32 LengthWithNull = Length + 1;
			int32 i = 0;
			for (const TArray<ANSICHAR>& OtherPluginNameUpper : PluginNamesUpperAnsi)
			{
				if (OtherPluginNameUpper.Num() == LengthWithNull && TCString<ANSICHAR>::Strcmp(PluginNameUpper, OtherPluginNameUpper.GetData()) == 0)
				{
					return i;
				}
				++i;
			}

			return INDEX_NONE;
		}
	};
} // namespace AssetViewUtils
#endif


/////////////////////////////////////////////////////////////
// WidgetHelpers implementation
//

namespace WidgetHelpers
{
	TSharedRef<SWidget> CreateColorWidget(FLinearColor* Color)
	{
		auto GetValue = [=] {
			return *Color;
		};

		auto SetValue = [=](FLinearColor NewColor) {
			*Color = NewColor;
		};

		auto OnGetMenuContent = [=]() -> TSharedRef<SWidget> {
			// Open a color picker
			return SNew(SColorPicker)
				.TargetColorAttribute_Lambda(GetValue)
				.UseAlpha(true)
				.DisplayInlineVersion(true)
				.OnColorCommitted_Lambda(SetValue);
		};

		return SNew(SComboButton)
			.ContentPadding(0)
			.HasDownArrow(false)
			.ButtonStyle(FAppStyle::Get(), "Sequencer.AnimationOutliner.ColorStrip")
			.OnGetMenuContent_Lambda(OnGetMenuContent)
			.CollapseMenuOnParentFocus(true)
			.ButtonContent()
			[
				SNew(SColorBlock)
				.Color_Lambda(GetValue)
				.ShowBackgroundForAlpha(true)
				.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
				.Size(FVector2D(10.0f, 10.0f))
			];
	}
}


/////////////////////////////////////////////////////////////
// SExtAssetView implementation
//

SExtAssetView::~SExtAssetView()
{
	// Remove the listener for when view settings are changed
	UExtContentBrowserSettings::OnSettingChanged().RemoveAll(this);

	if ( FrontendFilters.IsValid() )
	{
		// Clear the frontend filter changed delegate
		FrontendFilters->OnChanged().RemoveAll( this );
	}

	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathAdded().RemoveAll(this);
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathRemoved().RemoveAll(this);

#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	FExtContentBrowserSingleton::GetAssetRegistry().OnAssetGathered().RemoveAll(this);
#endif

#if ECB_WIP_REPARSE_ASSET
	FExtContentBrowserSingleton::GetAssetRegistry().OnAssetUpdated().RemoveAll(this);
#endif

	// Release all rendering resources being held onto
	AssetThumbnailPool.Reset();
}



BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SExtAssetView::Construct( const FArguments& InArgs )
{
	bIsWorking = false;
	TotalAmortizeTime = 0;
	AmortizeStartTime = 0;
	MaxSecondsPerFrame = 0.015;

	bFillEmptySpaceInTileView = InArgs._FillEmptySpaceInTileView;
	FillScale = 1.0f;

	ThumbnailHintFadeInSequence.JumpToStart();
	ThumbnailHintFadeInSequence.AddCurve(0, 0.5f, ECurveEaseFunction::Linear);

#if ECB_LEGACY
	// Load the asset registry module to listen for updates
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnAssetAdded().AddSP( this, &SExtAssetView::OnAssetAdded );
	AssetRegistryModule.Get().OnAssetRemoved().AddSP( this, &SExtAssetView::OnAssetRemoved );
	AssetRegistryModule.Get().OnAssetRenamed().AddSP( this, &SExtAssetView::OnAssetRenamed );
	AssetRegistryModule.Get().OnAssetUpdated().AddSP( this, &SExtAssetView::OnAssetUpdated );
	AssetRegistryModule.Get().OnPathAdded().AddSP( this, &SExtAssetView::OnAssetRegistryPathAdded );
	AssetRegistryModule.Get().OnPathRemoved().AddSP( this, &SExtAssetView::OnAssetRegistryPathRemoved );
#endif

#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	FExtContentBrowserSingleton::GetAssetRegistry().OnAssetGathered().AddSP(this, &SExtAssetView::OnAssetRegistryAssetGathered);
#endif

#if ECB_WIP_REPARSE_ASSET
	FExtContentBrowserSingleton::GetAssetRegistry().OnAssetUpdated().AddSP(this, &SExtAssetView::OnAssetUpdated);
#endif

	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathAdded().AddSP(this, &SExtAssetView::OnAssetRegistryRootPathAdded);
	FExtContentBrowserSingleton::GetAssetRegistry().OnRootPathRemoved().AddSP(this, &SExtAssetView::OnAssetRegistryRootPathRemoved);

#if ECB_WIP_COLLECTION
	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	CollectionManagerModule.Get().OnAssetsAdded().AddSP( this, &SExtAssetView::OnAssetsAddedToCollection );
	CollectionManagerModule.Get().OnAssetsRemoved().AddSP( this, &SExtAssetView::OnAssetsRemovedFromCollection );
	CollectionManagerModule.Get().OnCollectionRenamed().AddSP( this, &SExtAssetView::OnCollectionRenamed );
	CollectionManagerModule.Get().OnCollectionUpdated().AddSP( this, &SExtAssetView::OnCollectionUpdated );
#endif

#if ECB_LEGACY
	// Listen for when assets are loaded or changed to update item data
	FCoreUObjectDelegates::OnAssetLoaded.AddSP(this, &SExtAssetView::OnAssetLoaded);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SExtAssetView::OnObjectPropertyChanged);

	// Listen to find out when previously empty paths are populated with content
	{
		TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();
		EmptyFolderVisibilityManager->OnFolderPopulated().AddSP(this, &SExtAssetView::OnFolderPopulated);
	}
#endif
	// Listen for when view settings are changed
	UExtContentBrowserSettings::OnSettingChanged().AddSP(this, &SExtAssetView::HandleSettingChanged);

	// Get desktop metrics
	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetCachedDisplayMetrics( DisplayMetrics );

	const FVector2D DisplaySize(
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top );

	const float ThumbnailScaleRangeScalar = ( DisplaySize.Y / 1080 );

	// Create a thumbnail pool for rendering thumbnails	
	AssetThumbnailPool = MakeShareable( new FExtAssetThumbnailPool(1024, /*InAreRealTimeThumbnailsAllowed = */ false) );
	NumOffscreenThumbnails = 64;
	ListViewThumbnailResolution = 128;
	ListViewThumbnailSize = 64;
	ListViewThumbnailPadding = 4;
	TileViewThumbnailResolution = 256;
	TileViewThumbnailSize = 128;
	TileViewThumbnailPadding = 5;

	TileViewNameHeight = 36;
	ThumbnailScaleSliderValue = InArgs._ThumbnailScale; 

	if ( !ThumbnailScaleSliderValue.IsBound() )
	{
		ThumbnailScaleSliderValue = FMath::Clamp<float>(ThumbnailScaleSliderValue.Get(), 0.0f, 1.0f);
	}

	MinThumbnailScale = 0.2f * ThumbnailScaleRangeScalar;
	MaxThumbnailScale = 2.0f * ThumbnailScaleRangeScalar;

	bCanShowFolders = InArgs._CanShowFolders;
	bFilterRecursivelyWithBackendFilter = InArgs._FilterRecursivelyWithBackendFilter;
	bCanShowCollections = InArgs._CanShowCollections;
	bCanShowFavorites = InArgs._CanShowFavorites;
#if ECB_TODO
	bPreloadAssetsForContextMenu = InArgs._PreloadAssetsForContextMenu;
#endif
	SelectionMode = InArgs._SelectionMode;

	bShowMajorAssetTypeColumnsInColumnView = InArgs._ShowMajorAssetTypeColumnsInColumnView;
	bShowPathInColumnView = InArgs._ShowPathInColumnView;
	bShowTypeInColumnView = InArgs._ShowTypeInColumnView;
	bSortByPathInColumnView = bShowPathInColumnView & InArgs._SortByPathInColumnView;

	bPendingUpdateThumbnails = false;
	bShouldNotifyNextAssetSync = true;
	CurrentThumbnailSize = TileViewThumbnailSize;

	SourcesData = InArgs._InitialSourcesData;
	BackendFilter = InArgs._InitialBackendFilter;

	FrontendFilters = InArgs._FrontendFilters;
	if ( FrontendFilters.IsValid() )
	{
		FrontendFilters->OnChanged().AddSP( this, &SExtAssetView::OnFrontendFiltersChanged );
	}

	OnShouldFilterAsset = InArgs._OnShouldFilterAsset;

	OnAssetSelected = InArgs._OnAssetSelected;

	OnAssetsActivated = InArgs._OnAssetsActivated;
	OnGetAssetContextMenu = InArgs._OnGetAssetContextMenu;

#if ECB_LEGACY
	OnGetFolderContextMenu = InArgs._OnGetFolderContextMenu;
	OnGetPathContextMenuExtender = InArgs._OnGetPathContextMenuExtender;

	OnFindInAssetTreeRequested = InArgs._OnFindInAssetTreeRequested;
	OnAssetRenameCommitted = InArgs._OnAssetRenameCommitted;
	OnAssetTagWantsToBeDisplayed = InArgs._OnAssetTagWantsToBeDisplayed;
	OnIsAssetValidForCustomToolTip = InArgs._OnIsAssetValidForCustomToolTip;
	OnGetCustomAssetToolTip = InArgs._OnGetCustomAssetToolTip;
	OnVisualizeAssetToolTip = InArgs._OnVisualizeAssetToolTip;
	OnAssetToolTipClosing = InArgs._OnAssetToolTipClosing;
	OnGetCustomSourceAssets = InArgs._OnGetCustomSourceAssets;
#endif
	HighlightedText = InArgs._HighlightedText;
	ThumbnailLabel = InArgs._ThumbnailLabel;
	AllowThumbnailHintLabel = InArgs._AllowThumbnailHintLabel;
	AssetShowWarningText = InArgs._AssetShowWarningText;

	bAllowDragging = InArgs._AllowDragging;
#if ECB_LEGACY
	bAllowFocusOnSync = InArgs._AllowFocusOnSync;
#endif
	OnPathSelected = InArgs._OnPathSelected;

	HiddenColumnNames = DefaultHiddenColumnNames = InArgs._HiddenColumnNames;
	CustomColumns = InArgs._CustomColumns;

	OnSearchOptionsChanged = InArgs._OnSearchOptionsChanged;

	OnRequestShowChangeLog = InArgs._OnRequestShowChangeLog;

	if ( InArgs._InitialViewType >= 0 && InArgs._InitialViewType < EAssetViewType::MAX )
	{
		CurrentViewType = InArgs._InitialViewType;
	}
	else
	{
		CurrentViewType = EAssetViewType::Tile;
	}

#if ECB_TODO
	bPendingSortFilteredItems = false;
	bQuickFrontendListRefreshRequested = false;
	bSlowFullListRefreshRequested = false;
	LastSortTime = 0;
	SortDelaySeconds = 8;

	LastProcessAddsTime = 0;
#endif
	bBulkSelecting = false;
	bUserSearching = false;
	bPendingFocusOnSync = false;

	bWereItemsRecursivelyFiltered = false;
	NumVisibleColumns = 0;

#if ECB_LEGACY
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);
#endif

	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	ChildSlot
	[
		VerticalBox
	];

	// Assets area
	VerticalBox->AddSlot()
	.FillHeight(1.f)
	[
		SNew( SVerticalBox ) 

		// Working Bar >>
#if ECB_FOLD 
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SBox )
			.Visibility_Lambda([this] { return bIsWorking ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			.HeightOverride( 2 )
			[
				SNew( SProgressBar )
				.Percent( this, &SExtAssetView::GetIsWorkingProgressBarState )
				.BorderPadding( FVector2D(0,0) )
			]
		]
#endif	// Working Bar <<

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				// Container for the view types
				SAssignNew(ViewContainer, SBorder)
				.Padding(0)
			]

			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(FMargin(0, 14, 0, 0))
			[
				// A warning to display when there are no assets to show
				SNew( STextBlock ).Justification( ETextJustify::Center ).AutoWrapText(true)
				.Text( this, &SExtAssetView::GetAssetShowWarningText )
				.Visibility( this, &SExtAssetView::IsAssetShowWarningTextVisible )
			]
#if 0 //ECB_WIP_ASYNC_ASSET_DISCOVERY
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(FMargin(24, 0, 24, 0))
			[
				// Asset discovery indicator
				SNew(SExtAssetDiscoveryIndicator)
				.ScaleMode(EAssetDiscoveryIndicatorScaleMode::Scale_None)
				.Padding(FMargin(2))
				.FadeIn(false/*bFadeIn*/)
			]
#endif

#if ECB_LEGACY
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(8, 0))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ErrorReporting.EmptyBox"))
				.BorderBackgroundColor(this, &SExtAssetView::GetQuickJumpColor)
				.Visibility(this, &SExtAssetView::IsQuickJumpVisible)
				[
					SNew(STextBlock)
					.Text(this, &SExtAssetView::GetQuickJumpTerm)
				]
			]
#endif
		]
	];

	if (InArgs._ShowBottomToolbar)
	{
		// Bottom panel
		VerticalBox->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)

			// Asset count
			+SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0)
				[
					SNew(STextBlock).Text(this, &SExtAssetView::GetAssetCountText)
				]
#if ECB_FEA_ASYNC_ASSET_DISCOVERY
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(SExtAssetDiscoveryIndicator)
					.ScaleMode(EAssetDiscoveryIndicatorScaleMode::Scale_None)
					.Padding(FMargin(2))
					.FadeIn(true/*bFadeIn*/)
				]
#endif
			]

			// View mode combo button
			+SHorizontalBox::Slot().AutoWidth()
			[

				SNew(SHorizontalBox)

				// Thumbnail Scale
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SAssignNew(ThumbnailScaleContainer, SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("ThumbnailScale", "Thumbnail Scale"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(120.0f)
						[
							SNew(SSlider)
							.ToolTipText(LOCTEXT("ThumbnailScaleToolTip", "Adjust the size of thumbnails."))
							.Value(this, &SExtAssetView::GetThumbnailScale)
							.OnValueChanged(this, &SExtAssetView::SetThumbnailScale)
							.Locked(this, &SExtAssetView::IsThumbnailScalingLocked)
						]
					]
				]

				// View options
				+SHorizontalBox::Slot().AutoWidth()
				[
					SAssignNew(ViewOptionsComboButton, SComboButton)
					.ContentPadding(0)
					.ForegroundColor(this, &SExtAssetView::GetViewButtonForegroundColor)
					.ButtonStyle(FAppStyle::Get(), "ToggleButton") // Use the tool bar item style for this button
					.OnGetMenuContent(this, &SExtAssetView::GetViewButtonContent)
					.ButtonContent()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage).Image(FAppStyle::GetBrush("GenericViewButton"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(LOCTEXT("ViewButton", "View Options"))
						]
					]
				]
			]
		];
	}

	CreateCurrentView();
#if ECB_WIP_INITIAL_ASSET
	if( InArgs._InitialAssetSelection.IsValid() )
	{
		// sync to the initial item without notifying of selection
		bShouldNotifyNextAssetSync = false;
		TArray<FAssetData> AssetsToSync;
		AssetsToSync.Add( InArgs._InitialAssetSelection );
		SyncToAssets( AssetsToSync );
	}
#endif

#if ECB_WIP_MORE_VIEWTYPE

	// If currently looking at column, and you could choose to sort by path in column first and then name
	// Generalizing this is a bit difficult because the column ID is not accessible or is not known
	// Currently I assume this won't work, if this view mode is not column. Otherwise, I don't think sorting by path
	// is a good idea. 
	if (CurrentViewType == EAssetViewType::Column && bSortByPathInColumnView)
	{
		SortManager.SetSortColumnId(EColumnSortPriority::Primary, SortManager.PathColumnId);
		SortManager.SetSortColumnId(EColumnSortPriority::Secondary, SortManager.NameColumnId);
		SortManager.SetSortMode(EColumnSortPriority::Primary, EColumnSortMode::Ascending);
		SortManager.SetSortMode(EColumnSortPriority::Secondary, EColumnSortMode::Ascending);
		SortList();
	}
#endif

#if ECB_FEA_ASSET_DRAG_DROP
	static bool bInitAssetViewDragAndDropExtenders = false;
	if (!bInitAssetViewDragAndDropExtenders)
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetViewDragAndDropExtender>& AssetViewDragAndDropExtenders = ContentBrowserModule.GetAssetViewDragAndDropExtenders();

		FAssetViewDragAndDropExtender DragAndDropExtender(
			FAssetViewDragAndDropExtender::FOnDropDelegate::CreateSP(this, &SExtAssetView::OnDropAndDropToAssetView)
			, FAssetViewDragAndDropExtender::FOnDragOverDelegate::CreateLambda([](const FAssetViewDragAndDropExtender::FPayload&) {return true; })
			, FAssetViewDragAndDropExtender::FOnDragLeaveDelegate::CreateLambda([](const FAssetViewDragAndDropExtender::FPayload&) {return true; })
		);
		AssetViewDragAndDropExtenders.Add(DragAndDropExtender);

		bInitAssetViewDragAndDropExtenders = true;
	}

	static bool bInitLevelViewportDragAndDropExtenders = false;
	if (!bInitLevelViewportDragAndDropExtenders)
	{
		// Get all menu extenders for this context menu from the level editor module
		static const FName LevelEditorName("LevelEditor");
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(LevelEditorName);
		TArray<FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors>& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();

		MenuExtenders.Add(FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateSP(this, &SExtAssetView::OnExtendLevelViewportMenu));
		//FDelegateHandle LevelViewportContextMenuExtenderDelegateHandle = ContextMenuExtenders.Last().GetHandle();
	}
#endif
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TOptional< float > SExtAssetView::GetIsWorkingProgressBarState() const
{
	return bIsWorking ? TOptional< float >() : 0.0f; 
}

void SExtAssetView::SetSourcesData(const FSourcesData& InSourcesData)
{
	// Update the path and collection lists
	SourcesData = InSourcesData;
	RequestSlowFullListRefresh();
	ClearSelection();
}

const FSourcesData& SExtAssetView::GetSourcesData() const
{
	return SourcesData;
}
#if ECB_TODO
bool SExtAssetView::IsAssetPathSelected() const
{
	int32 NumAssetPaths, NumClassPaths;
	ContentBrowserUtils::CountPathTypes(SourcesData.PackagePaths, NumAssetPaths, NumClassPaths);

	// Check that only asset paths are selected
	return NumAssetPaths > 0 && NumClassPaths == 0;
}
#endif
void SExtAssetView::SetBackendFilter(const FARFilter& InBackendFilter)
{
	// Update the path and collection lists
	BackendFilter = InBackendFilter;
	RequestSlowFullListRefresh();
}
#if ECB_TODO
void SExtAssetView::RenameFolder(const FString& FolderToRename)
{
	for ( auto ItemIt = FilteredAssetItems.CreateConstIterator(); ItemIt; ++ItemIt )
	{
		const TSharedPtr<FExtAssetViewItem>& Item = *ItemIt;
		if ( Item.IsValid() && Item->GetType() == EAssetItemType::Folder )	
		{
			const TSharedPtr<FAssetViewFolder>& ItemAsFolder = StaticCastSharedPtr<FAssetViewFolder>(Item);
			if ( ItemAsFolder->FolderPath == FolderToRename )
			{
				ItemAsFolder->bRenameWhenScrolledIntoview = true;

				SetSelection(Item);
				RequestScrollIntoView(Item);
				break;
			}
		}
	}
}

void SExtAssetView::SyncToAssets( const TArray<FAssetData>& AssetDataList, const bool bFocusOnSync )
{
	PendingSyncItems.Reset();
	for (const FAssetData& AssetData : AssetDataList)
	{
		PendingSyncItems.SelectedAssets.Add(AssetData.ObjectPath);
	}

	bPendingFocusOnSync = bFocusOnSync;
}

void SExtAssetView::SyncToFolders(const TArray<FString>& FolderList, const bool bFocusOnSync)
{
	PendingSyncItems.Reset();
	PendingSyncItems.SelectedFolders = TSet<FString>(FolderList);

	bPendingFocusOnSync = bFocusOnSync;
}

void SExtAssetView::SyncTo(const FExtContentBrowserSelection& ItemSelection, const bool bFocusOnSync)
{
	PendingSyncItems.Reset();
	PendingSyncItems.SelectedFolders = TSet<FString>(ItemSelection.SelectedFolders);
	for (const FAssetData& AssetData : ItemSelection.SelectedAssets)
	{
		PendingSyncItems.SelectedAssets.Add(AssetData.ObjectPath);
	}

	bPendingFocusOnSync = bFocusOnSync;
}
#endif
void SExtAssetView::SyncToSelection( const bool bFocusOnSync )
{
	PendingSyncItems.Reset();

	TArray<TSharedPtr<FExtAssetViewItem>> SelectedItems = GetSelectedItems();
	for (const TSharedPtr<FExtAssetViewItem>& Item : SelectedItems)
	{
		if (Item.IsValid())
		{
			if (Item->GetType() == EExtAssetItemType::Folder)
			{
				PendingSyncItems.SelectedFolders.Add(StaticCastSharedPtr<FExtAssetViewFolder>(Item)->FolderPath);
			}
			else
			{
				PendingSyncItems.SelectedAssets.Add(StaticCastSharedPtr<FExtAssetViewAsset>(Item)->Data.ObjectPath);
			}
		}
	}

	bPendingFocusOnSync = bFocusOnSync;
}
#if ECB_WIP_HISTORY
void SExtAssetView::ApplyHistoryData( const FHistoryData& History )
{
	SetSourcesData(History.SourcesData);
	PendingSyncItems = History.SelectionData;
	bPendingFocusOnSync = true;
}
#endif
TArray<TSharedPtr<FExtAssetViewItem>> SExtAssetView::GetSelectedItems() const
{
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: return ListView->GetSelectedItems();
		case EAssetViewType::Tile: return TileView->GetSelectedItems();
		case EAssetViewType::Column: return ColumnView->GetSelectedItems();
		default:
		ensure(0); // Unknown list type
		return TArray<TSharedPtr<FExtAssetViewItem>>();
	}
}

TArray<FExtAssetData> SExtAssetView::GetSelectedAssets() const
{
	TArray<TSharedPtr<FExtAssetViewItem>> SelectedItems = GetSelectedItems();
	TArray<FExtAssetData> SelectedAssets;
	for ( auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt )
	{
		const TSharedPtr<FExtAssetViewItem>& Item = *ItemIt;

		// Only report non-temporary & non-folder items
		if ( Item.IsValid() && !Item->IsTemporaryItem() && Item->GetType() != EExtAssetItemType::Folder )	
		{
			SelectedAssets.Add(StaticCastSharedPtr<FExtAssetViewAsset>(Item)->Data);
		}
	}

	return SelectedAssets;
}

int32 SExtAssetView::GetNumSelectedAssets() const
{
	int32 Num = 0;
	TArray<TSharedPtr<FExtAssetViewItem>> SelectedItems = GetSelectedItems();
	for (auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt)
	{
		const TSharedPtr<FExtAssetViewItem>& Item = *ItemIt;

		// Only report non-temporary & non-folder items
		if (Item.IsValid() && !Item->IsTemporaryItem() && Item->GetType() != EExtAssetItemType::Folder)
		{
			++Num;
		}
	}

	return Num;
}

TArray<FString> SExtAssetView::GetSelectedFolders() const
{
	TArray<TSharedPtr<FExtAssetViewItem>> SelectedItems = GetSelectedItems();
	TArray<FString> SelectedFolders;
	for ( auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt )
	{
		const TSharedPtr<FExtAssetViewItem>& Item = *ItemIt;
		if ( Item.IsValid() && Item->GetType() == EExtAssetItemType::Folder )	
		{
			SelectedFolders.Add(StaticCastSharedPtr<FExtAssetViewFolder>(Item)->FolderPath);
		}
	}

	return SelectedFolders;
}

void SExtAssetView::RequestSlowFullListRefresh()
{
	bSlowFullListRefreshRequested = true;

	// Prefetch assets
	{
		const bool bRecurse = ShouldFilterRecursively();
		const bool bUsingFolders = IsShowingFolders();
		FARFilter Filter = SourcesData.MakeFilter(bRecurse, bUsingFolders);
		
		// Add the backend filters from the filter list
		Filter.Append(BackendFilter);
#if ECB_FEA_ASYNC_ASSET_DISCOVERY		
		//FExtContentBrowserSingleton::GetAssetRegistry().CacheAssetsAsync(Filter);
#else
		FExtContentBrowserSingleton::GetAssetRegistry().CacheAssets(Filter);
#endif
	}
}

void SExtAssetView::RequestQuickFrontendListRefresh()
{
	bQuickFrontendListRefreshRequested = true;
}

FString SExtAssetView::GetThumbnailScaleSettingPath(const FString& SettingsString) const
{
	return SettingsString + TEXT(".ThumbnailSizeScale");
}

FString SExtAssetView::GetCurrentViewTypeSettingPath(const FString& SettingsString) const
{
	return SettingsString + TEXT(".CurrentViewType");
}

void SExtAssetView::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	GConfig->SetFloat(*IniSection, *GetThumbnailScaleSettingPath(SettingsString), ThumbnailScaleSliderValue.Get(), IniFilename);
#if ECB_WIP_MORE_VIEWTYPE
	GConfig->SetInt(*IniSection, *GetCurrentViewTypeSettingPath(SettingsString), CurrentViewType, IniFilename);
#endif
	
	GConfig->SetArray(*IniSection, *(SettingsString + TEXT(".HiddenColumns")), HiddenColumnNames, IniFilename);
}

void SExtAssetView::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	float Scale = 0.f;
	if ( GConfig->GetFloat(*IniSection, *GetThumbnailScaleSettingPath(SettingsString), Scale, IniFilename) )
	{
		// Clamp value to normal range and update state
		Scale = FMath::Clamp<float>(Scale, 0.f, 1.f);
		SetThumbnailScale(Scale);
	}

#if ECB_WIP_MORE_VIEWTYPE
	int32 ViewType = EAssetViewType::Tile;
	if ( GConfig->GetInt(*IniSection, *GetCurrentViewTypeSettingPath(SettingsString), ViewType, IniFilename) )
	{
		// Clamp value to normal range and update state
		if ( ViewType < 0 || ViewType >= EAssetViewType::MAX)
		{
			ViewType = EAssetViewType::Tile;
		}
		SetCurrentViewType( (EAssetViewType::Type)ViewType );
	}

	TArray<FString> LoadedHiddenColumnNames;
	GConfig->GetArray(*IniSection, *(SettingsString + TEXT(".HiddenColumns")), LoadedHiddenColumnNames, IniFilename);
	if (LoadedHiddenColumnNames.Num() > 0)
	{
		HiddenColumnNames = LoadedHiddenColumnNames;
	}
#endif
}

#if ECB_TODO

// Adjusts the selected asset by the selection delta, which should be +1 or -1)
void SExtAssetView::AdjustActiveSelection(int32 SelectionDelta)
{
	// Find the index of the first selected item
	TArray<TSharedPtr<FExtAssetViewItem>> SelectionSet = GetSelectedItems();
	
	int32 SelectedSuggestion = INDEX_NONE;

	if (SelectionSet.Num() > 0)
	{
		if (!FilteredAssetItems.Find(SelectionSet[0], /*out*/ SelectedSuggestion))
		{
			// Should never happen
			ensureMsgf(false, TEXT("SExtAssetView has a selected item that wasn't in the filtered list"));
			return;
		}
	}
	else
	{
		SelectedSuggestion = 0;
		SelectionDelta = 0;
	}

	if (FilteredAssetItems.Num() > 0)
	{
		// Move up or down one, wrapping around
		SelectedSuggestion = (SelectedSuggestion + SelectionDelta + FilteredAssetItems.Num()) % FilteredAssetItems.Num();

		// Pick the new asset
		const TSharedPtr<FExtAssetViewItem>& NewSelection = FilteredAssetItems[SelectedSuggestion];

		RequestScrollIntoView(NewSelection);
		SetSelection(NewSelection);
	}
	else
	{
		ClearSelection();
	}
}

#endif

#if ECB_FEA_ASYNC_ASSET_DISCOVERY

void SExtAssetView::ProcessRecentlyLoadedOrChangedAssets()
{
	for (int32 AssetIdx = FilteredAssetItems.Num() - 1; AssetIdx >= 0 && RecentlyLoadedOrChangedAssets.Num() > 0; --AssetIdx)
	{
		if (FilteredAssetItems[AssetIdx]->GetType() != EExtAssetItemType::Folder)
		{
			const TSharedPtr<FExtAssetViewAsset>& ItemAsAsset = StaticCastSharedPtr<FExtAssetViewAsset>(FilteredAssetItems[AssetIdx]);
				
			// Find the updated version of the asset data from the set
			// This is the version of the data we should use to update our view
			FExtAssetData RecentlyLoadedOrChangedAsset;
			if (const FExtAssetData* RecentlyLoadedOrChangedAssetPtr = RecentlyLoadedOrChangedAssets.Find(ItemAsAsset->Data))
			{
				RecentlyLoadedOrChangedAsset = *RecentlyLoadedOrChangedAssetPtr;
				RecentlyLoadedOrChangedAssets.Remove(ItemAsAsset->Data);
			}

			if (RecentlyLoadedOrChangedAsset.IsValid() || RecentlyLoadedOrChangedAsset.IsParsed())
			{
				bool bShouldRemoveAsset = false;

				if (!PassesCurrentBackendFilter(RecentlyLoadedOrChangedAsset))
				{
					bShouldRemoveAsset = true;
				}

				if (!bShouldRemoveAsset && OnShouldFilterAsset.IsBound() && OnShouldFilterAsset.Execute(RecentlyLoadedOrChangedAsset))
				{
					bShouldRemoveAsset = true;
				}

				if (!bShouldRemoveAsset && (IsFrontendFilterActive() && !PassesCurrentFrontendFilter(RecentlyLoadedOrChangedAsset)))
				{
					bShouldRemoveAsset = true;
				}

				if (bShouldRemoveAsset)
				{
					FilteredAssetItems.RemoveAt(AssetIdx);
				}
				else
				{
					// Update the asset data on the item
					ItemAsAsset->SetAssetData(RecentlyLoadedOrChangedAsset);

					// Update the custom column data
					ItemAsAsset->CacheCustomColumns(CustomColumns, true, true, true);
				}

				RefreshList();
			}
		}
	}

	if (FilteredRecentlyAddedAssets.Num() == 0 && RecentlyAddedAssets.Num() == 0)
	{
		//No more assets coming in so if we haven't found them now we aren't going to
		RecentlyLoadedOrChangedAssets.Reset();
	}
}

#endif

void SExtAssetView::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	CalculateFillScale( AllottedGeometry );

	CurrentTime = InCurrentTime;

	static float WidthLastFrame = 0.f;
	if (WidthLastFrame != AllottedGeometry.Size.X)
	{
		ECB_LOG(Display, TEXT("AssetView:WidthLastFrame: %.2f"), WidthLastFrame);
		WidthLastFrame = AllottedGeometry.Size.X;
	}

	{
#if ECB_FEA_ASYNC_ASSET_DISCOVERY
		bool bIsGatheringAsset = FExtContentBrowserSingleton::GetAssetRegistry().IsGatheringAssets();
#else
		bool bIsGatheringAsset = false;
#endif

		ThumbnailScaleContainer->SetVisibility(WidthLastFrame < (bIsGatheringAsset ? 609.f : 372.f) ? EVisibility::Collapsed : EVisibility::Visible);

		ViewOptionsComboButton->SetVisibility(WidthLastFrame < (bIsGatheringAsset ? 405.f : 150.f) ? EVisibility::Collapsed : EVisibility::Visible);
	}
	

#if ECB_FEA_ASYNC_ASSET_DISCOVERY	
	if (FExtContentBrowserSingleton::GetAssetRegistry().GetAndTrimAssetGatherResult())
	{
		//bSlowFullListRefreshRequested = true;
	}

	// If there were any assets that were recently added via the asset registry, process them now
	ProcessRecentlyAddedAssets();

	// If there were any assets loaded since last frame that we are currently displaying thumbnails for, push them on the render stack now.
	ProcessRecentlyLoadedOrChangedAssets();
#endif

	CalculateThumbnailHintColorAndOpacity();

	if (FSlateApplication::Get().GetActiveModalWindow().IsValid())
	{
		// If we're in a model window then we need to tick the thumbnail pool in order for thumbnails to render correctly.
		AssetThumbnailPool->Tick(InDeltaTime);
	}

	if ( bPendingUpdateThumbnails )
	{
		UpdateThumbnails();
		bPendingUpdateThumbnails = false;
	}

	if (bSlowFullListRefreshRequested)
	{
		RefreshSourceItems();
		bSlowFullListRefreshRequested = false;
		bQuickFrontendListRefreshRequested = true;
	}

	if (QueriedAssetItems.Num() > 0)
	{
		check(OnShouldFilterAsset.IsBound());
		double TickStartTime = FPlatformTime::Seconds();

		// Mark the first amortize time
		if (AmortizeStartTime == 0)
		{
			AmortizeStartTime = FPlatformTime::Seconds();
			bIsWorking = true;
		}

		ProcessQueriedItems(TickStartTime);

		if (QueriedAssetItems.Num() == 0)
		{
			TotalAmortizeTime += FPlatformTime::Seconds() - AmortizeStartTime;
			AmortizeStartTime = 0;
			bIsWorking = false;
		}
		else
		{
			// Need to finish processing queried items before rest of function is safe
			return;
		}
	}

	if (bQuickFrontendListRefreshRequested)
	{
		//ResetQuickJump();
		
		RefreshFilteredItems();
		RefreshFolders();
		SortList(!PendingSyncItems.Num()); // Don't sync to selection if we are just going to do it below

		bQuickFrontendListRefreshRequested = false;
	}

	if ( PendingSyncItems.Num() > 0 )
	{
		if (bPendingSortFilteredItems)
		{
			// Don't sync to selection because we are just going to do it below
			SortList(/*bSyncToSelection=*/false);
		}
		
		bBulkSelecting = true;
		ClearSelection();
		bool bFoundScrollIntoViewTarget = false;

		for ( auto ItemIt = FilteredAssetItems.CreateConstIterator(); ItemIt; ++ItemIt )
		{
			const auto& Item = *ItemIt;
			if(Item.IsValid())
			{
				if(Item->GetType() == EExtAssetItemType::Folder)
				{
					const TSharedPtr<FExtAssetViewFolder>& ItemAsFolder = StaticCastSharedPtr<FExtAssetViewFolder>(Item);
					if ( PendingSyncItems.SelectedFolders.Contains(ItemAsFolder->FolderPath) )
					{
						SetItemSelection(*ItemIt, true, ESelectInfo::OnNavigation);

						// Scroll the first item in the list that can be shown into view
						if ( !bFoundScrollIntoViewTarget )
						{
							RequestScrollIntoView(Item);
							bFoundScrollIntoViewTarget = true;
						}
					}
				}
				else
				{
					const TSharedPtr<FExtAssetViewAsset>& ItemAsAsset = StaticCastSharedPtr<FExtAssetViewAsset>(Item);
					if ( PendingSyncItems.SelectedAssets.Contains(ItemAsAsset->Data.ObjectPath) )
					{
						SetItemSelection(*ItemIt, true, ESelectInfo::OnNavigation);

						// Scroll the first item in the list that can be shown into view
						if ( !bFoundScrollIntoViewTarget )
						{
							RequestScrollIntoView(Item);
							bFoundScrollIntoViewTarget = true;
						}
					}
				}
			}
		}
	
		bBulkSelecting = false;

		if (bShouldNotifyNextAssetSync && !bUserSearching)
		{
			AssetSelectionChanged(TSharedPtr<FExtAssetViewAsset>(), ESelectInfo::Direct);
		}

		// Default to always notifying
		bShouldNotifyNextAssetSync = true;

		PendingSyncItems.Reset();

		if (bAllowFocusOnSync && bPendingFocusOnSync)
		{
			FocusList();
		}
	}

	if ( IsHovered() )
	{
		// This prevents us from sorting the view immediately after the cursor leaves it
		LastSortTime = CurrentTime;
	}
	else if ( bPendingSortFilteredItems && InCurrentTime > LastSortTime + SortDelaySeconds )
	{
		SortList();
	}
#if ECB_WIP_BREADCRUMB
	// Do quick-jump last as the Tick function might have canceled it
	if(QuickJumpData.bHasChangedSinceLastTick)
	{
		QuickJumpData.bHasChangedSinceLastTick = false;

		const bool bWasJumping = QuickJumpData.bIsJumping;
		QuickJumpData.bIsJumping = true;

		QuickJumpData.LastJumpTime = InCurrentTime;
		QuickJumpData.bHasValidMatch = PerformQuickJump(bWasJumping);
	}
	else if(QuickJumpData.bIsJumping && InCurrentTime > QuickJumpData.LastJumpTime + JumpDelaySeconds)
	{
		ResetQuickJump();
	}

	TSharedPtr<FExtAssetViewItem> AssetAwaitingRename = AwaitingRename.Pin();
	if (AssetAwaitingRename.IsValid())
	{
		TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (!OwnerWindow.IsValid())
		{
			AssetAwaitingRename->bRenameWhenScrolledIntoview = false;
			AwaitingRename = nullptr;
		}
		else if (OwnerWindow->HasAnyUserFocusOrFocusedDescendants())
		{
			AssetAwaitingRename->RenamedRequestEvent.ExecuteIfBound();
			AssetAwaitingRename->bRenameWhenScrolledIntoview = false;
			AwaitingRename = nullptr;
		}
	}
#endif

}

void SExtAssetView::CalculateFillScale( const FGeometry& AllottedGeometry )
{
	if ( bFillEmptySpaceInTileView && CurrentViewType == EAssetViewType::Tile )
	{
		float ItemWidth = GetTileViewItemBaseWidth();

		// Scrollbars are 16, but we add 1 to deal with half pixels.
		const float ScrollbarWidth = 16 + 1;
		float TotalWidth = AllottedGeometry.GetLocalSize().X - ( ScrollbarWidth / AllottedGeometry.Scale );
		float Coverage = TotalWidth / ItemWidth;
		int32 Items = (int)( TotalWidth / ItemWidth );

		// If there isn't enough room to support even a single item, don't apply a fill scale.
		if ( Items > 0 )
		{
			float GapSpace = ItemWidth * ( Coverage - Items );
			float ExpandAmount = GapSpace / (float)Items;
			FillScale = ( ItemWidth + ExpandAmount ) / ItemWidth;
			FillScale = FMath::Max( 1.0f, FillScale );
		}
		else
		{
			FillScale = 1.0f;
		}
	}
	else
	{
		FillScale = 1.0f;
	}
}

void SExtAssetView::CalculateThumbnailHintColorAndOpacity()
{
	if ( HighlightedText.Get().IsEmpty() )
	{
		if ( ThumbnailHintFadeInSequence.IsPlaying() )
		{
			if ( ThumbnailHintFadeInSequence.IsForward() )
			{
				ThumbnailHintFadeInSequence.Reverse();
			}
		}
		else if ( ThumbnailHintFadeInSequence.IsAtEnd() ) 
		{
			ThumbnailHintFadeInSequence.PlayReverse(this->AsShared());
		}
	}
	else 
	{
		if ( ThumbnailHintFadeInSequence.IsPlaying() )
		{
			if ( ThumbnailHintFadeInSequence.IsInReverse() )
			{
				ThumbnailHintFadeInSequence.Reverse();
			}
		}
		else if ( ThumbnailHintFadeInSequence.IsAtStart() ) 
		{
			ThumbnailHintFadeInSequence.Play(this->AsShared());
		}
	}

	const float Opacity = ThumbnailHintFadeInSequence.GetLerp();
	ThumbnailHintColorAndOpacity = FLinearColor( 1.0, 1.0, 1.0, Opacity );
}

void SExtAssetView::ProcessQueriedItems( const double TickStartTime )
{
	const bool bFlushFullBuffer = TickStartTime < 0;

	bool ListNeedsRefresh = false;
	int32 AssetIndex = 0;
	for ( AssetIndex = QueriedAssetItems.Num() - 1; AssetIndex >= 0 ; AssetIndex--)
	{
		if ( !OnShouldFilterAsset.Execute( QueriedAssetItems[AssetIndex] ) )
		{
			AssetItems.Add( QueriedAssetItems[AssetIndex] );

			if ( !IsFrontendFilterActive() || PassesCurrentFrontendFilter(QueriedAssetItems[AssetIndex]))
			{
				const FExtAssetData& AssetData = QueriedAssetItems[AssetIndex];
				FilteredAssetItems.Add(MakeShareable(new FExtAssetViewAsset(AssetData)));
				ListNeedsRefresh = true;
				bPendingSortFilteredItems = true;
			}
		}

		// Check to see if we have run out of time in this tick
		if ( !bFlushFullBuffer && (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
		{
			break;
		}
	}

	// Trim the results array
	if (AssetIndex > 0)
	{
		QueriedAssetItems.RemoveAt( AssetIndex, QueriedAssetItems.Num() - AssetIndex );
	}
	else
	{
		QueriedAssetItems.Reset();
	}

	if ( ListNeedsRefresh )
	{
		RefreshList();
	}
}
#if ECB_FEA_ASSET_DRAG_DROP
void SExtAssetView::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
#if ECB_LEGACY
	TSharedPtr< FAssetDragDropOp > AssetDragDropOp = DragDropEvent.GetOperationAs< FAssetDragDropOp >();
	if( AssetDragDropOp.IsValid() )
	{
		AssetDragDropOp->ResetToDefaultToolTip();
		return;
	}

	TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
	if (DragDropOp.IsValid())
	{
		// Do we have a custom handler for this drag event?
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
		const TArray<FAssetViewDragAndDropExtender>& AssetViewDragAndDropExtenders = ContentBrowserModule.GetAssetViewDragAndDropExtenders();
		for (const auto& AssetViewDragAndDropExtender : AssetViewDragAndDropExtenders)
		{
			if (AssetViewDragAndDropExtender.OnDragLeaveDelegate.IsBound() && AssetViewDragAndDropExtender.OnDragLeaveDelegate.Execute(FAssetViewDragAndDropExtender::FPayload(DragDropOp, SourcesData.PackagePaths, SourcesData.Collections)))
			{
				return;
			}
		}
	}
#endif
}

FReply SExtAssetView::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
#if ECB_LEGACY
	TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
	if (DragDropOp.IsValid())
	{
		// Do we have a custom handler for this drag event?
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
		const TArray<FAssetViewDragAndDropExtender>& AssetViewDragAndDropExtenders = ContentBrowserModule.GetAssetViewDragAndDropExtenders();
		for (const auto& AssetViewDragAndDropExtender : AssetViewDragAndDropExtenders)
		{
			if (AssetViewDragAndDropExtender.OnDragOverDelegate.IsBound() && AssetViewDragAndDropExtender.OnDragOverDelegate.Execute(FAssetViewDragAndDropExtender::FPayload(DragDropOp, SourcesData.PackagePaths, SourcesData.Collections)))
			{
				return FReply::Handled();
			}
		}
	}

	if (SourcesData.HasPackagePaths())
	{
		// Note: We don't test IsAssetPathSelected here as we need to prevent dropping assets on class paths
		const FString DestPath = SourcesData.PackagePaths[0].ToString();

		bool bUnused = false;
		DragDropHandler::ValidateDragDropOnAssetFolder(MyGeometry, DragDropEvent, DestPath, bUnused);
		return FReply::Handled();
	}
	else if (HasSingleCollectionSource())
	{
		TArray< FExtAssetData > AssetDatas = AssetUtil::ExtractAssetDataFromDrag(DragDropEvent);

		if (AssetDatas.Num() > 0)
		{
			TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs< FAssetDragDropOp >();
			if (AssetDragDropOp.IsValid())
			{
				TArray< FName > ObjectPaths;
				FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
				const FCollectionNameType& Collection = SourcesData.Collections[0];
				CollectionManagerModule.Get().GetObjectsInCollection(Collection.Name, Collection.Type, ObjectPaths);

				bool IsValidDrop = false;
				for (const auto& AssetData : AssetDatas)
				{
					if (AssetData.GetClass()->IsChildOf(UClass::StaticClass()))
					{
						continue;
					}

					if (!ObjectPaths.Contains(AssetData.ObjectPath))
					{
						IsValidDrop = true;
						break;
					}
				}

				if (IsValidDrop)
				{
					AssetDragDropOp->SetToolTip(NSLOCTEXT("AssetView", "OnDragOverCollection", "Add to Collection"), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")));
				}
			}

			return FReply::Handled();
		}
	}
#endif
	return FReply::Unhandled();
}

FReply SExtAssetView::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
#if ECB_LEGACY
	TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
	if (DragDropOp.IsValid())
	{
		// Do we have a custom handler for this drag event?
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
		const TArray<FAssetViewDragAndDropExtender>& AssetViewDragAndDropExtenders = ContentBrowserModule.GetAssetViewDragAndDropExtenders();
		for (const auto& AssetViewDragAndDropExtender : AssetViewDragAndDropExtenders)
		{
			if (AssetViewDragAndDropExtender.OnDropDelegate.IsBound() && AssetViewDragAndDropExtender.OnDropDelegate.Execute(FAssetViewDragAndDropExtender::FPayload(DragDropOp, SourcesData.PackagePaths, SourcesData.Collections)))
			{
				return FReply::Handled();
			}
		}
	}

	if (SourcesData.HasPackagePaths())
	{
		// Note: We don't test IsAssetPathSelected here as we need to prevent dropping assets on class paths
		const FString DestPath = SourcesData.PackagePaths[0].ToString();

		bool bUnused = false;
		if (DragDropHandler::ValidateDragDropOnAssetFolder(MyGeometry, DragDropEvent, DestPath, bUnused))
		{
			// Handle drag drop for import
			TSharedPtr<FExternalDragOperation> ExternalDragDropOp = DragDropEvent.GetOperationAs<FExternalDragOperation>();
			if (ExternalDragDropOp.IsValid())
			{
				if (ExternalDragDropOp->HasFiles())
				{
					// Delay import until next tick to avoid blocking the process that files were dragged from
					GEditor->GetEditorSubsystem<UImportSubsystem>()->ImportNextTick(ExternalDragDropOp->GetFiles(), SourcesData.PackagePaths[0].ToString());
				}
			}

			TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
			if (AssetDragDropOp.IsValid())
			{
				OnAssetsOrPathsDragDropped(AssetDragDropOp->GetAssets(), AssetDragDropOp->GetAssetPaths(), DestPath);
			}
		}
		return FReply::Handled();
	}
	else if (HasSingleCollectionSource())
	{
		TArray<FExtAssetData> SelectedAssetDatas = AssetUtil::ExtractAssetDataFromDrag(DragDropEvent);

		if (SelectedAssetDatas.Num() > 0)
		{
			TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs< FAssetDragDropOp >();
			if (AssetDragDropOp.IsValid())
			{
				TArray<FName> ObjectPaths;
				for (const auto& AssetData : SelectedAssetDatas)
				{
					if (!AssetData.GetClass()->IsChildOf(UClass::StaticClass()))
					{
						ObjectPaths.Add(AssetData.ObjectPath);
					}
				}

				if (ObjectPaths.Num() > 0)
				{
					FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
					const FCollectionNameType& Collection = SourcesData.Collections[0];
					CollectionManagerModule.Get().AddToCollection(Collection.Name, Collection.Type, ObjectPaths);
				}	
			}

			return FReply::Handled();
		}
	}
#endif
	return FReply::Unhandled();
}
#endif
#if ECB_LEGACY
FReply SExtAssetView::OnKeyChar( const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent )
{
	const bool bIsControlOrCommandDown = InCharacterEvent.IsControlDown() || InCharacterEvent.IsCommandDown();
	
	const bool bTestOnly = false;
	if(HandleQuickJumpKeyDown(InCharacterEvent.GetCharacter(), bIsControlOrCommandDown, InCharacterEvent.IsAltDown(), bTestOnly).IsEventHandled())
	{
		return FReply::Handled();
	}

	// If the user pressed a key we couldn't handle, reset the quick-jump search
	ResetQuickJump();

	return FReply::Unhandled();
}

static bool IsValidObjectPath(const FString& Path)
{
	int32 NameStartIndex = INDEX_NONE;
	Path.FindChar(TCHAR('\''), NameStartIndex);
	if (NameStartIndex != INDEX_NONE)
	{
		int32 NameEndIndex = INDEX_NONE;
		Path.FindLastChar(TCHAR('\''), NameEndIndex);
		if (NameEndIndex > NameStartIndex)
		{
			const FString ClassName = Path.Left(NameStartIndex);
			const FString PathName = Path.Mid(NameStartIndex + 1, NameEndIndex - NameStartIndex - 1);

			UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName, true);
			if (Class)
			{
				return FPackageName::IsValidLongPackageName(FPackageName::ObjectPathToPackageName(PathName));
			}
		}
	}

	return false;
}

static bool ContainsT3D(const FString& ClipboardText)
{
	return (ClipboardText.StartsWith(TEXT("Begin Object")) && ClipboardText.EndsWith(TEXT("End Object")))
		|| (ClipboardText.StartsWith(TEXT("Begin Map")) && ClipboardText.EndsWith(TEXT("End Map")));
}
#endif

FReply SExtAssetView::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
#if ECB_LEGACY
	const bool bIsControlOrCommandDown = InKeyEvent.IsControlDown() || InKeyEvent.IsCommandDown();
	
	if (bIsControlOrCommandDown && InKeyEvent.GetCharacter() == 'V' && IsAssetPathSelected())
	{

		FString AssetPaths;
		TArray<FString> AssetPathsSplit;

		// Get the copied asset paths
		FPlatformApplicationMisc::ClipboardPaste(AssetPaths);

		// Make sure the clipboard does not contain T3D
		AssetPaths.TrimEndInline();
		if (!ContainsT3D(AssetPaths))
		{
			AssetPaths.ParseIntoArrayLines(AssetPathsSplit);

			// Get assets and copy them
			TArray<UObject*> AssetsToCopy;
			for (const FString& AssetPath : AssetPathsSplit)
			{
				// Validate string
				if (IsValidObjectPath(AssetPath))
				{
					UObject* ObjectToCopy = LoadObject<UObject>(nullptr, *AssetPath);
					if (ObjectToCopy && !ObjectToCopy->IsA(UClass::StaticClass()))
					{
						AssetsToCopy.Add(ObjectToCopy);
					}
				}
			}

			if (AssetsToCopy.Num())
			{
				ContentBrowserUtils::CopyAssets(AssetsToCopy, SourcesData.PackagePaths[0].ToString());
			}
		}

		return FReply::Handled();
	}
	// Swallow the key-presses used by the quick-jump in OnKeyChar to avoid other things (such as the viewport commands) getting them instead
	// eg) Pressing "W" without this would set the viewport to "translate" mode
	else if(HandleQuickJumpKeyDown(InKeyEvent.GetCharacter(), bIsControlOrCommandDown, InKeyEvent.IsAltDown(), /*bTestOnly*/true).IsEventHandled())
	{
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply SExtAssetView::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.IsControlDown() )
	{
		const float DesiredScale = FMath::Clamp<float>(GetThumbnailScale() + ( MouseEvent.GetWheelDelta() * 0.05f ), 0.0f, 1.0f);
		if ( DesiredScale != GetThumbnailScale() )
		{
			SetThumbnailScale( DesiredScale );
		}		
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

#if ECB_WIP_BREADCRUMB
void SExtAssetView::OnFocusChanging( const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	ResetQuickJump();
}
#endif

TSharedRef<SExtAssetTileView> SExtAssetView::CreateTileView()
{
	return SNew(SExtAssetTileView)
		.SelectionMode(SelectionMode)
		.ListItemsSource(&FilteredAssetItems)
		.OnGenerateTile(this, &SExtAssetView::MakeTileViewWidget)
		.OnItemScrolledIntoView(this, &SExtAssetView::ItemScrolledIntoView)
 		.OnContextMenuOpening(this, &SExtAssetView::OnGetContextMenuContent)
 		.OnMouseButtonDoubleClick(this, &SExtAssetView::OnListMouseButtonDoubleClick)
 		.OnSelectionChanged(this, &SExtAssetView::AssetSelectionChanged)
 		.ItemHeight(this, &SExtAssetView::GetTileViewItemHeight)
 		.ItemWidth(this, &SExtAssetView::GetTileViewItemWidth);
}

TSharedRef<SExtAssetListView> SExtAssetView::CreateListView()
{
	return SNew(SExtAssetListView)
		.SelectionMode( SelectionMode )
		.ListItemsSource(&FilteredAssetItems)
		.OnGenerateRow(this, &SExtAssetView::MakeListViewWidget)
		.OnItemScrolledIntoView(this, &SExtAssetView::ItemScrolledIntoView)
		.OnContextMenuOpening(this, &SExtAssetView::OnGetContextMenuContent)
		.OnMouseButtonDoubleClick(this, &SExtAssetView::OnListMouseButtonDoubleClick)
		.OnSelectionChanged(this, &SExtAssetView::AssetSelectionChanged)
		.ItemHeight(this, &SExtAssetView::GetListViewItemHeight);
}

TSharedRef<SExtAssetColumnView> SExtAssetView::CreateColumnView()
{
	TSharedPtr<SExtAssetColumnView> NewColumnView = SNew(SExtAssetColumnView)
		.SelectionMode( SelectionMode )
		.ListItemsSource(&FilteredAssetItems)
		.OnGenerateRow(this, &SExtAssetView::MakeColumnViewWidget)
		.OnItemScrolledIntoView(this, &SExtAssetView::ItemScrolledIntoView)
		.OnContextMenuOpening(this, &SExtAssetView::OnGetContextMenuContent)
		.OnMouseButtonDoubleClick(this, &SExtAssetView::OnListMouseButtonDoubleClick)
		.OnSelectionChanged(this, &SExtAssetView::AssetSelectionChanged)
		.Visibility(this, &SExtAssetView::GetColumnViewVisibility)
		.HeaderRow
		(
			SNew(SHeaderRow)
			.ResizeMode(ESplitterResizeMode::FixedSize)
			+ SHeaderRow::Column(SortManager.NameColumnId)
			.FillWidth(300)
			.SortMode( TAttribute< EColumnSortMode::Type >::Create( TAttribute< EColumnSortMode::Type >::FGetter::CreateSP( this, &SExtAssetView::GetColumnSortMode, SortManager.NameColumnId ) ) )
			.SortPriority(TAttribute< EColumnSortPriority::Type >::Create(TAttribute< EColumnSortPriority::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortPriority, SortManager.NameColumnId)))
			.OnSort( FOnSortModeChanged::CreateSP( this, &SExtAssetView::OnSortColumnHeader ) )
			.DefaultLabel( LOCTEXT("Column_Name", "Name") )
			.ShouldGenerateWidget(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SExtAssetView::ShouldColumnGenerateWidget, SortManager.NameColumnId.ToString())))
			.MenuContent()
			[
				CreateRowHeaderMenuContent(SortManager.NameColumnId.ToString())
			]
		);

	NewColumnView->GetHeaderRow()->SetOnGetMaxRowSizeForColumn(FOnGetMaxRowSizeForColumn::CreateRaw(NewColumnView.Get(), &SExtAssetColumnView::GetMaxRowSizeForColumn));


	NumVisibleColumns = HiddenColumnNames.Contains(SortManager.NameColumnId.ToString()) ? 0 : 1;

	if(bShowTypeInColumnView)
	{
		NewColumnView->GetHeaderRow()->AddColumn(
				SHeaderRow::Column(SortManager.ClassColumnId)
				.FillWidth(160)
				.SortMode(TAttribute< EColumnSortMode::Type >::Create(TAttribute< EColumnSortMode::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortMode, SortManager.ClassColumnId)))
				.SortPriority(TAttribute< EColumnSortPriority::Type >::Create(TAttribute< EColumnSortPriority::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortPriority, SortManager.ClassColumnId)))
				.OnSort(FOnSortModeChanged::CreateSP(this, &SExtAssetView::OnSortColumnHeader))
				.DefaultLabel(LOCTEXT("Column_Class", "Type"))
				.ShouldGenerateWidget(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SExtAssetView::ShouldColumnGenerateWidget, SortManager.ClassColumnId.ToString())))
				.MenuContent()
				[
					CreateRowHeaderMenuContent(SortManager.ClassColumnId.ToString())
				]
			);

		NumVisibleColumns += HiddenColumnNames.Contains(SortManager.ClassColumnId.ToString()) ? 0 : 1;
	}


	if (bShowPathInColumnView)
	{
		NewColumnView->GetHeaderRow()->AddColumn(
				SHeaderRow::Column(SortManager.PathColumnId)
				.FillWidth(160)
				.SortMode(TAttribute< EColumnSortMode::Type >::Create(TAttribute< EColumnSortMode::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortMode, SortManager.PathColumnId)))
				.SortPriority(TAttribute< EColumnSortPriority::Type >::Create(TAttribute< EColumnSortPriority::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortPriority, SortManager.PathColumnId)))
				.OnSort(FOnSortModeChanged::CreateSP(this, &SExtAssetView::OnSortColumnHeader))
				.DefaultLabel(LOCTEXT("Column_Path", "Path"))
				.ShouldGenerateWidget(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SExtAssetView::ShouldColumnGenerateWidget, SortManager.PathColumnId.ToString())))
				.MenuContent()
				[
					CreateRowHeaderMenuContent(SortManager.PathColumnId.ToString())
				]
			);


		NumVisibleColumns += HiddenColumnNames.Contains(SortManager.PathColumnId.ToString()) ? 0 : 1;
	}

	return NewColumnView.ToSharedRef();
}

#if ECB_LEGACY
bool SExtAssetView::IsValidSearchToken(const FString& Token) const
{
	if ( Token.Len() == 0 )
	{
		return false;
	}

	// A token may not be only apostrophe only, or it will match every asset because the text filter compares against the pattern Class'ObjectPath'
	if ( Token.Len() == 1 && Token[0] == '\'' )
	{
		return false;
	}

	return true;
}
#endif
void SExtAssetView::RefreshSourceItems()
{
	RecentlyLoadedOrChangedAssets.Reset();
	RecentlyAddedAssets.Reset();
	FilteredRecentlyAddedAssets.Reset();

	QueriedAssetItems.Reset();
	AssetItems.Reset();
	FilteredAssetItems.Reset();
	VisibleItems.Reset();
	RelevantThumbnails.Reset();
	Folders.Reset();

	TArray<FExtAssetData>& Items = OnShouldFilterAsset.IsBound() ? QueriedAssetItems : AssetItems;

	if (OnShouldFilterAsset.IsBound())
	{
		ECB_LOG(Display, TEXT("OnShouldFilterAsset.IsBound()"));
	}

#if ECB_TODO // show all?
	const bool bShowAll = SourcesData.IsEmpty() && BackendFilter.IsEmpty();
	if ( bShowAll )
	{
		// Include assets in memory
		TSet<FName> PackageNamesToSkip = AssetRegistryModule.Get().GetCachedEmptyPackages();
		for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
		{
			if (ObjIt->IsAsset())
			{
				if (!InitialAssetFilter.PassesPackagePathFilter(ObjIt->GetOutermost()->GetFName()))
				{
					continue;
				}

				int32 Index = Items.Emplace(*ObjIt);
				const FExtAssetData& AssetData = Items[Index];
				if (!InitialAssetFilter.PassesRedirectorMainAssetFilter(AssetData))
				{
					Items.RemoveAtSwap(Index, 1, false);
					continue;
				}

				PackageNamesToSkip.Add(AssetData.PackageName);
			}
		}

		// Include assets on disk
		const TMap<FName, const FAssetData*>& AssetDataMap = AssetRegistryModule.Get().GetAssetRegistryState()->GetObjectPathToAssetDataMap();
		for (const TPair<FName, const FAssetData*>& AssetDataPair : AssetDataMap)
		{
			const FAssetData* AssetData = AssetDataPair.Value;
			if (AssetData == nullptr)
			{
				continue;
			}

			// Make sure the asset's package was not loaded then the object was deleted/renamed
			if (PackageNamesToSkip.Contains(AssetData->PackageName))
			{
				continue;
			}

			if (!InitialAssetFilter.PassesFilter(*AssetData))
			{
				continue;
			}

			Items.Emplace(*AssetData);
		}

		bShowClasses = IsShowingCppContent();
		bWereItemsRecursivelyFiltered = true;
	}
	else
#endif
	{
		// Assemble the filter using the current sources
		// force recursion when the user is searching
		const bool bRecurse = ShouldFilterRecursively();
		const bool bUsingFolders = IsShowingFolders();
		const bool bIsDynamicCollection = SourcesData.IsDynamicCollection();
		FARFilter Filter = SourcesData.MakeFilter(bRecurse, bUsingFolders);

		// Add the backend filters from the filter list
		Filter.Append(BackendFilter);

		bWereItemsRecursivelyFiltered = bRecurse;

#if ECB_WIP_COLLECTION
		if ( SourcesData.HasCollections() && Filter.ObjectPaths.Num() == 0 && !bIsDynamicCollection )
		{
			// This is an empty collection, no asset will pass the check
		}
		else
#endif
		{
			
#if ECB_FEA_ASYNC_ASSET_DISCOVERY		
			FExtContentBrowserSingleton::GetAssetRegistry().CacheAssetsAsync(Filter);
#endif
			// Add assets found in the asset registry
			FExtContentBrowserSingleton::GetAssetRegistry().GetAssets(Filter, Items);
		}

#if ECB_WIP_INITIAL_ASSET
		for (int32 AssetIdx = Items.Num() - 1; AssetIdx >= 0; --AssetIdx)
		{
			if (!InitialAssetFilter.PassesFilter(Items[AssetIdx]))
			{
				Items.RemoveAtSwap(AssetIdx);
			}
		}
	}
#endif
	}

}

bool SExtAssetView::IsFiltering() const
{
	// In some cases we want to not filter recursively even if we have a backend filter (e.g. the open level window)
	// Most of the time, bFilterRecursivelyWithBackendFilter is true
	if (bFilterRecursivelyWithBackendFilter && !BackendFilter.IsEmpty())
	{
		return true;
	}

	// Otherwise, check if there are any non-inverse frontend filters selected
	if (FrontendFilters.IsValid())
	{
		for (int32 FilterIndex = 0; FilterIndex < FrontendFilters->Num(); ++FilterIndex)
		{
			const auto* Filter = static_cast<FExtFrontendFilter*>(FrontendFilters->GetFilterAtIndex(FilterIndex).Get());
			if (Filter)
			{
				if (!Filter->IsInverseFilter())
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool SExtAssetView::ShouldFilterRecursively() const
{
	const bool bSearchAndFilterRecursively = IsSearchAndFilterRecursively();

	// Quick check for conditions which force recursive filtering
	if (bUserSearching && bSearchAndFilterRecursively)
	{
		return true;
	}

	if (IsFiltering() && bSearchAndFilterRecursively)
	{
		return true;
	}

#if ECB_LEGACY

	// In some cases we want to not filter recursively even if we have a backend filter (e.g. the open level window)
	// Most of the time, bFilterRecursivelyWithBackendFilter is true
	if ( bFilterRecursivelyWithBackendFilter && !BackendFilter.IsEmpty() && bSearchAndFilterRecursively)
	{
		return true;
	}

	// Otherwise, check if there are any non-inverse frontend filters selected
	if (FrontendFilters.IsValid() && bSearchAndFilterRecursively)
	{
		for (int32 FilterIndex = 0; FilterIndex < FrontendFilters->Num(); ++FilterIndex)
		{
			const auto* Filter = static_cast<FExtFrontendFilter*>(FrontendFilters->GetFilterAtIndex(FilterIndex).Get());
			if (Filter)
			{
				if (!Filter->IsInverseFilter())
				{
					return true;
				}
			}
		}
	}
#endif

	// No filters, do not override folder view with recursive filtering
	return false;
}

void SExtAssetView::RefreshFilteredItems()
{

	//Build up a map of the existing AssetItems so we can preserve them while filtering
	TMap< FName, TSharedPtr< FExtAssetViewAsset > > ItemToObjectPath;
	for (int Index = 0; Index < FilteredAssetItems.Num(); Index++)
	{
		if(FilteredAssetItems[Index].IsValid() && FilteredAssetItems[Index]->GetType() != EExtAssetItemType::Folder)
		{
			TSharedPtr<FExtAssetViewAsset> Item = StaticCastSharedPtr<FExtAssetViewAsset>(FilteredAssetItems[Index]);

			// Clear custom column data
			Item->CustomColumnData.Reset();
			Item->CustomColumnDisplayText.Reset();

			ItemToObjectPath.Add( Item->Data.ObjectPath, Item );
		}
	}

	// Empty all the filtered lists
	FilteredAssetItems.Reset();
	VisibleItems.Reset();
	RelevantThumbnails.Reset();
	Folders.Reset();

	// true if the results from the asset registry query are filtered further by the content browser
	const bool bIsFrontendFilterActive = IsFrontendFilterActive();

	// true if we are looking at columns so we need to determine the majority asset type
	const bool bGatherAssetTypeCount = CurrentViewType == EAssetViewType::Column;
	TMap<FName, int32> AssetTypeCount;

	if ( bIsFrontendFilterActive && FrontendFilters.IsValid() )
	{
		const bool bRecurse = ShouldFilterRecursively();
#if ECB_WIP_SEARCH_RECURSE_TOGGLE
		const bool bShouldHideFolders = (bUserSearching || IsFiltering()) && !IsSearchAndFilterRecursively();
#else
		const bool bShouldHideFolders = false;
#endif
		const bool bUsingFolders = IsShowingFolders() && !bShouldHideFolders;
		FARFilter CombinedFilter = SourcesData.MakeFilter(bRecurse, bUsingFolders);
		CombinedFilter.Append(BackendFilter);

		// Let the frontend filters know the currently used filter in case it is necessary to conditionally filter based on path or class filters
		for ( int32 FilterIdx = 0; FilterIdx < FrontendFilters->Num(); ++FilterIdx )
		{
			// There are only FFrontendFilters in this collection
			const TSharedPtr<FExtFrontendFilter>& Filter = StaticCastSharedPtr<FExtFrontendFilter>( FrontendFilters->GetFilterAtIndex(FilterIdx) );
			if ( Filter.IsValid() )
			{
				Filter->SetCurrentFilter(CombinedFilter);
			}
		}
	}

	if ( bIsFrontendFilterActive && bGatherAssetTypeCount )
	{
		// Check the frontend filter for every asset and keep track of how many assets were found of each type
		for (int32 AssetIdx = 0; AssetIdx < AssetItems.Num(); ++AssetIdx)
		{
			const FExtAssetData& AssetData = AssetItems[AssetIdx];
			if ( PassesCurrentFrontendFilter(AssetData) )
			{
				const TSharedPtr< FExtAssetViewAsset >* AssetItem = ItemToObjectPath.Find( AssetData.ObjectPath );

				if ( AssetItem != NULL )
				{
					FilteredAssetItems.Add(*AssetItem);
				}
				else
				{
					FilteredAssetItems.Add(MakeShareable(new FExtAssetViewAsset(AssetData)));
				}

				int32* TypeCount = AssetTypeCount.Find(AssetData.AssetClass);
				if ( TypeCount )
				{
					(*TypeCount)++;
				}
				else
				{
					AssetTypeCount.Add(AssetData.AssetClass, 1);
				}
			}
		}
	}
	else if ( bIsFrontendFilterActive && !bGatherAssetTypeCount )
	{
		// Check the frontend filter for every asset and don't worry about asset type counts
		for (int32 AssetIdx = 0; AssetIdx < AssetItems.Num(); ++AssetIdx)
		{
			const FExtAssetData& AssetData = AssetItems[AssetIdx];
			if ( PassesCurrentFrontendFilter(AssetData) )
			{
				const TSharedPtr< FExtAssetViewAsset >* AssetItem = ItemToObjectPath.Find( AssetData.ObjectPath );

				if ( AssetItem != NULL )
				{
					FilteredAssetItems.Add(*AssetItem);
				}
				else
				{
					FilteredAssetItems.Add(MakeShareable(new FExtAssetViewAsset(AssetData)));
				}
			}
		}
	}
	else if ( !bIsFrontendFilterActive && bGatherAssetTypeCount )
	{
		// Don't need to check the frontend filter for every asset but keep track of how many assets were found of each type
		for (int32 AssetIdx = 0; AssetIdx < AssetItems.Num(); ++AssetIdx)
		{
			const FExtAssetData& AssetData = AssetItems[AssetIdx];
			const TSharedPtr< FExtAssetViewAsset >* AssetItem = ItemToObjectPath.Find( AssetData.ObjectPath );

			if ( AssetItem != NULL )
			{
				FilteredAssetItems.Add(*AssetItem);
			}
			else
			{
				FilteredAssetItems.Add(MakeShareable(new FExtAssetViewAsset(AssetData)));
			}

			int32* TypeCount = AssetTypeCount.Find(AssetData.AssetClass);
			if ( TypeCount )
			{
				(*TypeCount)++;
			}
			else
			{
				AssetTypeCount.Add(AssetData.AssetClass, 1);
			}
		}
	}
	else if ( !bIsFrontendFilterActive && !bGatherAssetTypeCount )
	{
		// Don't check the frontend filter and don't count the number of assets of each type. Just add all assets.
		for (int32 AssetIdx = 0; AssetIdx < AssetItems.Num(); ++AssetIdx)
		{
			const FExtAssetData& AssetData = AssetItems[AssetIdx];
			const TSharedPtr< FExtAssetViewAsset >* AssetItem = ItemToObjectPath.Find( AssetData.ObjectPath );

			if ( AssetItem != NULL )
			{
				FilteredAssetItems.Add(*AssetItem);
			}
			else
			{
				FilteredAssetItems.Add(MakeShareable(new FExtAssetViewAsset(AssetData)));
			}
		}
	}
	else
	{
		// The above cases should handle all combinations of bIsFrontendFilterActive and bGatherAssetTypeCount
		ensure(0);
	}

	if ( bGatherAssetTypeCount )
	{
		int32 HighestCount = 0;
		FName HighestType;
		for ( auto TypeIt = AssetTypeCount.CreateConstIterator(); TypeIt; ++TypeIt )
		{
			if ( TypeIt.Value() > HighestCount )
			{
				HighestType = TypeIt.Key();
				HighestCount = TypeIt.Value();
			}
		}

		SetMajorityAssetType(HighestType);
	}
}

void SExtAssetView::RefreshFolders()
{
	if(!IsShowingFolders() || ShouldFilterRecursively())
	{
		return;
	}

#if ECB_WIP_SEARCH_RECURSE_TOGGLE
	const bool bShouldHideFolders = (bUserSearching || IsFiltering()) && !IsSearchAndFilterRecursively();
	if (bShouldHideFolders)
	{
		return;
	}
#endif
	
	// Split the selected paths into asset and class paths
	TArray<FName> AssetPathsToShow;
	TArray<FName> ClassPathsToShow;
	for(const FName& PackagePath : SourcesData.PackagePaths)
	{
		if(ExtContentBrowserUtils::IsClassPath(PackagePath.ToString()))
		{
			ClassPathsToShow.Add(PackagePath);
		}
		else
		{
			AssetPathsToShow.Add(PackagePath);
		}
	}

	TArray<FString> FoldersToAdd;

	const bool bDisplayEmpty = IsShowingEmptyFolders();
	{
		TSet<FName> SubPaths;
		for(const FName& PackagePath : AssetPathsToShow)
		{
			SubPaths.Reset();
			FExtContentBrowserSingleton::GetAssetRegistry().GetOrCacheSubPaths(PackagePath, SubPaths, false);

			for(const FName& SubPath : SubPaths)
			{
				FString SubPathString = SubPath.ToString();

				if (!bDisplayEmpty && FExtContentBrowserSingleton::GetAssetRegistry().IsEmptyFolder(SubPathString))
				{
					continue;
				}
				
				if(!Folders.Contains(SubPathString))
				{
					FoldersToAdd.Add(SubPathString);
				}
			}
		}
	}

	// Add folders for any child collections of the currently selected collections
	if(SourcesData.HasCollections())
	{
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
		
		TArray<FCollectionNameType> ChildCollections;
		for(const FCollectionNameType& Collection : SourcesData.Collections)
		{
			ChildCollections.Reset();
			CollectionManagerModule.Get().GetChildCollections(Collection.Name, Collection.Type, ChildCollections);

			for(const FCollectionNameType& ChildCollection : ChildCollections)
			{
				// Use "Collections" as the root of the path to avoid this being confused with other asset view folders - see ContentBrowserUtils::IsCollectionPath
				FoldersToAdd.Add(FString::Printf(TEXT("/Collections/%s/%s"), ECollectionShareType::ToString(ChildCollection.Type), *ChildCollection.Name.ToString()));
			}
		}
	}

	if(FoldersToAdd.Num() > 0)
	{
		for(const FString& FolderPath : FoldersToAdd)
		{
			FilteredAssetItems.Add(MakeShareable(new FExtAssetViewFolder(FolderPath)));
			Folders.Add(FolderPath);
		}

		RefreshList();
		bPendingSortFilteredItems = true;
	}
}

void SExtAssetView::SetMajorityAssetType(FName NewMajorityAssetType)
{
	auto IsFixedColumn = [this](FName InColumnId)
	{
		const bool bIsFixedNameColumn = InColumnId == SortManager.NameColumnId;
		const bool bIsFixedClassColumn = bShowTypeInColumnView && InColumnId == SortManager.ClassColumnId;
		const bool bIsFixedPathColumn = bShowPathInColumnView && InColumnId == SortManager.PathColumnId;
		return bIsFixedNameColumn || bIsFixedClassColumn || bIsFixedPathColumn;
	};

	if ( NewMajorityAssetType != MajorityAssetType )
	{
		ECB_LOG(Display , TEXT("The majority of assets in the view are of type: %s"), *NewMajorityAssetType.ToString());

		MajorityAssetType = NewMajorityAssetType;

		TArray<FName> AddedColumns;

		// Since the asset type has changed, remove all columns except name and class
		const TIndirectArray<SHeaderRow::FColumn>& Columns = ColumnView->GetHeaderRow()->GetColumns();

		for ( int32 ColumnIdx = Columns.Num() - 1; ColumnIdx >= 0; --ColumnIdx )
		{
			const FName ColumnId = Columns[ColumnIdx].ColumnId;

			if ( ColumnId != NAME_None && !IsFixedColumn(ColumnId) )
			{
				ColumnView->GetHeaderRow()->RemoveColumn(ColumnId);
			}
		}

		// Keep track of the current column name to see if we need to change it now that columns are being removed
		// Name, Class, and Path are always relevant
		struct FSortOrder
		{
			bool bSortRelevant;
			FName SortColumn;
			FSortOrder(bool bInSortRelevant, const FName& InSortColumn) : bSortRelevant(bInSortRelevant), SortColumn(InSortColumn) {}
		};
		TArray<FSortOrder> CurrentSortOrder;
		for (int32 PriorityIdx = 0; PriorityIdx < EColumnSortPriority::Max; PriorityIdx++)
		{
			const FName SortColumn = SortManager.GetSortColumnId(static_cast<EColumnSortPriority::Type>(PriorityIdx));
			if (SortColumn != NAME_None)
			{
				const bool bSortRelevant = SortColumn == FAssetViewSortManager::NameColumnId
					|| SortColumn == FAssetViewSortManager::ClassColumnId
					|| SortColumn == FAssetViewSortManager::PathColumnId;
				CurrentSortOrder.Add(FSortOrder(bSortRelevant, SortColumn));
			}
		}

		// Add custom columns
		for (const FAssetViewCustomColumn& Column : CustomColumns)
		{
			FName TagName = Column.ColumnName;

			if (AddedColumns.Contains(TagName))
			{
				continue;
			}
			AddedColumns.Add(TagName);

			ColumnView->GetHeaderRow()->AddColumn(
				SHeaderRow::Column(TagName)
				.SortMode(TAttribute< EColumnSortMode::Type >::Create(TAttribute< EColumnSortMode::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortMode, TagName)))
				.SortPriority(TAttribute< EColumnSortPriority::Type >::Create(TAttribute< EColumnSortPriority::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortPriority, TagName)))
				.OnSort(FOnSortModeChanged::CreateSP(this, &SExtAssetView::OnSortColumnHeader))
				.DefaultLabel(Column.DisplayName)
				.DefaultTooltip(Column.TooltipText)
				.FillWidth(180)
				.ShouldGenerateWidget(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SExtAssetView::ShouldColumnGenerateWidget, TagName.ToString())))
				.MenuContent()
				[
					CreateRowHeaderMenuContent(TagName.ToString())
				]);

			NumVisibleColumns += HiddenColumnNames.Contains(TagName.ToString()) ? 0 : 1;

			// If we found a tag the matches the column we are currently sorting on, there will be no need to change the column
			for (int32 SortIdx = 0; SortIdx < CurrentSortOrder.Num(); SortIdx++)
			{
				if (TagName == CurrentSortOrder[SortIdx].SortColumn)
				{
					CurrentSortOrder[SortIdx].bSortRelevant = true;
				}
			}
		}

		// If we have a new majority type, add the new type's columns
		if ( NewMajorityAssetType != NAME_None )
		{
			// Determine the columns by querying the CDO for the tag map
			UClass* TypeClass = FindObject<UClass>(nullptr, *NewMajorityAssetType.ToString());
			if ( TypeClass )
			{
				UObject* CDO = TypeClass->GetDefaultObject();
				if ( CDO )
				{
					TArray<UObject::FAssetRegistryTag> AssetRegistryTags;
					CDO->GetAssetRegistryTags(AssetRegistryTags);

					// Add a column for every tag that isn't hidden or using a reserved name
					for ( auto TagIt = AssetRegistryTags.CreateConstIterator(); TagIt; ++TagIt )
					{
						if ( TagIt->Type != UObject::FAssetRegistryTag::TT_Hidden )
						{
							const FName TagName = TagIt->Name;

							if (IsFixedColumn(TagName))
							{
								// Reserved name
								continue;
							}

							//if ( !OnAssetTagWantsToBeDisplayed.IsBound() || OnAssetTagWantsToBeDisplayed.Execute(NewMajorityAssetType, TagName) )
							{
								if (AddedColumns.Contains(TagName))
								{
									continue;
								}
								AddedColumns.Add(TagName);

								// Get tag metadata
								TMap<FName, UObject::FAssetRegistryTagMetadata> MetadataMap;
								CDO->GetAssetRegistryTagMetadata(MetadataMap);
								const UObject::FAssetRegistryTagMetadata* Metadata = MetadataMap.Find(TagName);

								FText DisplayName;
								if (Metadata != nullptr && !Metadata->DisplayName.IsEmpty())
								{
									DisplayName = Metadata->DisplayName;
								}
								else
								{
									DisplayName = FText::FromName(TagName);
								}

								FText TooltipText;
								if (Metadata != nullptr && !Metadata->TooltipText.IsEmpty())
								{
									TooltipText = Metadata->TooltipText;
								}
								else
								{
									// If the tag name corresponds to a property name, use the property tooltip
									FProperty* Property = FindFProperty<FProperty>(TypeClass, TagName);
									TooltipText = (Property != nullptr) ? Property->GetToolTipText() : FText::FromString(FName::NameToDisplayString(TagName.ToString(), false));
								}

								ColumnView->GetHeaderRow()->AddColumn(
									SHeaderRow::Column(TagName)
									.SortMode(TAttribute< EColumnSortMode::Type >::Create(TAttribute< EColumnSortMode::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortMode, TagName)))
									.SortPriority(TAttribute< EColumnSortPriority::Type >::Create(TAttribute< EColumnSortPriority::Type >::FGetter::CreateSP(this, &SExtAssetView::GetColumnSortPriority, TagName)))
									.OnSort(FOnSortModeChanged::CreateSP(this, &SExtAssetView::OnSortColumnHeader))
									.DefaultLabel(DisplayName)
									.DefaultTooltip(TooltipText)
									.FillWidth(180)
									.ShouldGenerateWidget(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SExtAssetView::ShouldColumnGenerateWidget, TagName.ToString())))
									.MenuContent()
									[
										CreateRowHeaderMenuContent(TagName.ToString())
									]);								
								
								NumVisibleColumns += HiddenColumnNames.Contains(TagName.ToString()) ? 0 : 1;

								// If we found a tag the matches the column we are currently sorting on, there will be no need to change the column
								for (int32 SortIdx = 0; SortIdx < CurrentSortOrder.Num(); SortIdx++)
								{
									if (TagName == CurrentSortOrder[SortIdx].SortColumn)
									{
										CurrentSortOrder[SortIdx].bSortRelevant = true;
									}
								}
							}
						}
					}
				}			
			}	
		}

		// Are any of the sort columns irrelevant now, if so remove them from the list
		bool CurrentSortChanged = false;
		for (int32 SortIdx = CurrentSortOrder.Num() - 1; SortIdx >= 0; SortIdx--)
		{
			if (!CurrentSortOrder[SortIdx].bSortRelevant)
			{
				CurrentSortOrder.RemoveAt(SortIdx);
				CurrentSortChanged = true;
			}
		}
		if (CurrentSortOrder.Num() > 0 && CurrentSortChanged)
		{
			// Sort order has changed, update the columns keeping those that are relevant
			int32 PriorityNum = EColumnSortPriority::Primary;
			for (int32 SortIdx = 0; SortIdx < CurrentSortOrder.Num(); SortIdx++)
			{
				check(CurrentSortOrder[SortIdx].bSortRelevant);
				if (!SortManager.SetOrToggleSortColumn(static_cast<EColumnSortPriority::Type>(PriorityNum), CurrentSortOrder[SortIdx].SortColumn))
				{
					// Toggle twice so mode is preserved if this isn't a new column assignation
					SortManager.SetOrToggleSortColumn(static_cast<EColumnSortPriority::Type>(PriorityNum), CurrentSortOrder[SortIdx].SortColumn);
				}				
				bPendingSortFilteredItems = true;
				PriorityNum++;
			}
		}
		else if (CurrentSortOrder.Num() == 0)
		{
			// If the current sort column is no longer relevant, revert to "Name" and resort when convenient
			SortManager.ResetSort();
			bPendingSortFilteredItems = true;
		}
	}
}

#if ECB_WIP_COLLECTION
void SExtAssetView::OnAssetsAddedToCollection( const FCollectionNameType& Collection, const TArray< FName >& ObjectPaths )
{
	if ( !SourcesData.Collections.Contains( Collection ) )
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (int Index = 0; Index < ObjectPaths.Num(); Index++)
	{
		OnAssetAdded( AssetRegistryModule.Get().GetAssetByObjectPath( ObjectPaths[Index] ) );
	}
}

void SExtAssetView::OnAssetsRemovedFromCollection(const FCollectionNameType& Collection, const TArray< FName >& ObjectPaths)
{
	if (!SourcesData.Collections.Contains(Collection))
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (int Index = 0; Index < ObjectPaths.Num(); Index++)
	{
		OnAssetRemoved(AssetRegistryModule.Get().GetAssetByObjectPath(ObjectPaths[Index]));
	}
}
#endif

#if ECB_FEA_ASYNC_ASSET_DISCOVERY
void SExtAssetView::ProcessRecentlyAddedAssets()
{
	if (
		(RecentlyAddedAssets.Num() > 2048) ||
		(RecentlyAddedAssets.Num() > 0 && FPlatformTime::Seconds() - LastProcessAddsTime >= TimeBetweenAddingNewAssets)
		)
	{
		RunAssetsThroughBackendFilter(RecentlyAddedAssets);
		FilteredRecentlyAddedAssets.Append(RecentlyAddedAssets);
		RecentlyAddedAssets.Reset();
		LastProcessAddsTime = FPlatformTime::Seconds();
	}

	if (FilteredRecentlyAddedAssets.Num() > 0)
	{
		double TickStartTime = FPlatformTime::Seconds();
		bool bNeedsRefresh = false;

		TSet<FName> ExistingObjectPaths;
		for ( auto AssetIt = AssetItems.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ExistingObjectPaths.Add((*AssetIt).ObjectPath);
		}

		for ( auto AssetIt = QueriedAssetItems.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ExistingObjectPaths.Add((*AssetIt).ObjectPath);
		}

		int32 AssetIdx = 0;
		for ( ; AssetIdx < FilteredRecentlyAddedAssets.Num(); ++AssetIdx )
		{
			const FExtAssetData& AssetData = FilteredRecentlyAddedAssets[AssetIdx];
			if ( !ExistingObjectPaths.Contains(AssetData.ObjectPath) )
			{
				if ( AssetData.AssetClass != UObjectRedirector::StaticClass()->GetFName() || AssetData.IsUAsset() )
				{
					if ( !OnShouldFilterAsset.IsBound() || !OnShouldFilterAsset.Execute(AssetData) )
					{
						// Add the asset to the list
						int32 AddedAssetIdx = AssetItems.Add(AssetData);
						ExistingObjectPaths.Add(AssetData.ObjectPath);
						if (!IsFrontendFilterActive() || PassesCurrentFrontendFilter(AssetData))
						{
							FilteredAssetItems.Add(MakeShareable(new FExtAssetViewAsset(AssetData)));
							bNeedsRefresh = true;
							bPendingSortFilteredItems = true;
						}
					}
				}
			}

			if ( (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
			{
				// Increment the index to properly trim the buffer below
				++AssetIdx;
				break;
			}
		}

		// Trim the results array
		if (AssetIdx > 0)
		{
			FilteredRecentlyAddedAssets.RemoveAt(0, AssetIdx);
		}

		if (bNeedsRefresh)
		{
			RefreshList();
		}
	}
}
#endif

#if ECB_LEGACY
void SExtAssetView::OnAssetAdded(const FExtAssetData& AssetData)
{
	RecentlyAddedAssets.Add(AssetData);
}

void SExtAssetView::OnAssetRemoved(const FAssetData& AssetData)
{
	RemoveAssetByPath( AssetData.ObjectPath );
	RecentlyAddedAssets.RemoveSingleSwap(AssetData);
}

void SExtAssetView::OnAssetRegistryPathAdded(const FString& Path)
{
	if(IsShowingFolders() && !ShouldFilterRecursively())
	{
		TSharedRef<FEmptyFolderVisibilityManager> EmptyFolderVisibilityManager = FContentBrowserSingleton::Get().GetEmptyFolderVisibilityManager();

		// If this isn't a developer folder or we want to show them, continue
		const bool bDisplayEmpty = IsShowingEmptyFolders();
		const bool bDisplayL10N = IsShowingLocalizedContent();
		if ((bDisplayEmpty || EmptyFolderVisibilityManager->ShouldShowPath(Path)) &&  
			(bDisplayL10N || !ContentBrowserUtils::IsLocalizationFolder(Path))
			)
		{
			for (const FName& SourcePathName : SourcesData.PackagePaths)
			{
				// Ensure that /Folder2 is not considered a subfolder of /Folder by appending /
				FString SourcePath = SourcePathName.ToString() / TEXT("");
				if(Path.StartsWith(SourcePath))
				{
					const FString SubPath = Path.RightChop(SourcePath.Len());

					TArray<FString> SubPathItemList;
					SubPath.ParseIntoArray(SubPathItemList, TEXT("/"), /*InCullEmpty=*/true);

					if (SubPathItemList.Num() > 0)
					{
						const FString NewSubFolder = SourcePath / SubPathItemList[0];
						if (!Folders.Contains(NewSubFolder))
						{
							FilteredAssetItems.Add(MakeShareable(new FAssetViewFolder(NewSubFolder)));
							RefreshList();
							Folders.Add(NewSubFolder);
							bPendingSortFilteredItems = true;
						}
					}
				}
			}
		}
	}
}

void SExtAssetView::OnAssetRegistryPathRemoved(const FString& Path)
{
	FString* Folder = Folders.Find(Path);
	if(Folder != NULL)
	{
		Folders.Remove(Path);

		for (int32 AssetIdx = 0; AssetIdx < FilteredAssetItems.Num(); ++AssetIdx)
		{
			if(FilteredAssetItems[AssetIdx]->GetType() == EAssetItemType::Folder)
			{
				if ( StaticCastSharedPtr<FAssetViewFolder>(FilteredAssetItems[AssetIdx])->FolderPath == Path )
				{
					// Found the folder in the filtered items list, remove it
					FilteredAssetItems.RemoveAt(AssetIdx);
					RefreshList();
					break;
				}
			}
		}
	}
}

void SExtAssetView::OnFolderPopulated(const FString& Path)
{
	OnAssetRegistryPathAdded(Path);
}

void SExtAssetView::RemoveAssetByPath( const FName& ObjectPath )
{
	bool bFoundAsset = false;
	for (int32 AssetIdx = 0; AssetIdx < AssetItems.Num(); ++AssetIdx)
	{
		if ( AssetItems[AssetIdx].ObjectPath == ObjectPath )
		{
			// Found the asset in the cached list, remove it
			AssetItems.RemoveAt(AssetIdx);
			bFoundAsset = true;
			break;
		}
	}

	if ( bFoundAsset )
	{
		// If it was in the AssetItems list, see if it is also in the FilteredAssetItems list
		for (int32 AssetIdx = 0; AssetIdx < FilteredAssetItems.Num(); ++AssetIdx)
		{
			if(FilteredAssetItems[AssetIdx].IsValid() && FilteredAssetItems[AssetIdx]->GetType() != EAssetItemType::Folder)
			{
				if ( StaticCastSharedPtr<FExtAssetViewAsset>(FilteredAssetItems[AssetIdx])->Data.ObjectPath == ObjectPath && !FilteredAssetItems[AssetIdx]->IsTemporaryItem() )
				{
					// Found the asset in the filtered items list, remove it
					FilteredAssetItems.RemoveAt(AssetIdx);
					RefreshList();
					break;
				}
			}
		}
	}
	else
	{
		//Make sure we don't have the item still queued up for processing
		for (int32 AssetIdx = 0; AssetIdx < QueriedAssetItems.Num(); ++AssetIdx)
		{
			if ( QueriedAssetItems[AssetIdx].ObjectPath == ObjectPath )
			{
				// Found the asset in the cached list, remove it
				QueriedAssetItems.RemoveAt(AssetIdx);
				bFoundAsset = true;
				break;
			}
		}
	}
}

void SExtAssetView::OnCollectionRenamed( const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection )
{
	int32 FoundIndex = INDEX_NONE;
	if ( SourcesData.Collections.Find( OriginalCollection, FoundIndex ) )
	{
		SourcesData.Collections[ FoundIndex ] = NewCollection;
	}
}

void SExtAssetView::OnCollectionUpdated( const FCollectionNameType& Collection )
{
	// A collection has changed in some way, so we need to refresh our backend list
	RequestSlowFullListRefresh();
}

void SExtAssetView::OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	// Remove the old asset, if it exists
	FName OldObjectPackageName = *OldObjectPath;
	RemoveAssetByPath( OldObjectPackageName );
	RecentlyAddedAssets.RemoveAllSwap( [&](const FAssetData& Other) { return Other.ObjectPath == OldObjectPackageName; } );

	// Add the new asset, if it should be in the cached list
	OnAssetAdded( AssetData );

	// Force an update of the recently added asset next frame
	RequestAddNewAssetsNextFrame();
}


void SExtAssetView::OnAssetLoaded(UObject* Asset)
{
	if (Asset == nullptr)
	{
		return;
	}

	FName AssetPathName = FName(*Asset->GetPathName());
	RecentlyLoadedOrChangedAssets.Add( FAssetData(Asset) );

	UTexture2D* Texture2D = Cast<UTexture2D>(Asset);
	UMaterial* Material = Texture2D ? nullptr : Cast<UMaterial>(Asset);
	if ((Texture2D && !Texture2D->bForceMiplevelsToBeResident) || Material)
	{
		bool bHasWidgetForAsset = false;
		switch (GetCurrentViewType())
		{
		case EAssetViewType::List:
			bHasWidgetForAsset = ListView->HasWidgetForAsset(AssetPathName);
			break;
		case EAssetViewType::Tile:
			bHasWidgetForAsset = TileView->HasWidgetForAsset(AssetPathName);
			break;
		default:
			bHasWidgetForAsset = false;
			break;
		}

		if (bHasWidgetForAsset)
		{
			if (Texture2D)
			{
				Texture2D->bForceMiplevelsToBeResident = true;
			}
			else if (Material)
			{
				Material->SetForceMipLevelsToBeResident(true, true, -1.0f);
			}
		}
	};
}

void SExtAssetView::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object != nullptr && Object->IsAsset())
	{
		RecentlyLoadedOrChangedAssets.Add(FAssetData(Object));
	}
}

void SExtAssetView::OnClassHierarchyUpdated()
{
	// The class hierarchy has changed in some way, so we need to refresh our backend list
	RequestSlowFullListRefresh();
}
#endif

void SExtAssetView::OnAssetUpdated(const FExtAssetData& AssetData)
{
	RecentlyLoadedOrChangedAssets.Add(AssetData);
}

void SExtAssetView::OnFrontendFiltersChanged()
{
	RequestQuickFrontendListRefresh();

	// If we're not operating on recursively filtered data, we need to ensure a full slow
	// refresh is performed.
	if ( ShouldFilterRecursively() && !bWereItemsRecursivelyFiltered )
	{
		RequestSlowFullListRefresh();
	}
}

bool SExtAssetView::IsFrontendFilterActive() const
{
	return ( FrontendFilters.IsValid() && FrontendFilters->Num() > 0 );
}

bool SExtAssetView::PassesCurrentFrontendFilter(const FExtAssetData& Item) const
{
	// Check the frontend filters list
	if ( FrontendFilters.IsValid() && !FrontendFilters->PassesAllFilters(Item) )
	{
		return false;
	}

	return true;
}

#if ECB_FEA_ASYNC_ASSET_DISCOVERY
bool SExtAssetView::PassesCurrentBackendFilter(const FExtAssetData& Item) const
{
	TArray<FExtAssetData> AssetDataList;
	AssetDataList.Add(Item);
	RunAssetsThroughBackendFilter(AssetDataList);
	return AssetDataList.Num() == 1;
}

void SExtAssetView::RunAssetsThroughBackendFilter(TArray<FExtAssetData>& InOutAssetDataList) const
{
	const bool bRecurse = ShouldFilterRecursively();
	const bool bUsingFolders = IsShowingFolders();
	const bool bIsDynamicCollection = SourcesData.IsDynamicCollection();
	FARFilter Filter = SourcesData.MakeFilter(bRecurse, bUsingFolders);
	
	if ( SourcesData.HasCollections() && Filter.SoftObjectPaths.Num() == 0 && !bIsDynamicCollection )
	{
		// This is an empty collection, no asset will pass the check
		InOutAssetDataList.Reset();
	}
	else
	{
		// Actually append the backend filter
		Filter.Append(BackendFilter);

		FExtContentBrowserSingleton::GetAssetRegistry().RunAssetsThroughFilter(InOutAssetDataList, Filter);
#if ECB_LEGACY
		if ( SourcesData.HasCollections() && !bIsDynamicCollection )
		{
			// Include objects from child collections if we're recursing
			const ECollectionRecursionFlags::Flags CollectionRecursionMode = (Filter.bRecursivePaths) ? ECollectionRecursionFlags::SelfAndChildren : ECollectionRecursionFlags::Self;

			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
			TArray< FName > CollectionObjectPaths;
			for (const FCollectionNameType& Collection : SourcesData.Collections)
			{
				CollectionManagerModule.Get().GetObjectsInCollection(Collection.Name, Collection.Type, CollectionObjectPaths, CollectionRecursionMode);
			}

			for ( int32 AssetDataIdx = InOutAssetDataList.Num() - 1; AssetDataIdx >= 0; --AssetDataIdx )
			{
				const FAssetData& AssetData = InOutAssetDataList[AssetDataIdx];

				if ( !CollectionObjectPaths.Contains( AssetData.ObjectPath ) )
				{
					InOutAssetDataList.RemoveAtSwap(AssetDataIdx);
				}
			}
		}
#endif
	}
}
#endif

void SExtAssetView::SortList(bool bSyncToSelection)
{
	SortManager.SortList(FilteredAssetItems, MajorityAssetType, CustomColumns);

	// Update the thumbnails we were using since the order has changed
	bPendingUpdateThumbnails = true;

	if (bSyncToSelection)
	{
		// Make sure the selection is in view
		const bool bFocusOnSync = false;
		SyncToSelection(bFocusOnSync);
	}

	RefreshList();
	bPendingSortFilteredItems = false;
	LastSortTime = CurrentTime;
}

FLinearColor SExtAssetView::GetThumbnailHintColorAndOpacity() const
{
	//We update this color in tick instead of here as an optimization
	return ThumbnailHintColorAndOpacity;
}

FSlateColor SExtAssetView::GetViewButtonForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	return ViewOptionsComboButton->IsHovered() ? FAppStyle::GetSlateColor(InvertedForegroundName) : FAppStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SExtAssetView::GetViewButtonContent()
{
	UExtContentBrowserSettings* ExtContentBrowserSetting = GetMutableDefault<UExtContentBrowserSettings>();

	// Get all menu extenders for this context menu from the content browser module
	FExtContentBrowserModule& ExtContentBrowserModule = FModuleManager::GetModuleChecked<FExtContentBrowserModule>( TEXT("ExtContentBrowser") );
	TArray<FExtContentBrowserModule::FExtContentBrowserMenuExtender> MenuExtenderDelegates = ExtContentBrowserModule.GetAllAssetViewViewMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute());
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, NULL, MenuExtender, /*bCloseSelfOnly=*/ true);

	MenuBuilder.BeginSection("AssetViewType", LOCTEXT("ViewTypeHeading", "View Type"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TileViewOption", "Tiles"),
			LOCTEXT("TileViewOptionToolTip", "View assets as tiles in a grid."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &SExtAssetView::SetCurrentViewTypeFromMenu, EAssetViewType::Tile ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SExtAssetView::IsCurrentViewType, EAssetViewType::Tile )
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ListViewOption", "List"),
			LOCTEXT("ListViewOptionToolTip", "View assets in a list with thumbnails."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &SExtAssetView::SetCurrentViewTypeFromMenu, EAssetViewType::List ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SExtAssetView::IsCurrentViewType, EAssetViewType::List )
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
			);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ColumnViewOption", "Columns"),
			LOCTEXT("ColumnViewOptionToolTip", "View assets in a list with columns of details."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &SExtAssetView::SetCurrentViewTypeFromMenu, EAssetViewType::Column ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SExtAssetView::IsCurrentViewType, EAssetViewType::Column )
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
			);
	}
	MenuBuilder.EndSection();

	if (GetColumnViewVisibility() == EVisibility::Visible)
	{
		MenuBuilder.BeginSection("AssetColumns", LOCTEXT("ToggleColumnsHeading", "Columns"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ShowAssetTypeColumns", "Show Asset Type Columns"),
				LOCTEXT("ShowAssetTypeColumnsTooltip", "Show or hide major asset type columns."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SExtAssetView::ToggleMajorAssetTypeColumns),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingMajorAssetTypeColumns)
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);

			MenuBuilder.AddSubMenu(
				LOCTEXT("ToggleColumnsMenu", "Toggle Columns"),
				LOCTEXT("ToggleColumnsMenuTooltip", "Show or hide specific columns."),
				FNewMenuDelegate::CreateSP(this, &SExtAssetView::FillToggleColumnsMenu),
				false,
				FSlateIcon(),
				false
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("ResetColumns", "Reset Columns"),
				LOCTEXT("ResetColumnsToolTip", "Reset all columns to be visible again."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SExtAssetView::ResetColumns)),
				NAME_None,
				EUserInterfaceActionType::Button
			);
#if ECB_WIP_MORE_VIEWTYPE
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ExportColumns", "Export to CSV"),
				LOCTEXT("ExportColumnsToolTip", "Export column data to CSV."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SExtAssetView::ExportColumns)),
				NAME_None,
				EUserInterfaceActionType::Button
			);
#endif
		}

		MenuBuilder.EndSection();
	}
	
	MenuBuilder.BeginSection("View", LOCTEXT("ViewHeading", "View"));
	{
#if ECB_FEA_ENGINE_VERSION_OVERLAY
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowEngineVersionOverlayOption", "Show Engine Version Overlay"),
			LOCTEXT("ShowEngineVersionOverlayToolTip", "Show saved engine version on asset thumbnail?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowEngineVersionOverlayButton),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingEngineVersionOverlayButton)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

#if ECB_FEA_VALIDATE_OVERLAY
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowValidationOverlayOption", "Show Validation Status Overlay"),
			LOCTEXT("ShowValidationOverlayToolTip", "Show if an uasset file's validation status as overlay on asset thumbnail?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowValidationStatusOverlayButton),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingValidationStatusOverlayButton)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

#if ECB_WIP_CONTENT_TYPE_OVERLAY
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowContentTypeOverlayOption", "Show Content Type Overlay"),
			LOCTEXT("ShowContentTypeOverlayToolTip", "Show project/plugin/orphan content type as overlay"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowContentTypeOverlayButton),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingContentTypeOverlayButton)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

#if ECB_FEA_SHOW_INVALID
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowInvalidAssets", "Show Invalid Assets"),
			LOCTEXT("ShowInvalidAssetsToolTip", "Display invalid assets in the asset view?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowInvalidAssets),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingInvalidAssets)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

#if ECB_WIP_TOGGLE_TOOLBAR_BUTTON
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowToolbarButtonTooltipOption", "Show UAsset Toolbar Button"),
			LOCTEXT("ShowToolbarButtonOptionToolTip", "Show Open UAsset Browser button in level editor toolbar?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowToolbarButton),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingToolbarButton)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

		auto CreateShowFoldersSubMenu = [this](FMenuBuilder& SubMenuBuilder)
		{
			SubMenuBuilder.AddMenuEntry(
				LOCTEXT("ShowEmptyFoldersOption", "Show Empty Folders"),
				LOCTEXT("ShowEmptyFoldersOptionToolTip", "Show empty folders in the view as well as assets?"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &SExtAssetView::ToggleShowEmptyFolders ),
					FCanExecuteAction::CreateSP( this, &SExtAssetView::IsToggleShowEmptyFoldersAllowed ),
					FIsActionChecked::CreateSP( this, &SExtAssetView::IsShowingEmptyFolders )
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		};

		MenuBuilder.AddSubMenu(
			LOCTEXT("ShowFoldersOption", "Show Folders"),
			LOCTEXT("ShowFoldersOptionToolTip", "Show folders in the view as well as assets?"),
			FNewMenuDelegate::CreateLambda(CreateShowFoldersSubMenu),
			FUIAction(
				FExecuteAction::CreateSP( this, &SExtAssetView::ToggleShowFolders ),
				FCanExecuteAction::CreateSP( this, &SExtAssetView::IsToggleShowFoldersAllowed ),
				FIsActionChecked::CreateSP( this, &SExtAssetView::IsShowingFolders )
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

#if ECB_FEA_IGNORE_FOLDERS
		MenuBuilder.AddMenuEntry(
			LOCTEXT("IgnoreFoldersStartWithDot", "Ignore Folders Start With Dot"),
			LOCTEXT("IgnoreFoldersStartWithDotTooltip", "All Folders Start with Dot will be ignored in folder scan process"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->bIgnoreFoldersStartWithDot = !ExtContentBrowserSetting->bIgnoreFoldersStartWithDot; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->bIgnoreFoldersStartWithDot; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("IgnoreCommonNonContentFolders", "Ignore Common Non-Content Folders"),
			LOCTEXT("IgnoreCommonNonContentFoldersTooltip", "Common Non-Content Folders (Binaries, Config,DerivedDataCache, Intermediate, Resources, Saved, Source) will be ignored."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->bIgnoreCommonNonContentFolders = !ExtContentBrowserSetting->bIgnoreCommonNonContentFolders; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->bIgnoreCommonNonContentFolders; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("IgnoreExternalContentFolders", "Ignore External Package Content Folders"),
			LOCTEXT("IgnoreExternalContentFoldersTooltip", "Common External Package Folders (__ExternalActors__, __ExternalObjects__) will be ignored."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->bIgnoreExternalContentFolders = !ExtContentBrowserSetting->bIgnoreExternalContentFolders; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->bIgnoreExternalContentFolders; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("MoreIgnoreFolders", "More Ignore Folders"),
			LOCTEXT("MoreIgnoreFoldersTooltip", "Add more ignore folders..."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->bIgnoreMoreFolders = !ExtContentBrowserSetting->bIgnoreMoreFolders; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->bIgnoreMoreFolders; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);


		TSharedRef<SWidget> IgnoreFoldersWidget = SNew(SEditableTextBox)
			.Text_Lambda([this, ExtContentBrowserSetting]()
		{
			FString IgnoreFolders;
			for (int32 Index = 0; Index < ExtContentBrowserSetting->IgnoreFolders.Num(); ++Index)
			{
				if (Index != 0)
				{
					IgnoreFolders.Append(TEXT(","));
				}
				IgnoreFolders += ExtContentBrowserSetting->IgnoreFolders[Index];
			}
			return FText::FromString(IgnoreFolders);
		})
			.IsEnabled_Lambda([this, ExtContentBrowserSetting]()
		{
			return ExtContentBrowserSetting->bIgnoreMoreFolders;
		})
			//.OnTextChanged_Lambda([this](const FText& InText) {})
			.OnTextCommitted_Lambda([this, ExtContentBrowserSetting](const FText& InText, ETextCommit::Type)
		{
			FString InputText = InText.ToString().TrimStartAndEnd();
			if (!InputText.IsEmpty())
			{
				TArray<FString> Candidates;
				InText.ToString().ParseIntoArray(Candidates, TEXT(","));
				for (const FString& Candidate : Candidates)
				{
					FString Trimmed = Candidate.TrimStartAndEnd();
					if (!Trimmed.IsEmpty())
					{
						ExtContentBrowserSetting->IgnoreFolders.AddUnique(Trimmed);
					}
				}
			}
			else
			{
				ExtContentBrowserSetting->IgnoreFolders.Empty();
			}
			ExtContentBrowserSetting->PostEditChange();
		});

		MenuBuilder.AddWidget(IgnoreFoldersWidget, FText::GetEmpty());
#endif

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowAssetTooltipOption", "Show Tooltip"),
			LOCTEXT("ShowAssetTooltipOptionToolTip", "Show the asset tooltip?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowAssetTooltip),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingAssetTooltip)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

#if ECB_WIP_COLLECTION
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowCollectionOption", "Show Collections"),
			LOCTEXT("ShowCollectionOptionToolTip", "Show the collections list in the view?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &SExtAssetView::ToggleShowCollections ),
				FCanExecuteAction::CreateSP( this, &SExtAssetView::IsToggleShowCollectionsAllowed ),
				FIsActionChecked::CreateSP( this, &SExtAssetView::IsShowingCollections )
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

#if ECB_WIP_FAVORITE
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowFavoriteOptions", "Show Favorites"),
			LOCTEXT("ShowFavoriteOptionToolTip", "Show the favorite folders in the view?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleShowFavorites),
				FCanExecuteAction::CreateSP(this, &SExtAssetView::IsToggleShowFavoritesAllowed),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsShowingFavorites)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

		MenuBuilder.AddMenuEntry(
			LOCTEXT("Reset", "Reset"),
			LOCTEXT("ResetOptionsTooltip", "Reset view options to default"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->ResetViewSettings(); ExtContentBrowserSetting->PostEditChange(); })
				//FCanExecuteAction(),
				//FIsActionChecked::CreateLambda([this] { return false; })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection(); // Section: View

#if ECB_DEBUG
	MenuBuilder.BeginSection("Debug", LOCTEXT("DebugHeading", "Debug"));
	{
		MenuBuilder.AddWidget(
			SNew(SGridPanel)
			+ SGridPanel::Slot(0, 0).Padding(3, 0, 0, 0)
			[WidgetHelpers::CreateColorWidget(&FExtContentBrowserStyle::CustomContentBrowserBorderBackgroundColor)]
			+ SGridPanel::Slot(1, 0).Padding(3, 0, 0, 0)
			[WidgetHelpers::CreateColorWidget(&FExtContentBrowserStyle::CustomToolbarBackgroundColor)]
			+ SGridPanel::Slot(2, 0).Padding(3, 0, 0, 0)
			[WidgetHelpers::CreateColorWidget(&FExtContentBrowserStyle::CustomSourceViewBackgroundColor)]
			+ SGridPanel::Slot(3, 0).Padding(3, 0, 0, 0)
			[WidgetHelpers::CreateColorWidget(&FExtContentBrowserStyle::CustomAssetViewBackgroundColor)]
			, LOCTEXT("Color", "Color")
			,/*bNoIndent=*/true
		);
	}
	MenuBuilder.EndSection();
#endif

#if ECB_TODO
	MenuBuilder.BeginSection("Content", LOCTEXT("ContentHeading", "Content"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowEngineFolderOption", "Show Engine Content"),
			LOCTEXT("ShowEngineFolderOptionToolTip", "Show engine content in the view?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &SExtAssetView::ToggleShowEngineContent ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SExtAssetView::IsShowingEngineContent )
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();
#endif

#if ECB_WIP_SEARCH_RECURSE_TOGGLE
	MenuBuilder.BeginSection("Search", LOCTEXT("SearchHeading", "Search"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SearchFilterRecursivelyOption", "Search and Filter Recursively"),
			LOCTEXT("SearchFilterRecursivelyOptionTooltip", "Search and filter recursively or only in current folder?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleSearchAndFilterRecursively),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsSearchAndFilterRecursively)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

#if ECB_LEGACY
		MenuBuilder.AddMenuEntry(
			LOCTEXT("IncludeClassNameOption", "Search Asset Class Names"),
			LOCTEXT("IncludeClassesNameOptionTooltip", "Include asset type names in search criteria?  (e.g. Blueprint, Texture, Sound)"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleIncludeClassNames),
				FCanExecuteAction::CreateSP(this, &SExtAssetView::IsToggleIncludeClassNamesAllowed),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsIncludingClassNames)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("IncludeAssetPathOption", "Search Asset Path"),
			LOCTEXT("IncludeAssetPathOptionTooltip", "Include entire asset path in search criteria?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleIncludeAssetPaths),
				FCanExecuteAction::CreateSP(this, &SExtAssetView::IsToggleIncludeAssetPathsAllowed),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsIncludingAssetPaths)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("IncludeCollectionNameOption", "Search Collection Names"),
			LOCTEXT("IncludeCollectionNameOptionTooltip", "Include Collection names in search criteria?"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ToggleIncludeCollectionNames),
				FCanExecuteAction::CreateSP(this, &SExtAssetView::IsToggleIncludeCollectionNamesAllowed),
				FIsActionChecked::CreateSP(this, &SExtAssetView::IsIncludingCollectionNames)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif
	}
	MenuBuilder.EndSection();
#endif

#if ECB_LEGACY // Move outside
	MenuBuilder.BeginSection("AssetThumbnails", LOCTEXT("ThumbnailsHeading", "Thumbnails"));
	{
		MenuBuilder.AddWidget(
			SNew(SSlider)
				.ToolTipText( LOCTEXT("ThumbnailScaleToolTip", "Adjust the size of thumbnails.") )
				.Value( this, &SExtAssetView::GetThumbnailScale )
				.OnValueChanged( this, &SExtAssetView::SetThumbnailScale )
				.Locked( this, &SExtAssetView::IsThumbnailScalingLocked ),
			LOCTEXT("ThumbnailScaleLabel", "Scale"),
			/*bNoIndent=*/true
			);
	}
	MenuBuilder.EndSection();
#endif

	const auto PluginDescriptor = IPluginManager::Get().FindPlugin(TEXT("UAssetBrowser"))->GetDescriptor();
	const FText VersionName = FText::FromString(PluginDescriptor.VersionName);
	const FText PluginVersion = FText::Format(LOCTEXT("UAssetBrowserVersion", "UAsset Browser {0}"), VersionName);

	MenuBuilder.BeginSection("About", LOCTEXT("AboutHeading", "About"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Documentation...", "Documentation..."),
			LOCTEXT("DocumentationToolTip", "Show documentation in a browser"),
			FSlateIcon(FExtContentBrowserStyle::GetStyleSetName(), "UAssetBrowser.Help"),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ShowDocumentation)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		MenuBuilder.AddMenuEntry(
			PluginVersion,
			LOCTEXT("ShowChangeLogToolTip", "Show change log"),
			FSlateIcon(FExtContentBrowserStyle::GetStyleSetName(), "UAssetBrowser.Icon16x"),
			FUIAction(
				FExecuteAction::CreateSP(this, &SExtAssetView::ShowPluginVersionChangeLog)
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SExtAssetView::ToggleShowFolders()
{
	check( IsToggleShowFoldersAllowed() );
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayFolders = !GetDefault<UExtContentBrowserSettings>()->DisplayFolders;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsToggleShowFoldersAllowed() const
{
	return bCanShowFolders;
}

bool SExtAssetView::IsShowingFolders() const
{
	return IsToggleShowFoldersAllowed() && GetDefault<UExtContentBrowserSettings>()->DisplayFolders;
}

void SExtAssetView::ToggleShowEmptyFolders()
{
	check( IsToggleShowEmptyFoldersAllowed() );
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayEmptyFolders = !GetDefault<UExtContentBrowserSettings>()->DisplayEmptyFolders;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsToggleShowEmptyFoldersAllowed() const
{
	return bCanShowFolders;
}

bool SExtAssetView::IsShowingEmptyFolders() const
{
	return IsToggleShowEmptyFoldersAllowed() && GetDefault<UExtContentBrowserSettings>()->DisplayEmptyFolders;
}

void SExtAssetView::ToggleShowAssetTooltip()
{
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayAssetTooltip = !GetDefault<UExtContentBrowserSettings>()->DisplayAssetTooltip;
}

bool SExtAssetView::IsShowingAssetTooltip() const
{
	return GetDefault<UExtContentBrowserSettings>()->DisplayAssetTooltip;
}

void SExtAssetView::ShowDocumentation()
{
	FString Link(TEXT("https://marynate.github.io/uasset-browser/"));
	FPlatformProcess::LaunchURL(*Link, nullptr, nullptr);
}

void SExtAssetView::ShowPluginVersionChangeLog()
{
	OnRequestShowChangeLog.ExecuteIfBound();
}

void SExtAssetView::ToggleShowEngineVersionOverlayButton()
{
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayEngineVersionOverlay = !GetDefault<UExtContentBrowserSettings>()->DisplayEngineVersionOverlay;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsShowingEngineVersionOverlayButton() const
{
	return GetDefault<UExtContentBrowserSettings>()->DisplayEngineVersionOverlay;
}

void SExtAssetView::ToggleShowContentTypeOverlayButton()
{
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayContentTypeOverlay = !GetDefault<UExtContentBrowserSettings>()->DisplayContentTypeOverlay;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsShowingContentTypeOverlayButton() const
{
	return GetDefault<UExtContentBrowserSettings>()->DisplayContentTypeOverlay;
}

void SExtAssetView::ToggleShowValidationStatusOverlayButton()
{
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayValidationStatusOverlay = !GetDefault<UExtContentBrowserSettings>()->DisplayValidationStatusOverlay;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsShowingValidationStatusOverlayButton() const
{
	return GetDefault<UExtContentBrowserSettings>()->DisplayValidationStatusOverlay;
}

void SExtAssetView::ToggleShowInvalidAssets()
{
	GetMutableDefault<UExtContentBrowserSettings>()->DisplayInvalidAssets = !GetDefault<UExtContentBrowserSettings>()->DisplayInvalidAssets;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsShowingInvalidAssets() const
{
	return GetDefault<UExtContentBrowserSettings>()->DisplayInvalidAssets;
}

void SExtAssetView::ToggleShowToolbarButton()
{
	FExtContentBrowserSingleton::Get().ToggleShowToolbarButton();
}

bool SExtAssetView::IsShowingToolbarButton() const
{
	return GetDefault<UExtContentBrowserSettings>()->DisplayToolbarButton;
}

#if ECB_LEGACY

void SExtAssetView::ToggleShowEngineContent()
{
	bool bDisplayEngine = GetDefault<UExtContentBrowserSettings>()->GetDisplayEngineFolder();
	bool bRawDisplayEngine = GetDefault<UExtContentBrowserSettings>()->GetDisplayEngineFolder( true );

	// Only if both these flags are false when toggling we want to enable the flag, otherwise we're toggling off
	if ( !bDisplayEngine && !bRawDisplayEngine )
	{
		GetMutableDefault<UExtContentBrowserSettings>()->SetDisplayEngineFolder( true );
	}
	else
	{
		GetMutableDefault<UExtContentBrowserSettings>()->SetDisplayEngineFolder( false );
		GetMutableDefault<UExtContentBrowserSettings>()->SetDisplayEngineFolder( false, true );
	}	
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsShowingEngineContent() const
{
	return GetDefault<UExtContentBrowserSettings>()->GetDisplayEngineFolder();
}

#endif

void SExtAssetView::ToggleShowCollections()
{
	const bool bDisplayCollections = GetDefault<UExtContentBrowserSettings>()->GetDisplayCollections();
	GetMutableDefault<UExtContentBrowserSettings>()->SetDisplayCollections( !bDisplayCollections );
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsToggleShowCollectionsAllowed() const
{
	return bCanShowCollections;
}

bool SExtAssetView::IsShowingCollections() const
{
	return IsToggleShowCollectionsAllowed() && GetDefault<UExtContentBrowserSettings>()->GetDisplayCollections();
}

void SExtAssetView::ToggleShowFavorites()
{
	const bool bShowingFavorites = GetDefault<UExtContentBrowserSettings>()->GetDisplayFavorites();
	GetMutableDefault<UExtContentBrowserSettings>()->SetDisplayFavorites(!bShowingFavorites);
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsToggleShowFavoritesAllowed() const
{
	return bCanShowFavorites;
}

bool SExtAssetView::IsShowingFavorites() const
{
	return IsToggleShowFavoritesAllowed() && GetDefault<UExtContentBrowserSettings>()->GetDisplayFavorites();
}

void SExtAssetView::ToggleSearchAndFilterRecursively()
{
	GetMutableDefault<UExtContentBrowserSettings>()->SearchAndFilterRecursively = !GetDefault<UExtContentBrowserSettings>()->SearchAndFilterRecursively;
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();
}

bool SExtAssetView::IsSearchAndFilterRecursively() const
{
#if ECB_WIP_SEARCH_RECURSE_TOGGLE
	return GetDefault<UExtContentBrowserSettings>()->SearchAndFilterRecursively;
#else
	return true;
#endif
}

#if ECB_LEGACY
void SExtAssetView::ToggleIncludeClassNames()
{
	const bool bIncludeClassNames = GetDefault<UExtContentBrowserSettings>()->GetIncludeClassNames();
	GetMutableDefault<UExtContentBrowserSettings>()->SetIncludeClassNames(!bIncludeClassNames);
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();

	OnSearchOptionsChanged.ExecuteIfBound();
}

bool SExtAssetView::IsToggleIncludeClassNamesAllowed() const
{
	return true;
}

bool SExtAssetView::IsIncludingClassNames() const
{
	return IsToggleIncludeClassNamesAllowed() && GetDefault<UExtContentBrowserSettings>()->GetIncludeClassNames();
}

void SExtAssetView::ToggleIncludeAssetPaths()
{
	const bool bIncludeAssetPaths = GetDefault<UExtContentBrowserSettings>()->GetIncludeAssetPaths();
	GetMutableDefault<UExtContentBrowserSettings>()->SetIncludeAssetPaths(!bIncludeAssetPaths);
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();

	OnSearchOptionsChanged.ExecuteIfBound();
}

bool SExtAssetView::IsToggleIncludeAssetPathsAllowed() const
{
	return true;
}

bool SExtAssetView::IsIncludingAssetPaths() const
{
	return IsToggleIncludeAssetPathsAllowed() && GetDefault<UExtContentBrowserSettings>()->GetIncludeAssetPaths();
}

void SExtAssetView::ToggleIncludeCollectionNames()
{
	const bool bIncludeCollectionNames = GetDefault<UExtContentBrowserSettings>()->GetIncludeCollectionNames();
	GetMutableDefault<UExtContentBrowserSettings>()->SetIncludeCollectionNames(!bIncludeCollectionNames);
	GetMutableDefault<UExtContentBrowserSettings>()->PostEditChange();

	OnSearchOptionsChanged.ExecuteIfBound();
}

bool SExtAssetView::IsToggleIncludeCollectionNamesAllowed() const
{
	return true;
}

bool SExtAssetView::IsIncludingCollectionNames() const
{
	return IsToggleIncludeCollectionNamesAllowed() && GetDefault<UExtContentBrowserSettings>()->GetIncludeCollectionNames();
}

#endif

void SExtAssetView::SetCurrentViewType(EAssetViewType::Type NewType)
{
	if ( ensure(NewType != EAssetViewType::MAX) && NewType != CurrentViewType )
	{
		//ResetQuickJump();

		CurrentViewType = NewType;
		CreateCurrentView();

		SyncToSelection();

		// Clear relevant thumbnails to render fresh ones in the new view if needed
		RelevantThumbnails.Reset();
		VisibleItems.Reset();

		if ( NewType == EAssetViewType::Tile )
		{
			CurrentThumbnailSize = TileViewThumbnailSize;
			bPendingUpdateThumbnails = true;
		}
		else if ( NewType == EAssetViewType::List )
		{
			CurrentThumbnailSize = ListViewThumbnailSize;
			bPendingUpdateThumbnails = true;
		}
		else if ( NewType == EAssetViewType::Column )
		{
			// No thumbnails, but we do need to refresh filtered items to determine a majority asset type
			MajorityAssetType = NAME_None;
			RefreshFilteredItems();
			RefreshFolders();
			SortList();
		}
	}
}

void SExtAssetView::SetCurrentViewTypeFromMenu(EAssetViewType::Type NewType)
{
	if (NewType != CurrentViewType)
	{
		SetCurrentViewType(NewType);
		FSlateApplication::Get().DismissAllMenus();
	}
}

void SExtAssetView::CreateCurrentView()
{
	TileView.Reset();
	ListView.Reset();
	ColumnView.Reset();

	TSharedRef<SWidget> NewView = SNullWidget::NullWidget;
	switch (CurrentViewType)
	{
		case EAssetViewType::Tile:
			TileView = CreateTileView();
			NewView = CreateShadowOverlay(TileView.ToSharedRef());
			break;
		case EAssetViewType::List:
			ListView = CreateListView();
			NewView = CreateShadowOverlay(ListView.ToSharedRef());
			break;
		case EAssetViewType::Column:
			ColumnView = CreateColumnView();
			NewView = CreateShadowOverlay(ColumnView.ToSharedRef());
			break;
	}
	
	ViewContainer->SetContent( NewView );
}

TSharedRef<SWidget> SExtAssetView::CreateShadowOverlay( TSharedRef<STableViewBase> Table )
{
	return SNew(SScrollBorder, Table)
		[
			Table
		];
}

EAssetViewType::Type SExtAssetView::GetCurrentViewType() const
{
	return CurrentViewType;
}

bool SExtAssetView::IsCurrentViewType(EAssetViewType::Type ViewType) const
{
	return GetCurrentViewType() == ViewType;
}

void SExtAssetView::FocusList() const
{
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: FSlateApplication::Get().SetKeyboardFocus(ListView, EFocusCause::SetDirectly); break;
		case EAssetViewType::Tile: FSlateApplication::Get().SetKeyboardFocus(TileView, EFocusCause::SetDirectly); break;
		case EAssetViewType::Column: FSlateApplication::Get().SetKeyboardFocus(ColumnView, EFocusCause::SetDirectly); break;
	}
}

void SExtAssetView::RefreshList()
{
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: ListView->RequestListRefresh(); break;
		case EAssetViewType::Tile: TileView->RequestListRefresh(); break;
		case EAssetViewType::Column: ColumnView->RequestListRefresh(); break;
	}
}

void SExtAssetView::SetSelection(const TSharedPtr<FExtAssetViewItem>& Item)
{
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: ListView->SetSelection(Item); break;
		case EAssetViewType::Tile: TileView->SetSelection(Item); break;
		case EAssetViewType::Column: ColumnView->SetSelection(Item); break;
	}
}

void SExtAssetView::SetItemSelection(const TSharedPtr<FExtAssetViewItem>& Item, bool bSelected, const ESelectInfo::Type SelectInfo)
{
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: ListView->SetItemSelection(Item, bSelected, SelectInfo); break;
		case EAssetViewType::Tile: TileView->SetItemSelection(Item, bSelected, SelectInfo); break;
		case EAssetViewType::Column: ColumnView->SetItemSelection(Item, bSelected, SelectInfo); break;
	}
}

void SExtAssetView::RequestScrollIntoView(const TSharedPtr<FExtAssetViewItem>& Item)
{
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: ListView->RequestScrollIntoView(Item); break;
		case EAssetViewType::Tile: TileView->RequestScrollIntoView(Item); break;
		case EAssetViewType::Column: ColumnView->RequestScrollIntoView(Item); break;
	}
}
#if ECB_TODO
void SExtAssetView::OnOpenAssetsOrFolders()
{
	TArray<FAssetData> SelectedAssets = GetSelectedAssets();
	TArray<FString> SelectedFolders = GetSelectedFolders();
	if (SelectedAssets.Num() > 0 && SelectedFolders.Num() == 0)
	{
		OnAssetsActivated.ExecuteIfBound(SelectedAssets, EAssetTypeActivationMethod::Opened);
	}
	else if (SelectedAssets.Num() == 0 && SelectedFolders.Num() > 0)
	{
		OnPathSelected.ExecuteIfBound(SelectedFolders[0]);
	}
}

void SExtAssetView::OnPreviewAssets()
{
	OnAssetsActivated.ExecuteIfBound(GetSelectedAssets(), EAssetTypeActivationMethod::Previewed);
}
#endif

void SExtAssetView::ClearSelection(bool bForceSilent)
{
	const bool bTempBulkSelectingValue = bForceSilent ? true : bBulkSelecting;
	TGuardValue<bool>(bBulkSelecting, bTempBulkSelectingValue);
	switch ( GetCurrentViewType() )
	{
		case EAssetViewType::List: ListView->ClearSelection(); break;
		case EAssetViewType::Tile: TileView->ClearSelection(); break;
		case EAssetViewType::Column: ColumnView->ClearSelection(); break;
	}
}

TSharedRef<ITableRow> SExtAssetView::MakeListViewWidget(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if ( !ensure(AssetItem.IsValid()) )
	{
		return SNew( STableRow<TSharedPtr<FExtAssetViewAsset>>, OwnerTable );
	}

	VisibleItems.Add(AssetItem);
	bPendingUpdateThumbnails = true;

	if(AssetItem->GetType() == EExtAssetItemType::Folder)
	{
		TSharedPtr< STableRow<TSharedPtr<FExtAssetViewItem>> > TableRowWidget;
		SAssignNew( TableRowWidget, STableRow<TSharedPtr<FExtAssetViewItem>>, OwnerTable )
			.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
			.Cursor( bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default )
			.OnDragDetected( this, &SExtAssetView::OnDraggingAssetItem );

		TSharedRef<SExtAssetListItem> Item =
			SNew(SExtAssetListItem)
			.AssetItem(AssetItem)
			.ItemHeight(this, &SExtAssetView::GetListViewItemHeight)
			.OnItemDestroyed(this, &SExtAssetView::AssetItemWidgetDestroyed)
			.ShouldAllowToolTip(this, &SExtAssetView::ShouldAllowToolTips)
			.HighlightText(HighlightedText)
			.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FExtAssetViewItem>>::IsSelectedExclusively));
			//.OnAssetsOrPathsDragDropped(this, &SExtAssetView::OnAssetsOrPathsDragDropped)
			//.OnFilesDragDropped(this, &SExtAssetView::OnFilesDragDropped);

		TableRowWidget->SetContent(Item);

		return TableRowWidget.ToSharedRef();
	}
	else
	{
		TSharedPtr<FExtAssetViewAsset> AssetItemAsAsset = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem);

		TSharedPtr<FExtAssetThumbnail>* AssetThumbnailPtr = RelevantThumbnails.Find(AssetItemAsAsset);
		TSharedPtr<FExtAssetThumbnail> AssetThumbnail;
		if ( AssetThumbnailPtr )
		{
			AssetThumbnail = *AssetThumbnailPtr;
		}
		else
		{
			const float ThumbnailResolution = ListViewThumbnailResolution;
			AssetThumbnail = MakeShareable( new FExtAssetThumbnail( AssetItemAsAsset->Data, ThumbnailResolution, ThumbnailResolution, AssetThumbnailPool ) );
			RelevantThumbnails.Add( AssetItemAsAsset, AssetThumbnail );
			AssetThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render
		}

		TSharedPtr< STableRow<TSharedPtr<FExtAssetViewItem>> > TableRowWidget;
		SAssignNew( TableRowWidget, STableRow<TSharedPtr<FExtAssetViewItem>>, OwnerTable )
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Cursor( bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default )
		.OnDragDetected( this, &SExtAssetView::OnDraggingAssetItem );

		TSharedRef<SExtAssetListItem> Item =
			SNew(SExtAssetListItem)
			.AssetThumbnail(AssetThumbnail)
			.AssetItem(AssetItem)
			.ThumbnailPadding(ListViewThumbnailPadding)
			.ItemHeight(this, &SExtAssetView::GetListViewItemHeight)
			.OnItemDestroyed(this, &SExtAssetView::AssetItemWidgetDestroyed)
			.ShouldAllowToolTip(this, &SExtAssetView::ShouldAllowToolTips)
			.HighlightText(HighlightedText)
			.ThumbnailLabel(ThumbnailLabel)
			.ThumbnailHintColorAndOpacity(this, &SExtAssetView::GetThumbnailHintColorAndOpacity)
			.AllowThumbnailHintLabel(AllowThumbnailHintLabel)
			.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FExtAssetViewItem>>::IsSelectedExclusively));
			//.OnIsAssetValidForCustomToolTip(OnIsAssetValidForCustomToolTip)
			//.OnGetCustomAssetToolTip(OnGetCustomAssetToolTip)
			//.OnVisualizeAssetToolTip(OnVisualizeAssetToolTip)
			//.OnAssetToolTipClosing(OnAssetToolTipClosing);

		TableRowWidget->SetContent(Item);

		return TableRowWidget.ToSharedRef();
	}
}

TSharedRef<ITableRow> SExtAssetView::MakeTileViewWidget(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if ( !ensure(AssetItem.IsValid()) )
	{
		return SNew( STableRow<TSharedPtr<FExtAssetViewAsset>>, OwnerTable );
	}

	VisibleItems.Add(AssetItem);
	bPendingUpdateThumbnails = true;

	if(AssetItem->GetType() == EExtAssetItemType::Folder)
	{
		TSharedPtr< STableRow<TSharedPtr<FExtAssetViewItem>> > TableRowWidget;
		SAssignNew( TableRowWidget, STableRow<TSharedPtr<FExtAssetViewItem>>, OwnerTable )
			.Style( FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow" )
			.Cursor( bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default )
			.OnDragDetected( this, &SExtAssetView::OnDraggingAssetItem );

		TSharedRef<SExtAssetTileItem> Item =
			SNew(SExtAssetTileItem)
			.AssetItem(AssetItem)
			.ItemWidth(this, &SExtAssetView::GetTileViewItemWidth)
			.OnItemDestroyed(this, &SExtAssetView::AssetItemWidgetDestroyed)
			.ShouldAllowToolTip(this, &SExtAssetView::ShouldAllowToolTips)
			.HighlightText(HighlightedText)
			.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FExtAssetViewItem>>::IsSelectedExclusively));
//			.OnAssetsOrPathsDragDropped(this, &SExtAssetView::OnAssetsOrPathsDragDropped)
//			.OnFilesDragDropped(this, &SExtAssetView::OnFilesDragDropped);

		TableRowWidget->SetContent(Item);

		return TableRowWidget.ToSharedRef();
	}
	else
	{
		TSharedPtr<FExtAssetViewAsset> AssetItemAsAsset = StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem);

		TSharedPtr<FExtAssetThumbnail>* AssetThumbnailPtr = RelevantThumbnails.Find(AssetItemAsAsset);
		TSharedPtr<FExtAssetThumbnail> AssetThumbnail;
		if ( AssetThumbnailPtr )
		{
			AssetThumbnail = *AssetThumbnailPtr;
		}
		else
		{
			const float ThumbnailResolution = TileViewThumbnailResolution;
			AssetThumbnail = MakeShareable( new FExtAssetThumbnail( AssetItemAsAsset->Data, ThumbnailResolution, ThumbnailResolution, AssetThumbnailPool ) );
			RelevantThumbnails.Add( AssetItemAsAsset, AssetThumbnail );
			AssetThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render
		}

		TSharedPtr< STableRow<TSharedPtr<FExtAssetViewItem>> > TableRowWidget;
		SAssignNew( TableRowWidget, STableRow<TSharedPtr<FExtAssetViewItem>>, OwnerTable )
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Cursor( bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default )
		.OnDragDetected( this, &SExtAssetView::OnDraggingAssetItem );

		TSharedRef<SExtAssetTileItem> Item =
			SNew(SExtAssetTileItem)
			.AssetThumbnail(AssetThumbnail)
			.AssetItem(AssetItem)
			.ThumbnailPadding(TileViewThumbnailPadding)
			.ItemWidth(this, &SExtAssetView::GetTileViewItemWidth)
			.OnItemDestroyed(this, &SExtAssetView::AssetItemWidgetDestroyed)
			.ShouldAllowToolTip(this, &SExtAssetView::ShouldAllowToolTips)
			.HighlightText(HighlightedText)
			.ThumbnailLabel(ThumbnailLabel)
			.ThumbnailHintColorAndOpacity(this, &SExtAssetView::GetThumbnailHintColorAndOpacity)
			.AllowThumbnailHintLabel(AllowThumbnailHintLabel)
			.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FExtAssetViewItem>>::IsSelectedExclusively));
			//.OnIsAssetValidForCustomToolTip(OnIsAssetValidForCustomToolTip)
			//.OnGetCustomAssetToolTip(OnGetCustomAssetToolTip)
			//.OnVisualizeAssetToolTip( OnVisualizeAssetToolTip )
			//.OnAssetToolTipClosing( OnAssetToolTipClosing );

		TableRowWidget->SetContent(Item);

		return TableRowWidget.ToSharedRef();
	}
}

TSharedRef<ITableRow> SExtAssetView::MakeColumnViewWidget(TSharedPtr<FExtAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if ( !ensure(AssetItem.IsValid()) )
	{
		return SNew( STableRow<TSharedPtr<FExtAssetViewItem>>, OwnerTable )
			.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow");
	}

	// Update the cached custom data
	AssetItem->CacheCustomColumns(CustomColumns, false, true, false);
	
	return
		SNew( SExtAssetColumnViewRow, OwnerTable )
		.OnDragDetected( this, &SExtAssetView::OnDraggingAssetItem )
		.Cursor( bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default )
		.AssetColumnItem(
			SNew(SExtAssetColumnItem)
				.AssetItem(AssetItem)
				.OnItemDestroyed(this, &SExtAssetView::AssetItemWidgetDestroyed)
				.HighlightText( HighlightedText )
				//.OnAssetsOrPathsDragDropped(this, &SExtAssetView::OnAssetsOrPathsDragDropped)
				//.OnFilesDragDropped(this, &SExtAssetView::OnFilesDragDropped)
				//.OnIsAssetValidForCustomToolTip(OnIsAssetValidForCustomToolTip)
				//.OnGetCustomAssetToolTip(OnGetCustomAssetToolTip)
				//.OnVisualizeAssetToolTip( OnVisualizeAssetToolTip )
				//.OnAssetToolTipClosing( OnAssetToolTipClosing )
		);
}

void SExtAssetView::AssetItemWidgetDestroyed(const TSharedPtr<FExtAssetViewItem>& Item)
{
	if ( VisibleItems.Remove(Item) != INDEX_NONE )
	{
		bPendingUpdateThumbnails = true;
	}
}

void SExtAssetView::UpdateThumbnails()
{
	int32 MinItemIdx = INDEX_NONE;
	int32 MaxItemIdx = INDEX_NONE;
	int32 MinVisibleItemIdx = INDEX_NONE;
	int32 MaxVisibleItemIdx = INDEX_NONE;

	const int32 HalfNumOffscreenThumbnails = NumOffscreenThumbnails * 0.5;
	for ( auto ItemIt = VisibleItems.CreateConstIterator(); ItemIt; ++ItemIt )
	{
		int32 ItemIdx = FilteredAssetItems.Find(*ItemIt);
		if ( ItemIdx != INDEX_NONE )
		{
			const int32 ItemIdxLow = FMath::Max<int32>(0, ItemIdx - HalfNumOffscreenThumbnails);
			const int32 ItemIdxHigh = FMath::Min<int32>(FilteredAssetItems.Num() - 1, ItemIdx + HalfNumOffscreenThumbnails);
			if ( MinItemIdx == INDEX_NONE || ItemIdxLow < MinItemIdx )
			{
				MinItemIdx = ItemIdxLow;
			}
			if ( MaxItemIdx == INDEX_NONE || ItemIdxHigh > MaxItemIdx )
			{
				MaxItemIdx = ItemIdxHigh;
			}
			if ( MinVisibleItemIdx == INDEX_NONE || ItemIdx < MinVisibleItemIdx )
			{
				MinVisibleItemIdx = ItemIdx;
			}
			if ( MaxVisibleItemIdx == INDEX_NONE || ItemIdx > MaxVisibleItemIdx )
			{
				MaxVisibleItemIdx = ItemIdx;
			}
		}
	}

	if ( MinItemIdx != INDEX_NONE && MaxItemIdx != INDEX_NONE && MinVisibleItemIdx != INDEX_NONE && MaxVisibleItemIdx != INDEX_NONE )
	{
		// We have a new min and a new max, compare it to the old min and max so we can create new thumbnails
		// when appropriate and remove old thumbnails that are far away from the view area.
		TMap< TSharedPtr<FExtAssetViewAsset>, TSharedPtr<FExtAssetThumbnail> > NewRelevantThumbnails;

		// Operate on offscreen items that are furthest away from the visible items first since the thumbnail pool processes render requests in a LIFO order.
		while (MinItemIdx < MinVisibleItemIdx || MaxItemIdx > MaxVisibleItemIdx)
		{
			const int32 LowEndDistance = MinVisibleItemIdx - MinItemIdx;
			const int32 HighEndDistance = MaxItemIdx - MaxVisibleItemIdx;

			if ( HighEndDistance > LowEndDistance )
			{
				if(FilteredAssetItems.IsValidIndex(MaxItemIdx) && FilteredAssetItems[MaxItemIdx]->GetType() != EExtAssetItemType::Folder)
				{
					AddItemToNewThumbnailRelevancyMap( StaticCastSharedPtr<FExtAssetViewAsset>(FilteredAssetItems[MaxItemIdx]), NewRelevantThumbnails );
				}
				MaxItemIdx--;
			}
			else
			{
				if(FilteredAssetItems.IsValidIndex(MinItemIdx) && FilteredAssetItems[MinItemIdx]->GetType() != EExtAssetItemType::Folder)
				{
					AddItemToNewThumbnailRelevancyMap( StaticCastSharedPtr<FExtAssetViewAsset>(FilteredAssetItems[MinItemIdx]), NewRelevantThumbnails );
				}
				MinItemIdx++;
			}
		}

		// Now operate on VISIBLE items then prioritize them so they are rendered first
		TArray< TSharedPtr<FExtAssetThumbnail> > ThumbnailsToPrioritize;
		for ( int32 ItemIdx = MinVisibleItemIdx; ItemIdx <= MaxVisibleItemIdx; ++ItemIdx )
		{
			if(FilteredAssetItems.IsValidIndex(ItemIdx) && FilteredAssetItems[ItemIdx]->GetType() != EExtAssetItemType::Folder)
			{
				TSharedPtr<FExtAssetThumbnail> Thumbnail = AddItemToNewThumbnailRelevancyMap( StaticCastSharedPtr<FExtAssetViewAsset>(FilteredAssetItems[ItemIdx]), NewRelevantThumbnails );
				if ( Thumbnail.IsValid() )
				{
					ThumbnailsToPrioritize.Add(Thumbnail);
				}
			}
		}

		// Now prioritize all thumbnails there were in the visible range
		if ( ThumbnailsToPrioritize.Num() > 0 )
		{
			AssetThumbnailPool->PrioritizeThumbnails(ThumbnailsToPrioritize, CurrentThumbnailSize, CurrentThumbnailSize);
		}

		// Assign the new map of relevant thumbnails. This will remove any entries that were no longer relevant.
		RelevantThumbnails = NewRelevantThumbnails;
	}
}

TSharedPtr<FExtAssetThumbnail> SExtAssetView::AddItemToNewThumbnailRelevancyMap(const TSharedPtr<FExtAssetViewAsset>& Item, TMap< TSharedPtr<FExtAssetViewAsset>, TSharedPtr<FExtAssetThumbnail> >& NewRelevantThumbnails)
{
	const TSharedPtr<FExtAssetThumbnail>* Thumbnail = RelevantThumbnails.Find(Item);
	if ( Thumbnail )
	{
		// The thumbnail is still relevant, add it to the new list
		NewRelevantThumbnails.Add(Item, *Thumbnail);

		return *Thumbnail;
	}
	else
	{
		if ( !ensure(CurrentThumbnailSize > 0 && CurrentThumbnailSize <= MAX_THUMBNAIL_SIZE) )
		{
			// Thumbnail size must be in a sane range
			CurrentThumbnailSize = 64;
		}

		// The thumbnail newly relevant, create a new thumbnail
		const float ThumbnailResolution = CurrentThumbnailSize * MaxThumbnailScale;
		TSharedPtr<FExtAssetThumbnail> NewThumbnail = MakeShareable( new FExtAssetThumbnail( Item->Data, ThumbnailResolution, ThumbnailResolution, AssetThumbnailPool ) );
		NewRelevantThumbnails.Add( Item, NewThumbnail );
		NewThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render

		return NewThumbnail;
	}
}

void SExtAssetView::AssetSelectionChanged( TSharedPtr< struct FExtAssetViewItem > AssetItem, ESelectInfo::Type SelectInfo )
{
	if ( !bBulkSelecting )
	{
		if ( AssetItem.IsValid() && AssetItem->GetType() != EExtAssetItemType::Folder )
		{
			OnAssetSelected.ExecuteIfBound(StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data);
		}
		else
		{
			OnAssetSelected.ExecuteIfBound(FExtAssetData());
		}
	}
}

void SExtAssetView::ItemScrolledIntoView(TSharedPtr<struct FExtAssetViewItem> AssetItem, const TSharedPtr<ITableRow>& Widget )
{
	const bool bTryRestoreFocus = false; //if ( AssetItem->bRenameWhenScrolledIntoview )
	if (bTryRestoreFocus)
	{
		// Make sure we have window focus to avoid the inline text editor from canceling itself if we try to click on it
		// This can happen if creating an asset opens an intermediary window which steals our focus, 
		// eg, the blueprint and slate widget style class windows (TTP# 314240)
		TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if(OwnerWindow.IsValid())
		{
			OwnerWindow->BringToFront();
		}

		//AwaitingRename = AssetItem;
	}
}

TSharedPtr<SWidget> SExtAssetView::OnGetContextMenuContent()
{
	if ( CanOpenContextMenu() )
	{
		const TArray<FString> SelectedFolders = GetSelectedFolders();
		if(SelectedFolders.Num() > 0)
		{
			return NULL;// OnGetFolderContextMenu.Execute(SelectedFolders, OnGetPathContextMenuExtender);
		}
		else
		{
			return OnGetAssetContextMenu.Execute(GetSelectedAssets());
		}
	}
	return NULL;
}

bool SExtAssetView::CanOpenContextMenu() const
{
#if ECB_LEGACY
	if ( !OnGetAssetContextMenu.IsBound() )
	{
		// You can only a summon a context menu if one is set up
		return false;
	}
#endif
	TArray<FExtAssetData> SelectedAssets = GetSelectedAssets();

#if ECB_LEGACY
	// Detect if at least one temporary item was selected. If there were no valid assets selected and a temporary one was, then deny the context menu.
	TArray<TSharedPtr<FExtAssetViewItem>> SelectedItems = GetSelectedItems();
	bool bAtLeastOneTemporaryItemFound = false;
	for ( auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt )
	{
		const TSharedPtr<FExtAssetViewItem>& Item = *ItemIt;
		if ( Item->IsTemporaryItem() )
		{
			bAtLeastOneTemporaryItemFound = true;
		}
	}

	// If there were no valid assets found, but some invalid assets were found, deny the context menu
	if ( SelectedAssets.Num() == 0 && bAtLeastOneTemporaryItemFound )
	{
		return false;
	}

	if ( SelectedAssets.Num() == 0 && SourcesData.HasCollections() )
	{
		// Don't allow a context menu when we're viewing a collection and have no assets selected
		return false;
	}

	// Build a list of selected object paths
	TArray<FString> ObjectPaths;
	for(auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		ObjectPaths.Add( AssetIt->ObjectPath.ToString() );
	}

	bool bLoadSuccessful = true;

	if ( bPreloadAssetsForContextMenu )
	{
		TArray<UObject*> LoadedObjects;
		const bool bAllowedToPrompt = false;
		bLoadSuccessful = ContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedObjects, bAllowedToPrompt);
	}

	// Do not show the context menu if the load failed
	return bLoadSuccessful;
#endif
	return SelectedAssets.Num() > 0;
}

void SExtAssetView::OnListMouseButtonDoubleClick(TSharedPtr<FExtAssetViewItem> AssetItem)
{
	if ( !ensure(AssetItem.IsValid()) )
	{
		return;
	}

	if ( AssetItem->GetType() == EExtAssetItemType::Folder )
	{
		OnPathSelected.ExecuteIfBound(StaticCastSharedPtr<FExtAssetViewFolder>(AssetItem)->FolderPath);
		return;
	}

	if ( AssetItem->IsTemporaryItem() )
	{
		// You may not activate temporary items, they are just for display.
		return;
	}
#if ECB_LEGACY
	TArray<FExtAssetData> ActivatedAssets;
	ActivatedAssets.Add(StaticCastSharedPtr<FExtAssetViewAsset>(AssetItem)->Data);
	OnAssetsActivated.ExecuteIfBound( ActivatedAssets, EAssetTypeActivationMethod::DoubleClicked );
#endif
}

FReply SExtAssetView::OnDraggingAssetItem( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (bAllowDragging)
	{
		TArray<FExtAssetData> DraggedAssets;
		TArray<FString> DraggedAssetPaths;

		//TArray<FString> DraggedAssetFilePaths;
		TArray<FAssetData> FakeDraggedAssets;

		// Work out which assets to drag
		{
			TArray<FExtAssetData> AssetDataList = GetSelectedAssets();
			for (const FExtAssetData& AssetData : AssetDataList)
			{
				// Skip invalid assets and redirectors
				if (AssetData.IsValid() && AssetData.AssetClass != UObjectRedirector::StaticClass()->GetFName())
				{
					//DraggedAssets.Add(AssetData.AssetData);
					if (!FExtContentBrowserSingleton::GetAssetRegistry().IsCachedDependencyInfoInValid(AssetData))
					{
						DraggedAssets.Add(AssetData);
						//DraggedAssetFilePaths.Add(AssetData.PackageFilePath.ToString());
						FakeDraggedAssets.Add(AssetData.AssetData);
					}
				}
			}
		}

#if ECB_DISABLE // no support of dragging a whole external folder around, yet
		// Work out which asset paths to drag
		{
			TArray<FString> SelectedFolders = GetSelectedFolders();
			if (SelectedFolders.Num() > 0 && !SourcesData.HasCollections())
			{
				DraggedAssetPaths = MoveTemp(SelectedFolders);
			}
		}
#endif

#if ECB_DISABLE
		// Use the custom drag handler?
		if (DraggedAssets.Num() > 0 && FEditorDelegates::OnAssetDragStarted.IsBound())
		{
			FEditorDelegates::OnAssetDragStarted.Broadcast(DraggedAssets, nullptr);
			return FReply::Handled();
		}
#endif
		
		// Use the standard drag handler?
		if ((DraggedAssets.Num() > 0 || DraggedAssetPaths.Num() > 0) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			if (MouseEvent.IsControlDown())
			{
				//return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(MoveTemp(FakeDraggedAssets), MoveTemp(DraggedAssetPaths)));
				return FReply::Handled().BeginDragDrop(FExtAssetDragDropOp::New(MoveTemp(DraggedAssets), MoveTemp(DraggedAssetPaths)));
			}
			else
			{
				return FReply::Handled().BeginDragDrop(FExtAssetDragDropOp::New(MoveTemp(DraggedAssets), MoveTemp(DraggedAssetPaths)));
			}
		}
	}

	return FReply::Unhandled();
}

bool SExtAssetView::ShouldAllowToolTips() const
{
	if (!IsShowingAssetTooltip())
	{
		return false;
	}

	bool bIsRightClickScrolling = false;
	switch( CurrentViewType )
	{
		case EAssetViewType::List:
			bIsRightClickScrolling = ListView->IsRightClickScrolling();
			break;

		case EAssetViewType::Tile:
			bIsRightClickScrolling = TileView->IsRightClickScrolling();
			break;

		case EAssetViewType::Column:
			bIsRightClickScrolling = ColumnView->IsRightClickScrolling();
			break;

		default:
			bIsRightClickScrolling = false;
			break;
	}

	return !bIsRightClickScrolling;
}

FText SExtAssetView::GetAssetCountText() const
{
	const int32 NumAssets = FilteredAssetItems.Num();
	const int32 NumSelectedAssets = GetSelectedItems().Num();

	FText AssetCount = FText::GetEmpty();
	if ( NumSelectedAssets == 0 )
	{
		if ( NumAssets == 1 )
		{
			AssetCount = LOCTEXT("AssetCountLabelSingular", "1 item");
		}
		else
		{
			AssetCount = FText::Format( LOCTEXT("AssetCountLabelPlural", "{0} items"), FText::AsNumber(NumAssets) );
		}
	}
	else
	{
		if ( NumAssets == 1 )
		{
			AssetCount = FText::Format( LOCTEXT("AssetCountLabelSingularPlusSelection", "1 item ({0} selected)"), FText::AsNumber(NumSelectedAssets) );
		}
		else
		{
			AssetCount = FText::Format( LOCTEXT("AssetCountLabelPluralPlusSelection", "{0} items ({1} selected)"), FText::AsNumber(NumAssets), FText::AsNumber(NumSelectedAssets) );
		}
	}

	return AssetCount;
}

EVisibility SExtAssetView::GetListViewVisibility() const
{
	return GetCurrentViewType() == EAssetViewType::List ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SExtAssetView::GetTileViewVisibility() const
{
	return GetCurrentViewType() == EAssetViewType::Tile ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SExtAssetView::GetColumnViewVisibility() const
{
	return GetCurrentViewType() == EAssetViewType::Column ? EVisibility::Visible : EVisibility::Collapsed;
}

#if ECB_LEGACY
void SExtAssetView::ToggleThumbnailEditMode()
{
	bThumbnailEditMode = !bThumbnailEditMode;
}
#endif
float SExtAssetView::GetThumbnailScale() const
{
	return ThumbnailScaleSliderValue.Get();
}

void SExtAssetView::SetThumbnailScale( float NewValue )
{
	ThumbnailScaleSliderValue = NewValue;
	RefreshList();
}

bool SExtAssetView::IsThumbnailScalingLocked() const
{
	return GetCurrentViewType() == EAssetViewType::Column;
}

float SExtAssetView::GetListViewItemHeight() const
{
	return (ListViewThumbnailSize + ListViewThumbnailPadding * 2) * FMath::Lerp(MinThumbnailScale, MaxThumbnailScale, GetThumbnailScale());
}

float SExtAssetView::GetTileViewItemHeight() const
{
	return TileViewNameHeight + GetTileViewItemBaseHeight() * FillScale;
}

float SExtAssetView::GetTileViewItemBaseHeight() const
{
	return (TileViewThumbnailSize + TileViewThumbnailPadding * 2) * FMath::Lerp(MinThumbnailScale, MaxThumbnailScale, GetThumbnailScale());
}

float SExtAssetView::GetTileViewItemWidth() const
{
	return GetTileViewItemBaseWidth() * FillScale;
}

float SExtAssetView::GetTileViewItemBaseWidth() const //-V524
{
	return ( TileViewThumbnailSize + TileViewThumbnailPadding * 2 ) * FMath::Lerp( MinThumbnailScale, MaxThumbnailScale, GetThumbnailScale() );
}

EColumnSortMode::Type SExtAssetView::GetColumnSortMode(const FName ColumnId) const
{
	for (int32 PriorityIdx = 0; PriorityIdx < EColumnSortPriority::Max; PriorityIdx++)
	{
		const EColumnSortPriority::Type SortPriority = static_cast<EColumnSortPriority::Type>(PriorityIdx);
		if (ColumnId == SortManager.GetSortColumnId(SortPriority))
		{
			return SortManager.GetSortMode(SortPriority);
		}
	}
	return EColumnSortMode::None;
}

EColumnSortPriority::Type SExtAssetView::GetColumnSortPriority(const FName ColumnId) const
{
	for (int32 PriorityIdx = 0; PriorityIdx < EColumnSortPriority::Max; PriorityIdx++)
	{
		const EColumnSortPriority::Type SortPriority = static_cast<EColumnSortPriority::Type>(PriorityIdx);
		if (ColumnId == SortManager.GetSortColumnId(SortPriority))
		{
			return SortPriority;
		}
	}
	return EColumnSortPriority::Primary;
}

void SExtAssetView::OnSortColumnHeader(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type NewSortMode)
{
	SortManager.SetSortColumnId(SortPriority, ColumnId);
	SortManager.SetSortMode(SortPriority, NewSortMode);
	SortList();
}

EVisibility SExtAssetView::IsAssetShowWarningTextVisible() const
{
	return (FilteredAssetItems.Num() > 0 || bQuickFrontendListRefreshRequested) ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
}

FText SExtAssetView::GetAssetShowWarningText() const
{
	if (AssetShowWarningText.IsSet())
	{
		return AssetShowWarningText.Get();
	}
	
	FText NothingToShowText, DropText;
	if (ShouldFilterRecursively())
	{
		NothingToShowText = LOCTEXT( "NothingToShowCheckFilter", "No results, check your filter." );
	}
#if ECB_WIP_COLLECTION
	if ( SourcesData.HasCollections() && !SourcesData.IsDynamicCollection() )
	{
		DropText = LOCTEXT( "DragAssetsHere", "Drag and drop assets here to add them to the collection." );
	}
	else if ( OnGetAssetContextMenu.IsBound() )
	{
		DropText = LOCTEXT( "DropFilesOrRightClick", "Drop files here or right click to create content." );
	}
#endif	
	return NothingToShowText.IsEmpty() ? DropText : FText::Format(LOCTEXT("NothingToShowPattern", "{0}\n\n{1}"), NothingToShowText, DropText);
}

#if ECB_FEA_ASSET_DRAG_DROP
bool SExtAssetView::OnDropAndDropToAssetView(const FAssetViewDragAndDropExtender::FPayload& InPayLoad)
{
	ECB_LOG(Display, TEXT("FAssetViewDragAndDropExtenderHelper::OnDrop"));

	TSharedPtr<FDragDropOperation> DragDropOp = InPayLoad.DragDropOp;

	if (DragDropOp->IsOfType<FExtAssetDragDropOp>() && InPayLoad.PackagePaths.Num() > 0)
	{
		TSharedPtr<FExtAssetDragDropOp> AssetDragDropOp = StaticCastSharedPtr<FExtAssetDragDropOp>(DragDropOp);

		const FString DestPath = InPayLoad.PackagePaths[0].ToString();
		OnAssetsOrPathsDragDroppedToContentBrowser(AssetDragDropOp->GetAssets(), AssetDragDropOp->GetAssetPaths(), DestPath);
		return true;
	}

	return false;
}

TSharedRef<FExtender> SExtAssetView::OnExtendLevelViewportMenu(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> InActors)
{
	TSharedRef<FExtender> Extender(new FExtender());

	int32 NumSelected = GetNumSelectedAssets();
	if (NumSelected > 0)
	{
		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedRef<FUICommandList> LevelEditorCommandBindings = LevelEditor.GetGlobalLevelEditorActions();

		Extender->AddMenuExtension("ActorControl", EExtensionHook::After, LevelEditorCommandBindings, FMenuExtensionDelegate::CreateLambda(
			[this, NumSelected](FMenuBuilder& MenuBuilder) {
			MenuBuilder.BeginSection("UAssetBrowser", LOCTEXT("UAssetBrowser", "UAsset Browser"));
			//MenuBuilder.AddMenuEntry(FExtContentBrowserCommands::Get().ImportSelectedUAsset, NAME_None, FText::Format(LOCTEXT("ImportSelectedUAsset", "Import {0} UAsset Files"), FText::AsNumber(NumSelected)));
			MenuBuilder.AddMenuEntry(
				FText::Format(LOCTEXT("ImportAndPlaceSelectedUAsset", "Import and place {0} asset(s) here"), FText::AsNumber(NumSelected))
				, LOCTEXT("ImportAndPlaceSelectedUAssetTooltip", "Import selected UAsset Files to Content Browser and place them to current level."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SExtAssetView::ExecuteImportAndPlaceSelecteUAssetFiles),
					FCanExecuteAction::CreateLambda([this]() {return GetNumSelectedAssets() > 0; })
				)
			);

			MenuBuilder.EndSection();
		}
		));
	}
	
	return Extender;
}

void SExtAssetView::OnAssetsOrPathsDragDroppedToContentBrowser(const TArray<FExtAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& DestinationPath)
{
	DragDropHandler::HandleDropOnAssetFolderOfContentBrowser(
		SharedThis(this),
		AssetList,
		AssetPaths,
		DestinationPath,
		FText::FromString(FPaths::GetCleanFilename(DestinationPath)),
		DragDropHandler::FExecuteDropToImport::CreateSP(this, &SExtAssetView::ExecuteDropToContentBrowserToImport),
		DragDropHandler::FExecuteDropToImport::CreateSP(this, &SExtAssetView::ExecuteDropToContentBrowserToImportFlat)
	);
}

void SExtAssetView::ExecuteImportAndPlaceSelecteUAssetFiles()
{
	TArray<FExtAssetData> SelectedAssets = GetSelectedAssets();
	if (SelectedAssets.Num() > 0)
	{
		FUAssetImportSetting ImportSetting = FUAssetImportSetting::GetSavedImportSetting();
		ImportSetting.bPlaceImportedAssets = true;
		FExtAssetImporter::ImportAssets(SelectedAssets, ImportSetting);
	}
}

void SExtAssetView::ExecuteDropToContentBrowserToImport(TArray<FExtAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
	FExtAssetImporter::ImportAssets(AssetList, FUAssetImportSetting::GetSavedImportSetting());
#if ECB_LEGACY
	int32 NumItemsCopied = 0;

	if (AssetList.Num() > 0)
	{
		TArray<UObject*> DroppedObjects;
		ContentBrowserUtils::GetObjectsInAssetData(AssetList, DroppedObjects);

		TArray<UObject*> NewObjects;
		ObjectTools::DuplicateObjects(DroppedObjects, TEXT(""), DestinationPath, /*bOpenDialog=*/false, &NewObjects);

		NumItemsCopied += NewObjects.Num();
	}

	if (AssetPaths.Num() > 0)
	{
		if (ContentBrowserUtils::CopyFolders(AssetPaths, DestinationPath))
		{
			NumItemsCopied += AssetPaths.Num();
		}
	}

	// If any items were duplicated, report the success
	if (NumItemsCopied > 0)
	{
		const FText Message = FText::Format(LOCTEXT("AssetItemsDroppedCopy", "{0} {0}|plural(one=item,other=items) copied"), NumItemsCopied);
		const FVector2D& CursorPos = FSlateApplication::Get().GetCursorPos();
		FSlateRect MessageAnchor(CursorPos.X, CursorPos.Y, CursorPos.X, CursorPos.Y);
		ContentBrowserUtils::DisplayMessage(Message, MessageAnchor, SharedThis(this));
	}
#endif
}

void SExtAssetView::ExecuteDropToContentBrowserToImportFlat(TArray<FExtAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
	FExtAssetImporter::ImportAssetsToFolderPackagePath(AssetList, FUAssetImportSetting::GetFlatModeImportSetting(), DestinationPath);
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

	if (AssetPaths.Num() > 0)
	{
		ContentBrowserUtils::MoveFolders(AssetPaths, DestinationPath);
	}

	OnFolderPathChanged.ExecuteIfBound(MovedFolders);
#endif
}

#endif

#if ECB_LEGACY
bool SExtAssetView::HasSingleCollectionSource() const
{
	return ( SourcesData.Collections.Num() == 1 && SourcesData.PackagePaths.Num() == 0 );
}

void SExtAssetView::OnAssetsOrPathsDragDropped(const TArray<FAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& DestinationPath)
{
	DragDropHandler::HandleDropOnAssetFolder(
		SharedThis(this), 
		AssetList, 
		AssetPaths, 
		DestinationPath, 
		FText::FromString(FPaths::GetCleanFilename(DestinationPath)), 
		DragDropHandler::FExecuteCopyOrMove::CreateSP(this, &SExtAssetView::ExecuteDropCopy),
		DragDropHandler::FExecuteCopyOrMove::CreateSP(this, &SExtAssetView::ExecuteDropMove),
		DragDropHandler::FExecuteCopyOrMove::CreateSP(this, &SExtAssetView::ExecuteDropAdvancedCopy)
		);
}

void SExtAssetView::OnFilesDragDropped(const TArray<FString>& AssetList, const FString& DestinationPath)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().ImportAssets( AssetList, DestinationPath );
}

void SExtAssetView::ExecuteDropCopy(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
	int32 NumItemsCopied = 0;

	if (AssetList.Num() > 0)
	{
		TArray<UObject*> DroppedObjects;
		ContentBrowserUtils::GetObjectsInAssetData(AssetList, DroppedObjects);

		TArray<UObject*> NewObjects;
		ObjectTools::DuplicateObjects(DroppedObjects, TEXT(""), DestinationPath, /*bOpenDialog=*/false, &NewObjects);

		NumItemsCopied += NewObjects.Num();
	}

	if (AssetPaths.Num() > 0)
	{
		if (ContentBrowserUtils::CopyFolders(AssetPaths, DestinationPath))
		{
			NumItemsCopied += AssetPaths.Num();
		}
	}

	// If any items were duplicated, report the success
	if (NumItemsCopied > 0)
	{
		const FText Message = FText::Format(LOCTEXT("AssetItemsDroppedCopy", "{0} {0}|plural(one=item,other=items) copied"), NumItemsCopied);
		const FVector2D& CursorPos = FSlateApplication::Get().GetCursorPos();
		FSlateRect MessageAnchor(CursorPos.X, CursorPos.Y, CursorPos.X, CursorPos.Y);
		ContentBrowserUtils::DisplayMessage(Message, MessageAnchor, SharedThis(this));
	}
}

void SExtAssetView::ExecuteDropMove(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
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

	if (AssetPaths.Num() > 0)
	{
		ContentBrowserUtils::MoveFolders(AssetPaths, DestinationPath);
	}

	OnFolderPathChanged.ExecuteIfBound(MovedFolders);
}


void SExtAssetView::ExecuteDropAdvancedCopy(TArray<FAssetData> AssetList, TArray<FString> AssetPaths, FString DestinationPath)
{
	ContentBrowserUtils::BeginAdvancedCopyPackages(AssetList, AssetPaths, DestinationPath);
}
#endif

#if ECB_FEA_SEARCH
void SExtAssetView::SetUserSearching(bool bInSearching)
{
	if(bUserSearching != bInSearching)
	{
		RequestSlowFullListRefresh();
	}
	bUserSearching = bInSearching;
}
#endif

void SExtAssetView::HandleSettingChanged(FName PropertyName)
{
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UExtContentBrowserSettings, DisplayFolders)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UExtContentBrowserSettings, DisplayEmptyFolders)) ||
		(PropertyName == "DisplayDevelopersFolder") ||
		(PropertyName == "DisplayEngineFolder") ||
		(PropertyName == NAME_None))	// @todo: Needed if PostEditChange was called manually, for now
	{
		RequestSlowFullListRefresh();
	}
}

void SExtAssetView::OnAssetRegistryAssetGathered(const FExtAssetData& AssetData, int32 Left)
{
	RecentlyAddedAssets.Add(AssetData);
}

void SExtAssetView::OnAssetRegistryRootPathAdded(const FString& Path)
{

}

void SExtAssetView::OnAssetRegistryRootPathRemoved(const FString& Path)
{
	// Clear all
	SetSourcesData(FSourcesData());
}

#if ECB_WIP_BREADCRUMB
FText SExtAssetView::GetQuickJumpTerm() const
{
	return FText::FromString(QuickJumpData.JumpTerm);
}

EVisibility SExtAssetView::IsQuickJumpVisible() const
{
	return (QuickJumpData.JumpTerm.IsEmpty()) ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
}

FSlateColor SExtAssetView::GetQuickJumpColor() const
{
	return FAppStyle::GetColor((QuickJumpData.bHasValidMatch) ? "InfoReporting.BackgroundColor" : "ErrorReporting.BackgroundColor");
}

void SExtAssetView::ResetQuickJump()
{
	QuickJumpData.JumpTerm.Empty();
	QuickJumpData.bIsJumping = false;
	QuickJumpData.bHasChangedSinceLastTick = false;
	QuickJumpData.bHasValidMatch = false;
}

FReply SExtAssetView::HandleQuickJumpKeyDown(const TCHAR InCharacter, const bool bIsControlDown, const bool bIsAltDown, const bool bTestOnly)
{
	// Check for special characters
	if(bIsControlDown || bIsAltDown)
	{
		return FReply::Unhandled();
	}

	// Check for invalid characters
	for(int InvalidCharIndex = 0; InvalidCharIndex < UE_ARRAY_COUNT(INVALID_OBJECTNAME_CHARACTERS) - 1; ++InvalidCharIndex)
	{
		if(InCharacter == INVALID_OBJECTNAME_CHARACTERS[InvalidCharIndex])
		{
			return FReply::Unhandled();
		}
	}

	switch(InCharacter)
	{
	// Ignore some other special characters that we don't want to be entered into the buffer
	case 0:		// Any non-character key press, e.g. f1-f12, Delete, Pause/Break, etc.
				// These should be explicitly not handled so that their input bindings are handled higher up the chain.

	case 8:		// Backspace
	case 13:	// Enter
	case 27:	// Esc
		return FReply::Unhandled();

	default:
		break;
	}

	// Any other character!
	if(!bTestOnly)
	{
		QuickJumpData.JumpTerm.AppendChar(InCharacter);
		QuickJumpData.bHasChangedSinceLastTick = true;
	}

	return FReply::Handled();
}

bool SExtAssetView::PerformQuickJump(const bool bWasJumping)
{
	auto GetAssetViewItemName = [](const TSharedPtr<FExtAssetViewItem> &Item) -> FString
	{
		switch(Item->GetType())
		{
		case EAssetItemType::Normal:
			{
				const TSharedPtr<FExtAssetViewAsset>& ItemAsAsset = StaticCastSharedPtr<FExtAssetViewAsset>(Item);
				return ItemAsAsset->Data.AssetName.ToString();
			}

		case EAssetItemType::Folder:
			{
				const TSharedPtr<FAssetViewFolder>& ItemAsFolder = StaticCastSharedPtr<FAssetViewFolder>(Item);
				return ItemAsFolder->FolderName.ToString();
			}

		default:
			return FString();
		}
	};

	auto JumpToNextMatch = [this, &GetAssetViewItemName](const int StartIndex, const int EndIndex) -> bool
	{
		check(StartIndex >= 0);
		check(EndIndex <= FilteredAssetItems.Num());

		for(int NewSelectedItemIndex = StartIndex; NewSelectedItemIndex < EndIndex; ++NewSelectedItemIndex)
		{
			TSharedPtr<FExtAssetViewItem>& NewSelectedItem = FilteredAssetItems[NewSelectedItemIndex];
			const FString NewSelectedItemName = GetAssetViewItemName(NewSelectedItem);
			if(NewSelectedItemName.StartsWith(QuickJumpData.JumpTerm, ESearchCase::IgnoreCase))
			{
				SetSelection(NewSelectedItem);
				RequestScrollIntoView(NewSelectedItem);
				return true;
			}
		}

		return false;
	};

	TArray<TSharedPtr<FExtAssetViewItem>> SelectedItems = GetSelectedItems();
	TSharedPtr<FExtAssetViewItem> SelectedItem = (SelectedItems.Num()) ? SelectedItems[0] : nullptr;

	// If we have a selection, and we were already jumping, first check to see whether 
	// the current selection still matches the quick-jump term; if it does, we do nothing
	if(bWasJumping && SelectedItem.IsValid())
	{
		const FString SelectedItemName = GetAssetViewItemName(SelectedItem);
		if(SelectedItemName.StartsWith(QuickJumpData.JumpTerm, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	// We need to move on to the next match in FilteredAssetItems that starts with the given quick-jump term
	const int SelectedItemIndex = (SelectedItem.IsValid()) ? FilteredAssetItems.Find(SelectedItem) : INDEX_NONE;
	const int StartIndex = (SelectedItemIndex == INDEX_NONE) ? 0 : SelectedItemIndex + 1;
	
	bool ValidMatch = JumpToNextMatch(StartIndex, FilteredAssetItems.Num());
	if(!ValidMatch && StartIndex > 0)
	{
		// If we didn't find a match, we need to loop around and look again from the start (assuming we weren't already)
		return JumpToNextMatch(0, StartIndex);
	}

	return ValidMatch;
}
#endif

void SExtAssetView::ToggleMajorAssetTypeColumns()
{
	bShowMajorAssetTypeColumnsInColumnView = !bShowMajorAssetTypeColumnsInColumnView;
	ColumnView->GetHeaderRow()->RefreshColumns();
	ColumnView->RebuildList();
}

bool SExtAssetView::IsShowingMajorAssetTypeColumns() const
{
	return bShowMajorAssetTypeColumnsInColumnView;
}

void SExtAssetView::FillToggleColumnsMenu(FMenuBuilder& MenuBuilder)
{
	// Column view may not be valid if we toggled off columns view while the columns menu was open
	if(ColumnView.IsValid())
	{
		const TIndirectArray<SHeaderRow::FColumn> Columns = ColumnView->GetHeaderRow()->GetColumns();

		for (int32 ColumnIndex = 0; ColumnIndex < Columns.Num(); ++ColumnIndex)
		{
			const FString ColumnName = Columns[ColumnIndex].ColumnId.ToString();

			MenuBuilder.AddMenuEntry(
				Columns[ColumnIndex].DefaultText,
				LOCTEXT("ShowHideColumnTooltip", "Show or hide column"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SExtAssetView::ToggleColumn, ColumnName),
					FCanExecuteAction::CreateSP(this, &SExtAssetView::CanToggleColumn, ColumnName),
					FIsActionChecked::CreateSP(this, &SExtAssetView::IsColumnVisible, ColumnName),
					EUIActionRepeatMode::RepeatEnabled
				),
				NAME_None,
				EUserInterfaceActionType::Check
			);
		}
	}
}

void SExtAssetView::ResetColumns()
{
	HiddenColumnNames.Empty();
	NumVisibleColumns = ColumnView->GetHeaderRow()->GetColumns().Num();
	ColumnView->GetHeaderRow()->RefreshColumns();
	ColumnView->RebuildList();
}

void SExtAssetView::ExportColumns()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	const FText Title = LOCTEXT("ExportToCSV", "Export columns as CSV...");
	const FString FileTypes = TEXT("Data Table CSV (*.csv)|*.csv");

	TArray<FString> OutFilenames;
	DesktopPlatform->SaveFileDialog(
		ParentWindowWindowHandle,
		Title.ToString(),
		TEXT(""),
		TEXT("Report.csv"),
		FileTypes,
		EFileDialogFlags::None,
		OutFilenames
	);

	if (OutFilenames.Num() > 0)
	{
		const TIndirectArray<SHeaderRow::FColumn>& Columns = ColumnView->GetHeaderRow()->GetColumns();

		TArray<FName> ColumnNames;
		for (const SHeaderRow::FColumn& Column : Columns)
		{
			ColumnNames.Add(Column.ColumnId);
		}

		FString SaveString;
		SortManager.ExportColumnsToCSV(FilteredAssetItems, ColumnNames, CustomColumns, SaveString);

		FFileHelper::SaveStringToFile(SaveString, *OutFilenames[0]);
	}
}

void SExtAssetView::ToggleColumn(const FString ColumnName)
{
	SetColumnVisibility(ColumnName, HiddenColumnNames.Contains(ColumnName));
}

void SExtAssetView::SetColumnVisibility(const FString ColumnName, const bool bShow)
{
	if (!bShow)
	{
		--NumVisibleColumns;
		HiddenColumnNames.Add(ColumnName);
	}
	else
	{
		++NumVisibleColumns;
		check(HiddenColumnNames.Contains(ColumnName));
		HiddenColumnNames.Remove(ColumnName);
	}

	ColumnView->GetHeaderRow()->RefreshColumns();
	ColumnView->RebuildList();
}

bool SExtAssetView::CanToggleColumn(const FString ColumnName) const
{
	return (HiddenColumnNames.Contains(ColumnName) || NumVisibleColumns > 1);
}

bool SExtAssetView::IsColumnVisible(const FString ColumnName) const
{
	const FName ColumnNameToCheck(*ColumnName);
	const bool bAssetTypeSpecificColumns = ColumnNameToCheck != SortManager.NameColumnId
		&& ColumnNameToCheck != SortManager.ClassColumnId
		&& ColumnNameToCheck != SortManager.PathColumnId;

	const bool bHideMajorAssetTypeColumns = !bShowMajorAssetTypeColumnsInColumnView && bAssetTypeSpecificColumns;

	return !bHideMajorAssetTypeColumns && !HiddenColumnNames.Contains(ColumnName);
}

bool SExtAssetView::ShouldColumnGenerateWidget(const FString ColumnName) const
{
	return IsColumnVisible(ColumnName);// !HiddenColumnNames.Contains(ColumnName);
}

TSharedRef<SWidget> SExtAssetView::CreateRowHeaderMenuContent(const FString ColumnName)
{
	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HideColumn", "Hide Column"),
		LOCTEXT("HideColumnToolTip", "Hides this column."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SExtAssetView::SetColumnVisibility, ColumnName, false), FCanExecuteAction::CreateSP(this, &SExtAssetView::CanToggleColumn, ColumnName)),
		NAME_None,
		EUserInterfaceActionType::Button);

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

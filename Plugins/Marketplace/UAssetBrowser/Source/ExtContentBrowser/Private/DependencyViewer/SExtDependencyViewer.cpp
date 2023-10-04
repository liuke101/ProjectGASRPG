// Copyright 2017-2021 marynate. All Rights Reserved.

#include "SExtDependencyViewer.h"
#include "EdGraph_ExtDependencyViewer.h"
#include "EdGraphNode_ExtDependency.h"
#include "ExtDependencyViewerSchema.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtAssetData.h"
#include "ExtContentBrowserSettings.h"
#include "ExtDependencyViewerCommands.h"

#include "Widgets/SOverlay.h"
#include "Engine/GameViewportClient.h"
#include "Dialogs/Dialogs.h"
#include "Misc/MessageDialog.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSlider.h"
#include "EditorStyleSet.h"
#include "Engine/Selection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"
#include "Editor.h"
#include "EditorWidgetsModule.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "Engine/AssetManager.h"
#include "Widgets/Input/SComboBox.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ObjectTools.h"

#define LOCTEXT_NAMESPACE "ExtDependencyViewer"

SExtDependencyViewer::~SExtDependencyViewer()
{
	if (!GExitPurge)
	{
		if ( ensure(GraphObj) )
		{
			GraphObj->RemoveFromRoot();
		}		
	}
}

void SExtDependencyViewer::Construct(const FArguments& InArgs)
{
	// Create an action list and register commands
	RegisterActions();

	// Set up the history manager
	HistoryManager.SetOnApplyHistoryData(FOnApplyExtRefHistoryData::CreateSP(this, &SExtDependencyViewer::OnApplyHistoryData));
	HistoryManager.SetOnUpdateHistoryData(FOnUpdateExtRefHistoryData::CreateSP(this, &SExtDependencyViewer::OnUpdateHistoryData));

	// Fill out CollectionsComboList
	{
		TArray<FName> CollectionNames;
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
		CollectionManagerModule.Get().GetCollectionNames(ECollectionShareType::CST_All, CollectionNames);
		CollectionNames.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });
		CollectionsComboList.Add(MakeShareable(new FName(NAME_None)));
		for (FName CollectionName : CollectionNames)
		{
			CollectionsComboList.Add(MakeShareable(new FName(CollectionName)));
		}
	}

	// Create the graph
	GraphObj = NewObject<UEdGraph_ExtDependencyViewer>();
	GraphObj->Schema = UExtDependencyViewerSchema::StaticClass();
	GraphObj->AddToRoot();
	GraphObj->SetReferenceViewer(StaticCastSharedRef<SExtDependencyViewer>(AsShared()));

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &SExtDependencyViewer::OnNodeDoubleClicked);
	GraphEvents.OnCreateActionMenu = SGraphEditor::FOnCreateActionMenu::CreateSP(this, &SExtDependencyViewer::OnCreateGraphActionMenu);

	// Create the graph editor
	GraphEditorPtr = SNew(SGraphEditor)
		.AdditionalCommands(ReferenceViewerActions)
		.GraphToEdit(GraphObj)
		.GraphEvents(GraphEvents)
		.OnNavigateHistoryBack(FSimpleDelegate::CreateSP(this, &SExtDependencyViewer::GraphNavigateHistoryBack))
		.OnNavigateHistoryForward(FSimpleDelegate::CreateSP(this, &SExtDependencyViewer::GraphNavigateHistoryForward));

	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_None, FMargin(16, 8), false);

	static const FName DefaultForegroundName("DefaultForeground");

	ChildSlot
	[
		SNew(SVerticalBox)

#if ECB_TODO
		// Title Bar>>
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2, 0, 1)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("Graph.TitleBackground")))
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DepencencyViewerGraphLabel", "Dependency Viewer"))
					.TextStyle(FAppStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
				]
			]
		]
		// Title Bar <<
#endif

#if ECB_WIP_REF_VIEWER_HISTORY
		// Path and history
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0, 0, 0, 4 )
		[
			SNew(SHorizontalBox)

			// History Back Button
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(1,0)
			[
				SNew(SButton)
				.ButtonStyle( FAppStyle::Get(), "HoverHintOnly" )
				.ForegroundColor( FAppStyle::GetSlateColor(DefaultForegroundName) )
				.ToolTipText( this, &SExtDependencyViewer::GetHistoryBackTooltip )
				.ContentPadding( 0 )
				.OnClicked(this, &SExtDependencyViewer::BackClicked)
				.IsEnabled(this, &SExtDependencyViewer::IsBackEnabled)
				[
					SNew(SImage) .Image(FAppStyle::GetBrush("ContentBrowser.HistoryBack"))
				]
			]

			// History Forward Button
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1,0,3,0)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FAppStyle::Get(), "HoverHintOnly" )
				.ForegroundColor( FAppStyle::GetSlateColor(DefaultForegroundName) )
				.ToolTipText( this, &SExtDependencyViewer::GetHistoryForwardTooltip )
				.ContentPadding( 0 )
				.OnClicked(this, &SExtDependencyViewer::ForwardClicked)
				.IsEnabled(this, &SExtDependencyViewer::IsForwardEnabled)
				[
					SNew(SImage) .Image(FAppStyle::GetBrush("ContentBrowser.HistoryForward"))
				]
			]

			// Path
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Fill)
			.FillWidth(1.f)
			[
				SNew(SBorder)
				.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
				[
					SNew(SEditableTextBox)
					.Text(this, &SExtDependencyViewer::GetAddressBarText)
					.OnTextCommitted(this, &SExtDependencyViewer::OnAddressBarTextCommitted)
					.OnTextChanged(this, &SExtDependencyViewer::OnAddressBarTextChanged)
					.SelectAllTextWhenFocused(true)
					.SelectAllTextOnCommit(true)
					.Style(FAppStyle::Get(), "ReferenceViewer.PathText")
				]
			]
		]
#endif

		// Graph
#if ECB_FOLD
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SOverlay)

			+SOverlay::Slot()
			[
				GraphEditorPtr.ToSharedRef()
			]

			// Float Panel >>
#if ECB_TODO
			+SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(8)
			[
				SNew(SBorder)
				.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(2.f)
					.AutoHeight()
					[
						SAssignNew(SearchBox, SSearchBox)
						.HintText(LOCTEXT("Search", "Search..."))
						.ToolTipText(LOCTEXT("SearchTooltip", "Type here to search (pressing Enter zooms to the results)"))
						.OnTextChanged(this, &SExtDependencyViewer::HandleOnSearchTextChanged)
						.OnTextCommitted(this, &SExtDependencyViewer::HandleOnSearchTextCommitted)
					]
#if ECB_LEGACY
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SearchDepthLabelText", "Search Depth Limit"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged( this, &SExtDependencyViewer::OnSearchDepthEnabledChanged )
							.IsChecked( this, &SExtDependencyViewer::IsSearchDepthEnabledChecked )
						]
					
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SBox)
							.WidthOverride(100)
							[
								SNew(SSpinBox<int32>)
								.Value(this, &SExtDependencyViewer::GetSearchDepthCount)
								.OnValueChanged(this, &SExtDependencyViewer::OnSearchDepthCommitted)
								.MinValue(1)
								.MaxValue(50)
								.MaxSliderValue(12)
							]
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SearchBreadthLabelText", "Search Breadth Limit"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged( this, &SExtDependencyViewer::OnSearchBreadthEnabledChanged )
							.IsChecked( this, &SExtDependencyViewer::IsSearchBreadthEnabledChecked )
						]
					
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SBox)
							.WidthOverride(100)
							[
								SNew(SSpinBox<int32>)
								.Value(this, &SExtDependencyViewer::GetSearchBreadthCount)
								.OnValueChanged(this, &SExtDependencyViewer::OnSearchBreadthCommitted)
								.MinValue(1)
								.MaxValue(1000)
								.MaxSliderValue(50)
							]
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CollectionFilter", "Collection Filter"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged( this, &SExtDependencyViewer::OnEnableCollectionFilterChanged )
							.IsChecked( this, &SExtDependencyViewer::IsEnableCollectionFilterChecked )
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SBox)
							.WidthOverride(100)
							[
								SNew(SComboBox<TSharedPtr<FName>>)
								.OptionsSource(&CollectionsComboList)
								.OnGenerateWidget(this, &SExtDependencyViewer::GenerateCollectionFilterItem)
								.OnSelectionChanged(this, &SExtDependencyViewer::HandleCollectionFilterChanged)
								.ToolTipText(this, &SExtDependencyViewer::GetCollectionFilterText)
								[
									SNew(STextBlock)
									.Text(this, &SExtDependencyViewer::GetCollectionFilterText)
								]
							]
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ShowHideSoftReferences", "Show Soft References"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged( this, &SExtDependencyViewer::OnShowSoftReferencesChanged )
							.IsChecked( this, &SExtDependencyViewer::IsShowSoftReferencesChecked )
						]
					]

				
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ShowHideHardReferences", "Show Hard References"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged(this, &SExtDependencyViewer::OnShowHardReferencesChanged)
							.IsChecked(this, &SExtDependencyViewer::IsShowHardReferencesChecked)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SExtDependencyViewer::GetManagementReferencesVisibility)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ShowHideManagementReferences", "Show Management References"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged(this, &SExtDependencyViewer::OnShowManagementReferencesChanged)
							.IsChecked(this, &SExtDependencyViewer::IsShowManagementReferencesChecked)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ShowHideSearchableNames", "Show Searchable Names"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged(this, &SExtDependencyViewer::OnShowSearchableNamesChanged)
							.IsChecked(this, &SExtDependencyViewer::IsShowSearchableNamesChecked)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ShowHideNativePackages", "Show Native Packages"))
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2.f)
						[
							SNew(SCheckBox)
							.OnCheckStateChanged(this, &SExtDependencyViewer::OnShowNativePackagesChanged)
							.IsChecked(this, &SExtDependencyViewer::IsShowNativePackagesChecked)
						]
					]
#endif
				]
			]
#endif		// Float Panel <<

			+SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(24, 0, 24, 0))
			[
				AssetDiscoveryIndicator
			]
		]
#endif

		// Bottom panel
#if ECB_FEA_REF_VIEWER_OPTIONS
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)

#if ECB_FEA_REF_VIEWER_NODE_SPACING
				// Node Spacing
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SAssignNew(NodeSpacingSliderContainer, SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("NodeSpacing", "Node Spacing"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(120.0f)
						[
							SNew(SSlider)
							.ToolTipText(LOCTEXT("NodeSpacingToolTip", "Adjust the x spacing between nodes"))
							.Value(this, &SExtDependencyViewer::GetNodeSpacingScale)
							.OnValueChanged(this, &SExtDependencyViewer::SetNodeSpacingScale)
							.Locked(this, &SExtDependencyViewer::IsNodeSpacingScaleLocked)
						]
					]
				]
#endif

				// View options
				+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
				[
					SAssignNew(ViewOptionsComboButton, SComboButton)
					.ContentPadding(0)
					.ForegroundColor(this, &SExtDependencyViewer::GetViewButtonForegroundColor)
					.ButtonStyle(FAppStyle::Get(), "ToggleButton")
					.OnGetMenuContent(this, &SExtDependencyViewer::GetViewButtonContent)
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
							SNew(STextBlock).Text(LOCTEXT("OptionsButton", "Options"))
						]
					]
				]
			]
		]
#endif
	];
}

void SExtDependencyViewer::SetGraphRootIdentifiers(const TArray<FExtAssetIdentifier>& NewGraphRootIdentifiers)
{
	GraphObj->SetGraphRoot(NewGraphRootIdentifiers);
	
	RebuildGraph();

	// Zoom once this frame to make sure widgets are visible, then zoom again so size is correct
	TriggerZoomToFit(0, 0);
	RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SExtDependencyViewer::TriggerZoomToFit));

	// Set the initial history data
	HistoryManager.AddHistoryData();
}

EActiveTimerReturnType SExtDependencyViewer::TriggerZoomToFit(double InCurrentTime, float InDeltaTime)
{
	if (GraphEditorPtr.IsValid())
	{
		GraphEditorPtr->ZoomToFit(false);
	}
	return EActiveTimerReturnType::Stop;
}

void SExtDependencyViewer::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	GConfig->SetFloat(*IniSection, TEXT(".DependencyViewerNodeXSpacingScale"), NodeSpacingScale, IniFilename);
}

void SExtDependencyViewer::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	float Scale = 0.f;
	if (GConfig->GetFloat(*IniSection, TEXT(".DependencyViewerNodeXSpacingScale"), Scale, IniFilename))
	{
		SetNodeSpacingScale(Scale);
	}
}

float SExtDependencyViewer::GetNodeSpacingScale() const
{
	return NodeSpacingScale;
}

void SExtDependencyViewer::SetNodeSpacingScale(float NewValue)
{
	NodeSpacingScale = FMath::Clamp<float>(NewValue, 0.f, 1.f);;

	const float Min = 200.f;
	const float Max = 1200.f;
	float NewNodeXSpacing = Min + (Max - Min) * NodeSpacingScale;

	if (GraphObj)
	{
		GraphObj->SetNodeXSpacing(NewNodeXSpacing);
	}
	RebuildGraph();
}

bool SExtDependencyViewer::IsNodeSpacingScaleLocked() const
{
	return GraphObj == NULL;
}

FSlateColor SExtDependencyViewer::GetViewButtonForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	return ViewOptionsComboButton->IsHovered() ? FAppStyle::GetSlateColor(InvertedForegroundName) : FAppStyle::GetSlateColor(DefaultForegroundName);
}

TSharedRef<SWidget> SExtDependencyViewer::GetViewButtonContent()
{
	UExtContentBrowserSettings* ExtContentBrowserSetting = GetMutableDefault<UExtContentBrowserSettings>();

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/false, NULL, TSharedPtr<FExtender>(), /*bCloseSelfOnly=*/ true);

	MenuBuilder.BeginSection("Show", LOCTEXT("ShowHeading", "Show"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowDirectDependenciesOnly", "Direct Dependencies Only"),
			LOCTEXT("ShowDirectDependenciesOnlyTooltip", "Only showing direct dependencies, un-check to show dependencies recursively"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { OnSearchDepthEnabledChanged(IsSearchDepthEnabledChecked() == ECheckBoxState::Checked ? ECheckBoxState::Unchecked : ECheckBoxState::Checked); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this] { return IsSearchDepthEnabledChecked() == ECheckBoxState::Checked; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowHardReferences", "Hard References"),
			LOCTEXT("ShowHardReferencesTooltip", "Show hard referenced dependencies"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { OnShowHardReferencesChanged(IsShowHardReferencesChecked() == ECheckBoxState::Checked ? ECheckBoxState::Unchecked : ECheckBoxState::Checked); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this] { return IsShowHardReferencesChecked() == ECheckBoxState::Checked; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowSoftReferences", "Soft References"),
			LOCTEXT("ShowSoftReferencesTooltip", "Show soft referenced dependencies"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { OnShowSoftReferencesChanged(IsShowSoftReferencesChecked() == ECheckBoxState::Checked ? ECheckBoxState::Unchecked : ECheckBoxState::Checked); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this] { return IsShowSoftReferencesChecked() == ECheckBoxState::Checked; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowHideNativePackages", "Native Packages"),
			LOCTEXT("ShowHideNativePackagesTooltip", "Show or hide native packages"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { OnShowNativePackagesChanged(IsShowNativePackagesChecked() == ECheckBoxState::Checked ? ECheckBoxState::Unchecked : ECheckBoxState::Checked); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this] { return IsShowNativePackagesChecked() == ECheckBoxState::Checked; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Zoom", LOCTEXT("ZoomHeading", "Zoom"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("FitAll", "Fit All"),
			LOCTEXT("FitAllTooltip", "Zooms out to fit all nodes"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { ZoomToFitAll(); })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("FitSelected", "Fit Selected"),
			LOCTEXT("FitSelectedTooltip", "Zooms out to fit selected nodes"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this]	{ ZoomToFitSelected();})
			),
			NAME_None,
			EUserInterfaceActionType::Button
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("FitError", "Select and Fit Error Nodes"),
			LOCTEXT("FitErrorTooltip", "Select and zoom to error nodes"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] 
				{ 
					if (GraphObj == nullptr || !GraphEditorPtr.IsValid())
					{
						return;
					}
					
					GraphEditorPtr->ClearSelectionSet();

					TArray<UEdGraphNode_ExtDependency*> AllNodes;
					GraphObj->GetNodesOfClass<UEdGraphNode_ExtDependency>(AllNodes);

					for (const auto Node : AllNodes)
					{
						if (Node->IsMissingOrInvalid())
						{
							GraphEditorPtr->SetNodeSelection(Node, true);
						}
					}
					
					ZoomToFitSelected();
				})
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Style", LOCTEXT("StyleHeading", "Style"));
	{
#if ECB_WIP_REF_VIEWER_HIGHLIGHT_ERROR
		MenuBuilder.AddMenuEntry(
			LOCTEXT("HighlightErrorNodes", "Highlight Error Nodes"),
			LOCTEXT("HighlightErrorNodesTooltip", "Whether to highlight error nodes"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->HighlightErrorNodesInDependencyViewer = !ExtContentBrowserSetting->HighlightErrorNodesInDependencyViewer; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->HighlightErrorNodesInDependencyViewer; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

		MenuBuilder.AddMenuEntry(
			LOCTEXT("StraightConnectLine", "Straight Connect Line"),
			LOCTEXT("StraightConnectLineTooltip", "Whether use straight or spline connect line between nodes"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->UseStraightLineInDependencyViewer = !ExtContentBrowserSetting->UseStraightLineInDependencyViewer; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->UseStraightLineInDependencyViewer; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Layout", LOCTEXT("LayoutHeading", "Layout"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowUnderAssetView", "Show Under Asset View"),
			LOCTEXT("ShowUnderAssetViewTooltip", "Show dependency viewer under asset view or on right of asset view"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, ExtContentBrowserSetting] { ExtContentBrowserSetting->ShowDependencyViewerUnderAssetView = !ExtContentBrowserSetting->ShowDependencyViewerUnderAssetView; ExtContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ExtContentBrowserSetting] { return ExtContentBrowserSetting->ShowDependencyViewerUnderAssetView; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SExtDependencyViewer::SetCurrentRegistrySource(/*const FAssetManagerEditorRegistrySource* RegistrySource*/)
{
	RebuildGraph();
}

void SExtDependencyViewer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	static float WidthLastFrame = 0.f;
	if (WidthLastFrame != AllottedGeometry.Size.X)
	{
		WidthLastFrame = AllottedGeometry.Size.X;

		NodeSpacingSliderContainer->SetVisibility(WidthLastFrame < 290.f ? EVisibility::Collapsed : EVisibility::Visible);

		ViewOptionsComboButton->SetVisibility(WidthLastFrame < 80.f ? EVisibility::Collapsed : EVisibility::Visible);
	}
}

void SExtDependencyViewer::OnNodeDoubleClicked(UEdGraphNode* Node)
{
#if ECB_WIP_REF_VIEWER_JUMP
	TSet<UObject*> Nodes;
	Nodes.Add(Node);
	ReCenterGraphOnNodes( Nodes );
#endif
}

void SExtDependencyViewer::RebuildGraph()
{
#if ECB_FEA_ASYNC_ASSET_DISCOVERY
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		// We are still discovering assets, listen for the completion delegate before building the graph
		if (!AssetRegistryModule.Get().OnFilesLoaded().IsBoundToObject(this))
		{
			AssetRegistryModule.Get().OnFilesLoaded().AddSP(this, &SExtDependencyViewer::OnInitialAssetRegistrySearchComplete);
		}
	}
	else
#endif
	{
		// All assets are already discovered, build the graph now, if we have one
		if (GraphObj)
		{
			GraphObj->RebuildGraph();
		}
	}
}

FActionMenuContent SExtDependencyViewer::OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
{
	// no context menu when not over a node
	return FActionMenuContent();
}

bool SExtDependencyViewer::IsBackEnabled() const
{
	return HistoryManager.CanGoBack();
}

bool SExtDependencyViewer::IsForwardEnabled() const
{
	return HistoryManager.CanGoForward();
}

FReply SExtDependencyViewer::BackClicked()
{
	HistoryManager.GoBack();

	return FReply::Handled();
}

FReply SExtDependencyViewer::ForwardClicked()
{
	HistoryManager.GoForward();

	return FReply::Handled();
}

void SExtDependencyViewer::GraphNavigateHistoryBack()
{
	BackClicked();
}

void SExtDependencyViewer::GraphNavigateHistoryForward()
{
	ForwardClicked();
}

FText SExtDependencyViewer::GetHistoryBackTooltip() const
{
	if ( HistoryManager.CanGoBack() )
	{
		return FText::Format( LOCTEXT("HistoryBackTooltip", "Back to {0}"), HistoryManager.GetBackDesc() );
	}
	return FText::GetEmpty();
}

FText SExtDependencyViewer::GetHistoryForwardTooltip() const
{
	if ( HistoryManager.CanGoForward() )
	{
		return FText::Format( LOCTEXT("HistoryForwardTooltip", "Forward to {0}"), HistoryManager.GetForwardDesc() );
	}
	return FText::GetEmpty();
}

FText SExtDependencyViewer::GetAddressBarText() const
{
	if ( GraphObj )
	{
		if (TemporaryPathBeingEdited.IsEmpty())
		{
			const TArray<FExtAssetIdentifier>& CurrentGraphRootPackageNames = GraphObj->GetCurrentGraphRootIdentifiers();
			if (CurrentGraphRootPackageNames.Num() == 1)
			{
				return FText::FromString(CurrentGraphRootPackageNames[0].ToString());
			}
			else if (CurrentGraphRootPackageNames.Num() > 1)
			{
				return FText::Format(LOCTEXT("AddressBarMultiplePackagesText", "{0} and {1} others"), FText::FromString(CurrentGraphRootPackageNames[0].ToString()), FText::AsNumber(CurrentGraphRootPackageNames.Num()));
			}
		}
		else
		{
			return TemporaryPathBeingEdited;
		}
	}

	return FText();
}

void SExtDependencyViewer::OnAddressBarTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		TArray<FExtAssetIdentifier> NewPaths;
		NewPaths.Add(FExtAssetIdentifier::FromString(NewText.ToString()));

		SetGraphRootIdentifiers(NewPaths);
	}

	TemporaryPathBeingEdited = FText();
}

void SExtDependencyViewer::OnAddressBarTextChanged(const FText& NewText)
{
	TemporaryPathBeingEdited = NewText;
}

void SExtDependencyViewer::OnApplyHistoryData(const FExtDependencyViewerHistoryData& History)
{
	if ( GraphObj )
	{
		GraphObj->SetGraphRoot(History.Identifiers);
		UEdGraphNode_ExtDependency* NewRootNode = GraphObj->RebuildGraph();
		
		if ( NewRootNode && ensure(GraphEditorPtr.IsValid()) )
		{
			GraphEditorPtr->SetNodeSelection(NewRootNode, true);
		}
	}
}

void SExtDependencyViewer::OnUpdateHistoryData(FExtDependencyViewerHistoryData& HistoryData) const
{
	if ( GraphObj )
	{
		const TArray<FExtAssetIdentifier>& CurrentGraphRootIdentifiers = GraphObj->GetCurrentGraphRootIdentifiers();
		HistoryData.HistoryDesc = GetAddressBarText();
		HistoryData.Identifiers = CurrentGraphRootIdentifiers;
	}
	else
	{
		HistoryData.HistoryDesc = FText::GetEmpty();
		HistoryData.Identifiers.Empty();
	}
}

void SExtDependencyViewer::OnSearchDepthEnabledChanged( ECheckBoxState NewState )
{
	if ( GraphObj )
	{
		GraphObj->SetSearchDepthLimitEnabled(NewState == ECheckBoxState::Checked);
		RebuildGraph();
		ZoomToFitAll();
	}
}

ECheckBoxState SExtDependencyViewer::IsSearchDepthEnabledChecked() const
{
	if ( GraphObj )
	{
		return GraphObj->IsSearchDepthLimited() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

int32 SExtDependencyViewer::GetSearchDepthCount() const
{
	if ( GraphObj )
	{
		return GraphObj->GetSearchDepthLimit();
	}
	else
	{
		return 0;
	}
}

void SExtDependencyViewer::OnSearchDepthCommitted(int32 NewValue)
{
	if ( GraphObj )
	{
		GraphObj->SetSearchDepthLimit(NewValue);
		RebuildGraph();
	}
}

void SExtDependencyViewer::OnSearchBreadthEnabledChanged( ECheckBoxState NewState )
{
	if ( GraphObj )
	{
		GraphObj->SetSearchBreadthLimitEnabled(NewState == ECheckBoxState::Checked);
		RebuildGraph();
	}
}

ECheckBoxState SExtDependencyViewer::IsSearchBreadthEnabledChecked() const
{
	if ( GraphObj )
	{
		return GraphObj->IsSearchBreadthLimited() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

TSharedRef<SWidget> SExtDependencyViewer::GenerateCollectionFilterItem(TSharedPtr<FName> InItem)
{
	FText ItemAsText = FText::FromName(*InItem);
	return
		SNew(SBox)
		.WidthOverride(300)
		[
			SNew(STextBlock)
			.Text(ItemAsText)
			.ToolTipText(ItemAsText)
		];
}

void SExtDependencyViewer::OnEnableCollectionFilterChanged(ECheckBoxState NewState)
{
	if (GraphObj)
	{
		const bool bNewValue = NewState == ECheckBoxState::Checked;
		const bool bCurrentValue = GraphObj->GetEnableCollectionFilter();
		if (bCurrentValue != bNewValue)
		{
			GraphObj->SetEnableCollectionFilter(NewState == ECheckBoxState::Checked);
			RebuildGraph();
		}
	}
}

ECheckBoxState SExtDependencyViewer::IsEnableCollectionFilterChecked() const
{
	if (GraphObj)
	{
		return GraphObj->GetEnableCollectionFilter() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

void SExtDependencyViewer::HandleCollectionFilterChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo)
{
	if (GraphObj)
	{
		const FName NewFilter = *Item;
		const FName CurrentFilter = GraphObj->GetCurrentCollectionFilter();
		if (CurrentFilter != NewFilter)
		{
			if (CurrentFilter == NAME_None)
			{
				// Automatically check the box to enable the filter if the previous filter was None
				GraphObj->SetEnableCollectionFilter(true);
			}

			GraphObj->SetCurrentCollectionFilter(NewFilter);
			RebuildGraph();
		}
	}
}

FText SExtDependencyViewer::GetCollectionFilterText() const
{
	return FText::FromName(GraphObj->GetCurrentCollectionFilter());
}

void SExtDependencyViewer::OnShowSoftReferencesChanged(ECheckBoxState NewState)
{
	if (GraphObj)
	{
		GraphObj->SetShowSoftReferencesEnabled(NewState == ECheckBoxState::Checked);
		RebuildGraph();
	}
}

ECheckBoxState SExtDependencyViewer::IsShowSoftReferencesChecked() const
{
	if (GraphObj)
	{
		return GraphObj->IsShowSoftReferences()? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

void SExtDependencyViewer::OnShowHardReferencesChanged(ECheckBoxState NewState)
{
	if (GraphObj)
	{
		GraphObj->SetShowHardReferencesEnabled(NewState == ECheckBoxState::Checked);
		RebuildGraph();
	}
}


ECheckBoxState SExtDependencyViewer::IsShowHardReferencesChecked() const
{
	if (GraphObj)
	{
		return GraphObj->IsShowHardReferences() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

EVisibility SExtDependencyViewer::GetManagementReferencesVisibility() const
{
	if (UAssetManager::IsValid())
	{
		return EVisibility::SelfHitTestInvisible;
	}
	return EVisibility::Collapsed;
}

void SExtDependencyViewer::OnShowManagementReferencesChanged(ECheckBoxState NewState)
{
	if (GraphObj)
	{
		// This can take a few seconds if it isn't ready
		UAssetManager::Get().UpdateManagementDatabase();

		GraphObj->SetShowManagementReferencesEnabled(NewState == ECheckBoxState::Checked);
		RebuildGraph();
	}
}

ECheckBoxState SExtDependencyViewer::IsShowManagementReferencesChecked() const
{
	if (GraphObj)
	{
		return GraphObj->IsShowManagementReferences() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

void SExtDependencyViewer::OnShowSearchableNamesChanged(ECheckBoxState NewState)
{
	if (GraphObj)
	{
		GraphObj->SetShowSearchableNames(NewState == ECheckBoxState::Checked);
		RebuildGraph();
	}
}

ECheckBoxState SExtDependencyViewer::IsShowSearchableNamesChecked() const
{
	if (GraphObj)
	{
		return GraphObj->IsShowSearchableNames() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

void SExtDependencyViewer::OnShowNativePackagesChanged(ECheckBoxState NewState)
{
	if (GraphObj)
	{
		GraphObj->SetShowNativePackages(NewState == ECheckBoxState::Checked);
		RebuildGraph();
	}
}

ECheckBoxState SExtDependencyViewer::IsShowNativePackagesChecked() const
{
	if (GraphObj)
	{
		return GraphObj->IsShowNativePackages() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

int32 SExtDependencyViewer::GetSearchBreadthCount() const
{
	if ( GraphObj )
	{
		return GraphObj->GetSearchBreadthLimit();
	}
	else
	{
		return 0;
	}
}

void SExtDependencyViewer::OnSearchBreadthCommitted(int32 NewValue)
{
	if ( GraphObj )
	{
		GraphObj->SetSearchBreadthLimit(NewValue);
		RebuildGraph();
	}
}

void SExtDependencyViewer::RegisterActions()
{
	ReferenceViewerActions = MakeShareable(new FUICommandList);

	FExtDependencyViewerCommands::Register();

	ReferenceViewerActions->MapAction(
		FExtDependencyViewerCommands::Get().ZoomToFitSelected,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ZoomToFitSelected),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::CanZoomToFit));
	
#if ECB_LEGACY

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().Find,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::OnFind));

	ReferenceViewerActions->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ShowSelectionInContentBrowser),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().OpenSelectedInAssetEditor,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::OpenSelectedInAssetEditor),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOneRealNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().ReCenterGraph,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ReCenterGraph),
		FCanExecuteAction());

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().CopyReferencedObjects,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::CopyReferencedObjects),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().CopyReferencingObjects,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::CopyReferencingObjects),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().ShowReferencedObjects,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ShowReferencedObjects),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().ShowReferencingObjects,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ShowReferencingObjects),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().MakeLocalCollectionWithReferencers,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies, ECollectionShareType::CST_Local, true),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));
	
	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().MakePrivateCollectionWithReferencers,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies, ECollectionShareType::CST_Private, true),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));
	
	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().MakeSharedCollectionWithReferencers,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies, ECollectionShareType::CST_Shared, true),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().MakeLocalCollectionWithDependencies,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies, ECollectionShareType::CST_Local, false),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().MakePrivateCollectionWithDependencies,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies, ECollectionShareType::CST_Private, false),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().MakeSharedCollectionWithDependencies,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies, ECollectionShareType::CST_Shared, false),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));
	
	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().ShowReferenceTree,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ShowReferenceTree),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasExactlyOnePackageNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().ViewSizeMap,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ViewSizeMap),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOneRealNodeSelected));

	ReferenceViewerActions->MapAction(
		FAssetManagerEditorCommands::Get().ViewAssetAudit,
		FExecuteAction::CreateSP(this, &SExtDependencyViewer::ViewAssetAudit),
		FCanExecuteAction::CreateSP(this, &SExtDependencyViewer::HasAtLeastOneRealNodeSelected));
#endif
}

void SExtDependencyViewer::ShowSelectionInContentBrowser()
{
	TArray<FExtAssetData> AssetList;

	// Build up a list of selected assets from the graph selection set
	TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		if (UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(*It))
		{
			if (ReferenceNode->GetAssetData().IsValid())
			{
				AssetList.Add(ReferenceNode->GetAssetData());
			}
		}
	}

	if (AssetList.Num() > 0)
	{
		//GEditor->SyncBrowserToObjects(AssetList);
	}
}

void SExtDependencyViewer::OpenSelectedInAssetEditor()
{
	TArray<FExtAssetIdentifier> IdentifiersToEdit;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		if (UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(*It))
		{
			if (!ReferenceNode->IsCollapsed())
			{
				ReferenceNode->GetAllIdentifiers(IdentifiersToEdit);
			}
		}
	}

	// This will handle packages as well as searchable names if other systems register
	//FEditorDelegates::OnEditAssetIdentifiers.Broadcast(IdentifiersToEdit);
}

void SExtDependencyViewer::ReCenterGraph()
{
	ReCenterGraphOnNodes( GraphEditorPtr->GetSelectedNodes() );
}

FString SExtDependencyViewer::GetReferencedObjectsList() const
{
	FString ReferencedObjectsList;

	TSet<FName> AllSelectedPackageNames;
	GetPackageNamesFromSelectedNodes(AllSelectedPackageNames);

	if (AllSelectedPackageNames.Num() > 0)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		for (const FName SelectedPackageName : AllSelectedPackageNames)
		{
			TArray<FName> HardDependencies;
			FExtContentBrowserSingleton::GetAssetRegistry().GetDependencies(SelectedPackageName, HardDependencies, EAssetRegistryDependencyType::Hard);
			
			TArray<FName> SoftDependencies;
			FExtContentBrowserSingleton::GetAssetRegistry().GetDependencies(SelectedPackageName, SoftDependencies, EAssetRegistryDependencyType::Soft);

			ReferencedObjectsList += FString::Printf(TEXT("[%s - Dependencies]\n"), *SelectedPackageName.ToString());
			if (HardDependencies.Num() > 0)
			{
				ReferencedObjectsList += TEXT("  [HARD]\n");
				for (const FName HardDependency : HardDependencies)
				{
					const FString PackageString = HardDependency.ToString();
					ReferencedObjectsList += FString::Printf(TEXT("    %s.%s\n"), *PackageString, *FPackageName::GetLongPackageAssetName(PackageString));
				}
			}
			if (SoftDependencies.Num() > 0)
			{
				ReferencedObjectsList += TEXT("  [SOFT]\n");
				for (const FName SoftDependency : SoftDependencies)
				{
					const FString PackageString = SoftDependency.ToString();
					ReferencedObjectsList += FString::Printf(TEXT("    %s.%s\n"), *PackageString, *FPackageName::GetLongPackageAssetName(PackageString));
				}
			}
		}
	}

	return ReferencedObjectsList;
}

FString SExtDependencyViewer::GetReferencingObjectsList() const
{
	FString ReferencingObjectsList;

	TSet<FName> AllSelectedPackageNames;
	GetPackageNamesFromSelectedNodes(AllSelectedPackageNames);

	if (AllSelectedPackageNames.Num() > 0)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		for (const FName SelectedPackageName : AllSelectedPackageNames)
		{
			TArray<FName> HardDependencies;
			FExtContentBrowserSingleton::GetAssetRegistry().GetReferencers(SelectedPackageName, HardDependencies, EAssetRegistryDependencyType::Hard);

			TArray<FName> SoftDependencies;
			AssetRegistryModule.Get().GetReferencers(SelectedPackageName, SoftDependencies, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Soft);

			ReferencingObjectsList += FString::Printf(TEXT("[%s - Referencers]\n"), *SelectedPackageName.ToString());
			if (HardDependencies.Num() > 0)
			{
				ReferencingObjectsList += TEXT("  [HARD]\n");
				for (const FName HardDependency : HardDependencies)
				{
					const FString PackageString = HardDependency.ToString();
					ReferencingObjectsList += FString::Printf(TEXT("    %s.%s\n"), *PackageString, *FPackageName::GetLongPackageAssetName(PackageString));
				}
			}
			if (SoftDependencies.Num() > 0)
			{
				ReferencingObjectsList += TEXT("  [SOFT]\n");
				for (const FName SoftDependency : SoftDependencies)
				{
					const FString PackageString = SoftDependency.ToString();
					ReferencingObjectsList += FString::Printf(TEXT("    %s.%s\n"), *PackageString, *FPackageName::GetLongPackageAssetName(PackageString));
				}
			}
		}
	}

	return ReferencingObjectsList;
}

void SExtDependencyViewer::CopyReferencedObjects()
{
	const FString ReferencedObjectsList = GetReferencedObjectsList();
	FPlatformApplicationMisc::ClipboardCopy(*ReferencedObjectsList);
}

void SExtDependencyViewer::CopyReferencingObjects()
{
	const FString ReferencingObjectsList = GetReferencingObjectsList();
	FPlatformApplicationMisc::ClipboardCopy(*ReferencingObjectsList);
}

void SExtDependencyViewer::ShowReferencedObjects()
{
	const FString ReferencedObjectsList = GetReferencedObjectsList();
	SGenericDialogWidget::OpenDialog(LOCTEXT("ReferencedObjectsDlgTitle", "Referenced Objects"), SNew(STextBlock).Text(FText::FromString(ReferencedObjectsList)));
}

void SExtDependencyViewer::ShowReferencingObjects()
{	
	const FString ReferencingObjectsList = GetReferencingObjectsList();
	SGenericDialogWidget::OpenDialog(LOCTEXT("ReferencingObjectsDlgTitle", "Referencing Objects"), SNew(STextBlock).Text(FText::FromString(ReferencingObjectsList)));
}

void SExtDependencyViewer::MakeCollectionWithReferencersOrDependencies(ECollectionShareType::Type ShareType, bool bReferencers)
{
	TSet<FName> AllSelectedPackageNames;
	GetPackageNamesFromSelectedNodes(AllSelectedPackageNames);

	if (AllSelectedPackageNames.Num() > 0)
	{
		if (ensure(ShareType != ECollectionShareType::CST_All))
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			FText CollectionNameAsText;
			FString FirstAssetName = FPackageName::GetLongPackageAssetName(AllSelectedPackageNames.Array()[0].ToString());
			if (bReferencers)
			{
				if (AllSelectedPackageNames.Num() > 1)
				{
					CollectionNameAsText = FText::Format(LOCTEXT("ReferencersForMultipleAssetNames", "{0}AndOthers_Referencers"), FText::FromString(FirstAssetName));
				}
				else
				{
					CollectionNameAsText = FText::Format(LOCTEXT("ReferencersForSingleAsset", "{0}_Referencers"), FText::FromString(FirstAssetName));
				}
			}
			else
			{
				if (AllSelectedPackageNames.Num() > 1)
				{
					CollectionNameAsText = FText::Format(LOCTEXT("DependenciesForMultipleAssetNames", "{0}AndOthers_Dependencies"), FText::FromString(FirstAssetName));
				}
				else
				{
					CollectionNameAsText = FText::Format(LOCTEXT("DependenciesForSingleAsset", "{0}_Dependencies"), FText::FromString(FirstAssetName));
				}
			}

			FName CollectionName;
			CollectionManagerModule.Get().CreateUniqueCollectionName(*CollectionNameAsText.ToString(), ShareType, CollectionName);

			FText ResultsMessage;
			
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			TArray<FName> PackageNamesToAddToCollection;
			if (bReferencers)
			{
				for (FName SelectedPackage : AllSelectedPackageNames)
				{
					FExtContentBrowserSingleton::GetAssetRegistry().GetReferencers(SelectedPackage, PackageNamesToAddToCollection);
				}
			}
			else
			{
				for (FName SelectedPackage : AllSelectedPackageNames)
				{
					FExtContentBrowserSingleton::GetAssetRegistry().GetDependencies(SelectedPackage, PackageNamesToAddToCollection);
				}
			}

			TSet<FName> PackageNameSet;
			for (FName PackageToAdd : PackageNamesToAddToCollection)
			{
				if (!AllSelectedPackageNames.Contains(PackageToAdd))
				{
					PackageNameSet.Add(PackageToAdd);
				}
			}

			//IAssetManagerEditorModule::Get().WriteCollection(CollectionName, ShareType, PackageNameSet.Array(), true);
		}
	}
}

void SExtDependencyViewer::ShowReferenceTree()
{
	UObject* SelectedObject = GetObjectFromSingleSelectedNode();

	if ( SelectedObject )
	{
		bool bObjectWasSelected = false;
		for (FSelectionIterator It(*GEditor->GetSelectedObjects()) ; It; ++It)
		{
			if ( (*It) == SelectedObject )
			{
				GEditor->GetSelectedObjects()->Deselect( SelectedObject );
				bObjectWasSelected = true;
			}
		}

		ObjectTools::ShowReferenceGraph( SelectedObject );

		if ( bObjectWasSelected )
		{
			GEditor->GetSelectedObjects()->Select( SelectedObject );
		}
	}
}

void SExtDependencyViewer::ViewSizeMap()
{
	TArray<FExtAssetIdentifier> AssetIdentifiers;
	TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
	for (UObject* Node : SelectedNodes)
	{
		UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(Node);
		if (ReferenceNode)
		{
			ReferenceNode->GetAllIdentifiers(AssetIdentifiers);
		}
	}
#if ECB_LEGACY
	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenSizeMapUI(AssetIdentifiers);
	}
#endif
}

void SExtDependencyViewer::ViewAssetAudit()
{
	TSet<FName> SelectedAssetPackageNames;
	GetPackageNamesFromSelectedNodes(SelectedAssetPackageNames);
#if ECB_LEGACY
	if (SelectedAssetPackageNames.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenAssetAuditUI(SelectedAssetPackageNames.Array());
	}
#endif
}

void SExtDependencyViewer::ReCenterGraphOnNodes(const TSet<UObject*>& Nodes)
{
	TArray<FExtAssetIdentifier> NewGraphRootNames;
	FIntPoint TotalNodePos(ForceInitToZero);
	for ( auto NodeIt = Nodes.CreateConstIterator(); NodeIt; ++NodeIt )
	{
		UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(*NodeIt);
		if ( ReferenceNode )
		{
			ReferenceNode->GetAllIdentifiers(NewGraphRootNames);
			TotalNodePos.X += ReferenceNode->NodePosX;
			TotalNodePos.Y += ReferenceNode->NodePosY;
		}
	}

	if ( NewGraphRootNames.Num() > 0 )
	{
		const FIntPoint AverageNodePos = TotalNodePos / NewGraphRootNames.Num();
		GraphObj->SetGraphRoot(NewGraphRootNames, AverageNodePos);
		UEdGraphNode_ExtDependency* NewRootNode = GraphObj->RebuildGraph();

		if ( NewRootNode && ensure(GraphEditorPtr.IsValid()) )
		{
			GraphEditorPtr->ClearSelectionSet();
			GraphEditorPtr->SetNodeSelection(NewRootNode, true);
		}

		// Set the initial history data
		HistoryManager.AddHistoryData();
	}
}

UObject* SExtDependencyViewer::GetObjectFromSingleSelectedNode() const
{
	UObject* ReturnObject = nullptr;

	TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
	if ( ensure(SelectedNodes.Num()) == 1 )
	{
		UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(SelectedNodes.Array()[0]);
		if ( ReferenceNode )
		{
			const FExtAssetData& AssetData = ReferenceNode->GetAssetData();
			if (AssetData.IsAssetLoaded())
			{
				ReturnObject = AssetData.GetAsset();
			}
			else
			{
				FScopedSlowTask SlowTask(0, LOCTEXT("LoadingSelectedObject", "Loading selection..."));
				SlowTask.MakeDialog();
				ReturnObject = AssetData.GetAsset();
			}
		}
	}

	return ReturnObject;
}

void SExtDependencyViewer::GetPackageNamesFromSelectedNodes(TSet<FName>& OutNames) const
{
	TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
	for (UObject* Node : SelectedNodes)
	{
		UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(Node);
		if (ReferenceNode)
		{
			TArray<FName> NodePackageNames;
			ReferenceNode->GetAllPackageNames(NodePackageNames);
			OutNames.Append(NodePackageNames);
		}
	}
}

bool SExtDependencyViewer::HasExactlyOneNodeSelected() const
{
	if ( GraphEditorPtr.IsValid() )
	{
		return GraphEditorPtr->GetSelectedNodes().Num() == 1;
	}
	
	return false;
}

bool SExtDependencyViewer::HasExactlyOnePackageNodeSelected() const
{
	if (GraphEditorPtr.IsValid())
	{
		if (GraphEditorPtr->GetSelectedNodes().Num() != 1)
		{
			return false;
		}

		TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
		for (UObject* Node : SelectedNodes)
		{
			UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(Node);
			if (ReferenceNode)
			{
				if (ReferenceNode->IsPackage())
				{
					return true;
				}
			}
			return false;
		}
	}

	return false;
}

bool SExtDependencyViewer::HasAtLeastOnePackageNodeSelected() const
{
	if ( GraphEditorPtr.IsValid() )
	{
		TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
		for (UObject* Node : SelectedNodes)
		{
			UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(Node);
			if (ReferenceNode)
			{
				if (ReferenceNode->IsPackage())
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

bool SExtDependencyViewer::HasAtLeastOneRealNodeSelected() const
{
	if (GraphEditorPtr.IsValid())
	{
		TSet<UObject*> SelectedNodes = GraphEditorPtr->GetSelectedNodes();
		for (UObject* Node : SelectedNodes)
		{
			UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(Node);
			if (ReferenceNode)
			{
				if (!ReferenceNode->IsCollapsed())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void SExtDependencyViewer::OnInitialAssetRegistrySearchComplete()
{
	if ( GraphObj )
	{
		GraphObj->RebuildGraph();
	}
}

void SExtDependencyViewer::ZoomToFitSelected()
{
	if (GraphEditorPtr.IsValid())
	{
		GraphEditorPtr->ZoomToFit(/*bSelectedOnly*/true);
	}
}

void SExtDependencyViewer::ZoomToFitAll()
{
	if (GraphEditorPtr.IsValid())
	{
		GraphEditorPtr->ZoomToFit(/*bSelectedOnly*/false);
	}
}

bool SExtDependencyViewer::CanZoomToFit() const
{
	if (GraphEditorPtr.IsValid())
	{
		return true;
	}

	return false;
}

void SExtDependencyViewer::OnFind()
{
	FSlateApplication::Get().SetKeyboardFocus(SearchBox, EFocusCause::SetDirectly);
}

void SExtDependencyViewer::HandleOnSearchTextChanged(const FText& SearchText)
{
	if (GraphObj == nullptr || !GraphEditorPtr.IsValid())
	{
		return;
	}

	GraphEditorPtr->ClearSelectionSet();

	if (SearchText.IsEmpty())
	{
		return;
	}

	FString SearchString = SearchText.ToString();
	TArray<FString> SearchWords;
	SearchString.ParseIntoArrayWS( SearchWords );

	TArray<UEdGraphNode_ExtDependency*> AllNodes;
	GraphObj->GetNodesOfClass<UEdGraphNode_ExtDependency>( AllNodes );

	TArray<FName> NodePackageNames;
	for (UEdGraphNode_ExtDependency* Node : AllNodes)
	{
		NodePackageNames.Empty();
		Node->GetAllPackageNames(NodePackageNames);

		for (const FName& PackageName : NodePackageNames)
		{
			// package name must match all words
			bool bMatch = true;
			for (const FString& Word : SearchWords)
			{
				if (!PackageName.ToString().Contains(Word))
				{
					bMatch = false;
					break;
				}
			}

			if (bMatch)
			{
				GraphEditorPtr->SetNodeSelection(Node, true);
				break;
			}
		}
	}
}

void SExtDependencyViewer::HandleOnSearchTextCommitted(const FText& SearchText, ETextCommit::Type CommitType)
{
	if (!GraphEditorPtr.IsValid())
	{
		return;
	}

	if (CommitType == ETextCommit::OnCleared)
	{
		GraphEditorPtr->ClearSelectionSet();
	}
		
	GraphEditorPtr->ZoomToFit(true);
}

#undef LOCTEXT_NAMESPACE

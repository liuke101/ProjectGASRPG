// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "GraphEditor.h"
#include "ExtDependencyHistoryManager.h"
#include "CollectionManagerTypes.h"

class UEdGraph;
class UEdGraph_ExtDependencyViewer;

class SComboButton;

/**
 * 
 */
class SExtDependencyViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SExtDependencyViewer ){}

	SLATE_END_ARGS()

	~SExtDependencyViewer();

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	/** Sets a new root package name */
	void SetGraphRootIdentifiers(const TArray<FExtAssetIdentifier>& NewGraphRootIdentifiers);

	/** Gets graph editor */
	TSharedPtr<SGraphEditor> GetGraphEditor() const { return GraphEditorPtr; }

	/** Called when the current registry source changes */
	void SetCurrentRegistrySource(/*const FAssetManagerEditorRegistrySource* RegistrySource*/);

	// SWidget inherited
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:

	/** Call after a structural change is made that causes the graph to be recreated */
	void RebuildGraph();

	/** Called to create context menu when right-clicking on graph */
	FActionMenuContent OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);

	/** Called when a node is double clicked */
	void OnNodeDoubleClicked(class UEdGraphNode* Node);

	/** True if the user may use the history back button */
	bool IsBackEnabled() const;

	/** True if the user may use the history forward button */
	bool IsForwardEnabled() const;

	/** Handler for clicking the history back button */
	FReply BackClicked();

	/** Handler for clicking the history forward button */
	FReply ForwardClicked();

	/** Handler for when the graph panel tells us to go back in history (like using the mouse thumb button) */
	void GraphNavigateHistoryBack();

	/** Handler for when the graph panel tells us to go forward in history (like using the mouse thumb button) */
	void GraphNavigateHistoryForward();

	/** Gets the tool tip text for the history back button */
	FText GetHistoryBackTooltip() const;

	/** Gets the tool tip text for the history forward button */
	FText GetHistoryForwardTooltip() const;

	/** Gets the text to be displayed in the address bar */
	FText GetAddressBarText() const;

	/** Called when the path is being edited */
	void OnAddressBarTextChanged(const FText& NewText);

	/** Sets the new path for the viewer */
	void OnAddressBarTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo);

	void OnApplyHistoryData(const FExtDependencyViewerHistoryData& History);

	void OnUpdateHistoryData(FExtDependencyViewerHistoryData& HistoryData) const;
	
	void OnSearchDepthEnabledChanged( ECheckBoxState NewState );
	ECheckBoxState IsSearchDepthEnabledChecked() const;
	int32 GetSearchDepthCount() const;
	void OnSearchDepthCommitted(int32 NewValue);

	void OnSearchBreadthEnabledChanged( ECheckBoxState NewState );
	ECheckBoxState IsSearchBreadthEnabledChecked() const;

	void OnEnableCollectionFilterChanged(ECheckBoxState NewState);
	ECheckBoxState IsEnableCollectionFilterChecked() const;
	TSharedRef<SWidget> GenerateCollectionFilterItem(TSharedPtr<FName> InItem);
	void HandleCollectionFilterChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo);
	FText GetCollectionFilterText() const;

	void OnShowSoftReferencesChanged( ECheckBoxState NewState );
	ECheckBoxState IsShowSoftReferencesChecked() const;
	void OnShowHardReferencesChanged(ECheckBoxState NewState);
	ECheckBoxState IsShowHardReferencesChecked() const;

	EVisibility GetManagementReferencesVisibility() const;
	void OnShowManagementReferencesChanged(ECheckBoxState NewState);
	ECheckBoxState IsShowManagementReferencesChecked() const;

	void OnShowSearchableNamesChanged(ECheckBoxState NewState);
	ECheckBoxState IsShowSearchableNamesChecked() const;
	void OnShowNativePackagesChanged(ECheckBoxState NewState);
	ECheckBoxState IsShowNativePackagesChecked() const;

	int32 GetSearchBreadthCount() const;
	void OnSearchBreadthCommitted(int32 NewValue);

	void RegisterActions();
	void ShowSelectionInContentBrowser();
	void OpenSelectedInAssetEditor();
	void ReCenterGraph();
	void CopyReferencedObjects();
	void CopyReferencingObjects();
	void ShowReferencedObjects();
	void ShowReferencingObjects();
	void MakeCollectionWithReferencersOrDependencies(ECollectionShareType::Type ShareType, bool bReferencers);
	void ShowReferenceTree();
	void ViewSizeMap();
	void ViewAssetAudit();
	void ZoomToFitSelected();
	void ZoomToFitAll();
	bool CanZoomToFit() const;
	void OnFind();

	/** Handlers for searching */
	void HandleOnSearchTextChanged(const FText& SearchText);
	void HandleOnSearchTextCommitted(const FText& SearchText, ETextCommit::Type CommitType);

	void ReCenterGraphOnNodes(const TSet<UObject*>& Nodes);

	FString GetReferencedObjectsList() const;
	FString GetReferencingObjectsList() const;

	UObject* GetObjectFromSingleSelectedNode() const;
	void GetPackageNamesFromSelectedNodes(TSet<FName>& OutNames) const;
	bool HasExactlyOneNodeSelected() const;
	bool HasExactlyOnePackageNodeSelected() const;
	bool HasAtLeastOnePackageNodeSelected() const;
	bool HasAtLeastOneRealNodeSelected() const;

	void OnInitialAssetRegistrySearchComplete();
	EActiveTimerReturnType TriggerZoomToFit(double InCurrentTime, float InDeltaTime);

public:

	/** Saves any settings to config that should be persistent between editor sessions */
	void SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const;

	/** Loads any settings to config that should be persistent between editor sessions */
	void LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString);


private:

	/** Gets the current value for the scale slider (0 to 1) */
	float GetNodeSpacingScale() const;

	/** Sets the current scale value (0 to 1) */
	void SetNodeSpacingScale(float NewValue);

	/** Is node spacing slider locked? */
	bool IsNodeSpacingScaleLocked() const;

	/** Returns the foreground color for the view button */
	FSlateColor GetViewButtonForegroundColor() const;

	/** Handler for when the view combo button is clicked */
	TSharedRef<SWidget> GetViewButtonContent();

	/** The widget holds node spacing slider */
	TSharedPtr<SWidget> NodeSpacingSliderContainer;

	/** The button that displays view options */
	TSharedPtr<SComboButton> ViewOptionsComboButton;

private:

	/** The manager that keeps track of history data for this browser */
	FExtDependencyViewerHistoryManager HistoryManager;

	TSharedPtr<SGraphEditor> GraphEditorPtr;

	TSharedPtr<FUICommandList> ReferenceViewerActions;
	TSharedPtr<SSearchBox> SearchBox;

	UEdGraph_ExtDependencyViewer* GraphObj;

	/** The temporary copy of the path text when it is actively being edited. */
	FText TemporaryPathBeingEdited;

	/** List of collection filter options */
	TArray<TSharedPtr<FName>> CollectionsComboList;

	float NodeSpacingScale = 0.1f;
};

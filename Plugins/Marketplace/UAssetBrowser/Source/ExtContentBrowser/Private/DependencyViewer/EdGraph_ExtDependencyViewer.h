// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtAssetData.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraph.h"
#include "Misc/AssetRegistryInterface.h"

#include "EdGraph_ExtDependencyViewer.generated.h"

struct ExtAssetData;

class FExtAssetThumbnailPool;
class UEdGraphNode_ExtDependency;
class SExtDependencyViewer;

UCLASS()
class UEdGraph_ExtDependencyViewer : public UEdGraph
{
	GENERATED_UCLASS_BODY()

public:
	// UObject implementation
	virtual void BeginDestroy() override;
	// End UObject implementation

	/** Set reference viewer to focus on these assets */
	void SetGraphRoot(const TArray<FExtAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin = FIntPoint(ForceInitToZero));

	/** Returns list of currently focused assets */
	const TArray<FExtAssetIdentifier>& GetCurrentGraphRootIdentifiers() const;

	/** If you're extending the reference viewer via GetAllGraphEditorContextMenuExtender you can use this to get the list of selected assets to use in your menu extender */
	bool GetSelectedAssetsForMenuExtender(const class UEdGraphNode* Node, TArray<FExtAssetIdentifier>& SelectedAssets) const;

	/** Accessor for the thumbnail pool in this graph */
	const TSharedPtr<class FExtAssetThumbnailPool>& GetAssetThumbnailPool() const;

	/** Force the graph to rebuild */
	class UEdGraphNode_ExtDependency* RebuildGraph();

	bool IsSearchDepthLimited() const;
	bool IsSearchBreadthLimited() const;
	bool IsShowSoftReferences() const;
	bool IsShowHardReferences() const;
	bool IsShowManagementReferences() const;
	bool IsShowSearchableNames() const;
	bool IsShowNativePackages() const;

	void SetSearchDepthLimitEnabled(bool newEnabled);
	void SetSearchBreadthLimitEnabled(bool newEnabled);
	void SetShowSoftReferencesEnabled(bool newEnabled);
	void SetShowHardReferencesEnabled(bool newEnabled);
	void SetShowManagementReferencesEnabled(bool newEnabled);
	void SetShowSearchableNames(bool newEnabled);
	void SetShowNativePackages(bool newEnabled);

	int32 GetSearchDepthLimit() const;
	int32 GetSearchBreadthLimit() const;

	void SetSearchDepthLimit(int32 NewDepthLimit);
	void SetSearchBreadthLimit(int32 NewBreadthLimit);

	FName GetCurrentCollectionFilter() const;
	void SetCurrentCollectionFilter(FName NewFilter);

	bool GetEnableCollectionFilter() const;
	void SetEnableCollectionFilter(bool bEnabled);

public:
	float GetNodeXSpacing() const;
	void SetNodeXSpacing(float NewNodeXSpacing);

private:
	void SetReferenceViewer(TSharedPtr<SExtDependencyViewer> InViewer);
	UEdGraphNode_ExtDependency* ConstructNodes(const TArray<FExtAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin);
	int32 RecursivelyGatherSizes(bool bReferencers, const TArray<FExtAssetIdentifier>& Identifiers, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FExtAssetIdentifier>& VisitedNames, TMap<FExtAssetIdentifier, int32>& OutNodeSizes) const;
	void GatherAssetData(const TSet<FName>& AllPackageNames, TMap<FName, FExtAssetData>& OutPackageToAssetDataMap) const;
	class UEdGraphNode_ExtDependency* RecursivelyConstructNodes(bool bReferencers, UEdGraphNode_ExtDependency* RootNode, const TArray<FExtAssetIdentifier>& Identifiers, const FIntPoint& NodeLoc, const TMap<FExtAssetIdentifier, int32>& NodeSizes, const TMap<FName, FExtAssetData>& PackagesToAssetDataMap, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FExtAssetIdentifier>& VisitedNames);

	bool ExceedsMaxSearchDepth(int32 Depth) const;
	bool ExceedsMaxSearchBreadth(int32 Breadth) const;
	EAssetRegistryDependencyType::Type GetReferenceSearchFlags(bool bHardOnly) const;

	UEdGraphNode_ExtDependency* CreateReferenceNode();

	/** Removes all nodes from the graph */
	void RemoveAllNodes();

	/** Returns true if filtering is enabled and we have a valid collection */
	bool ShouldFilterByCollection() const;

private:
	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<FExtAssetThumbnailPool> AssetThumbnailPool;

	/** Editor for this pool */
	TWeakPtr<SExtDependencyViewer> ReferenceViewer;

	TArray<FExtAssetIdentifier> CurrentGraphRootIdentifiers;
	FIntPoint CurrentGraphRootOrigin;

	int32 MaxSearchDepth;
	int32 MaxSearchBreadth;

	/** Current collection filter. NAME_None for no filter */
	FName CurrentCollectionFilter;
	bool bEnableCollectionFilter;

	bool bLimitSearchDepth;
	bool bLimitSearchBreadth;
	bool bIsShowSoftReferences;
	bool bIsShowHardReferences;
	bool bIsShowManagementReferences;
	bool bIsShowSearchableNames;
	bool bIsShowNativePackages;

	float NodeXSpacing;

	friend SExtDependencyViewer;
};

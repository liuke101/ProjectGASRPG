// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtAssetData.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph_ExtDependencyViewer.h"

#include "EdGraphNode_ExtDependency.generated.h"

class UEdGraphPin;

UCLASS()
class UEdGraphNode_ExtDependency : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** Returns first asset identifier */
	FExtAssetIdentifier GetIdentifier() const;
	
	/** Returns all identifiers on this node including virtual things */
	void GetAllIdentifiers(TArray<FExtAssetIdentifier>& OutIdentifiers) const;

	/** Returns only the packages in this node, skips searchable names */
	void GetAllPackageNames(TArray<FName>& OutPackageNames) const;

	/** Returns our owning graph */
	UEdGraph_ExtDependencyViewer* GetDependencyViewerGraph() const;

	// UEdGraphNode implementation
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void AllocateDefaultPins() override;
	virtual UObject* GetJumpTargetForDoubleClick() const override;
	// End UEdGraphNode implementation

	bool UsesThumbnail() const;
	bool IsPackage() const;
	bool IsCollapsed() const;
	bool IsMissingOrInvalid() const;
	FExtAssetData GetAssetData() const;

	UEdGraphPin* GetDependencyPin();
	UEdGraphPin* GetReferencerPin();

private:
	void CacheAssetData(const FExtAssetData& AssetData);
	void SetupReferenceNode(const FIntPoint& NodeLoc, const TArray<FExtAssetIdentifier>& NewIdentifiers, const FExtAssetData& InAssetData);
	void SetReferenceNodeCollapsed(const FIntPoint& NodeLoc, int32 InNumReferencesExceedingMax);
	void AddReferencer(class UEdGraphNode_ExtDependency* ReferencerNode);

	TArray<FExtAssetIdentifier> Identifiers;
	FText NodeTitle;

	bool bUsesThumbnail;
	bool bIsPackage;
	bool bIsPrimaryAsset;
	bool bIsCollapsed;
	bool bIsMissingOrInvalid;
	FExtAssetData CachedAssetData;

	UEdGraphPin* DependencyPin;
	UEdGraphPin* ReferencerPin;

	friend UEdGraph_ExtDependencyViewer;
};



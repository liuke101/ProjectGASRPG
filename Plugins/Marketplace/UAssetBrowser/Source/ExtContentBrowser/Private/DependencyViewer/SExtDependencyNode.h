// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphNode.h"
#include "EdGraphUtilities.h"

class FExtAssetThumbnail;
class UEdGraphNode_ExtDependency;

/**
 * A GraphNode representing an ext reference node
 */
class SExtDependencyNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS( SExtDependencyNode ){}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, UEdGraphNode_ExtDependency* InNode );

	// SGraphNode implementation
	virtual void UpdateGraphNode() override;
	virtual bool IsNodeEditable() const override { return false; }
	// End SGraphNode implementation

private:
	FSlateColor GetNodeTitleBackgroundColor() const;
	FSlateColor GetNodeOverlayColor() const;
	FSlateColor GetNodeBodyBackgroundColor() const;

private:
	TSharedPtr<class FExtAssetThumbnail> AssetThumbnail;
};

/**
 * SExtDependencyNode Factory
 */
class FExtDependencyGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};



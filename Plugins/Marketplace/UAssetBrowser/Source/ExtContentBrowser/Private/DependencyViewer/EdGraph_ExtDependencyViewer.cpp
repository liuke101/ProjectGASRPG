// Copyright 2017-2021 marynate. All Rights Reserved.

#include "EdGraph_ExtDependencyViewer.h"
#include "EdGraphNode_ExtDependency.h"
#include "ExtContentBrowserSingleton.h"
#include "ExtAssetData.h"
#include "ExtAssetThumbnail.h"

#include "EdGraph/EdGraphPin.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetThumbnail.h"
#include "SExtDependencyViewer.h"
#include "SExtDependencyNode.h"
#include "GraphEditor.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"
#include "Engine/AssetManager.h"
#include "Logging/TokenizedMessage.h"

UEdGraph_ExtDependencyViewer::UEdGraph_ExtDependencyViewer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssetThumbnailPool = MakeShareable( new FExtAssetThumbnailPool(1024) );

	MaxSearchDepth = 1;
	MaxSearchBreadth = 15;

	bLimitSearchDepth = true;
	bLimitSearchBreadth = false;
	bIsShowSoftReferences = true;
	bIsShowHardReferences = true;
	bIsShowManagementReferences = false;
	bIsShowSearchableNames = false;
	bIsShowNativePackages = true;

	NodeXSpacing = 400.f;
}

void UEdGraph_ExtDependencyViewer::BeginDestroy()
{
	AssetThumbnailPool.Reset();

	Super::BeginDestroy();
}

void UEdGraph_ExtDependencyViewer::SetGraphRoot(const TArray<FExtAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin)
{
	CurrentGraphRootIdentifiers = GraphRootIdentifiers;
	CurrentGraphRootOrigin = GraphRootOrigin;

	// If we're focused on a searchable name, enable that flag
	for (const FExtAssetIdentifier& AssetId : GraphRootIdentifiers)
	{
		if (AssetId.IsValue())
		{
			bIsShowSearchableNames = true;
		}
		else if (AssetId.GetPrimaryAssetId().IsValid())
		{
			if (UAssetManager::IsValid())
			{
				UAssetManager::Get().UpdateManagementDatabase();
			}
			
			bIsShowManagementReferences = true;
		}
	}
}

const TArray<FExtAssetIdentifier>& UEdGraph_ExtDependencyViewer::GetCurrentGraphRootIdentifiers() const
{
	return CurrentGraphRootIdentifiers;
}

void UEdGraph_ExtDependencyViewer::SetReferenceViewer(TSharedPtr<SExtDependencyViewer> InViewer)
{
	ReferenceViewer = InViewer;
}

bool UEdGraph_ExtDependencyViewer::GetSelectedAssetsForMenuExtender(const class UEdGraphNode* Node, TArray<FExtAssetIdentifier>& SelectedAssets) const
{
	if (!ReferenceViewer.IsValid())
	{
		return false;
	}
	TSharedPtr<SGraphEditor> GraphEditor = ReferenceViewer.Pin()->GetGraphEditor();

	if (!GraphEditor.IsValid())
	{
		return false;
	}

	TSet<UObject*> SelectedNodes = GraphEditor->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		if (UEdGraphNode_ExtDependency* ReferenceNode = Cast<UEdGraphNode_ExtDependency>(*It))
		{
			if (!ReferenceNode->IsCollapsed())
			{
				SelectedAssets.Add(ReferenceNode->GetIdentifier());
			}
		}
	}
	return true;
}

UEdGraphNode_ExtDependency* UEdGraph_ExtDependencyViewer::RebuildGraph()
{
	RemoveAllNodes();
	UEdGraphNode_ExtDependency* NewRootNode = ConstructNodes(CurrentGraphRootIdentifiers, CurrentGraphRootOrigin);
	NotifyGraphChanged();

	return NewRootNode;
}

bool UEdGraph_ExtDependencyViewer::IsSearchDepthLimited() const
{
	return bLimitSearchDepth;
}

bool UEdGraph_ExtDependencyViewer::IsSearchBreadthLimited() const
{
	return bLimitSearchBreadth;
}

bool UEdGraph_ExtDependencyViewer::IsShowSoftReferences() const
{
	return bIsShowSoftReferences;
}

bool UEdGraph_ExtDependencyViewer::IsShowHardReferences() const
{
	return bIsShowHardReferences;
}

bool UEdGraph_ExtDependencyViewer::IsShowManagementReferences() const
{
	return bIsShowManagementReferences;
}

bool UEdGraph_ExtDependencyViewer::IsShowSearchableNames() const
{
	return bIsShowSearchableNames;
}

bool UEdGraph_ExtDependencyViewer::IsShowNativePackages() const
{
	return bIsShowNativePackages;
}

void UEdGraph_ExtDependencyViewer::SetSearchDepthLimitEnabled(bool newEnabled)
{
	bLimitSearchDepth = newEnabled;
}

void UEdGraph_ExtDependencyViewer::SetSearchBreadthLimitEnabled(bool newEnabled)
{
	bLimitSearchBreadth = newEnabled;
}

void UEdGraph_ExtDependencyViewer::SetShowSoftReferencesEnabled(bool newEnabled)
{
	bIsShowSoftReferences = newEnabled;
}

void UEdGraph_ExtDependencyViewer::SetShowHardReferencesEnabled(bool newEnabled)
{
	bIsShowHardReferences = newEnabled;
}

void UEdGraph_ExtDependencyViewer::SetShowManagementReferencesEnabled(bool newEnabled)
{
	bIsShowManagementReferences = newEnabled;
}

void UEdGraph_ExtDependencyViewer::SetShowSearchableNames(bool newEnabled)
{
	bIsShowSearchableNames = newEnabled;
}

void UEdGraph_ExtDependencyViewer::SetShowNativePackages(bool newEnabled)
{
	bIsShowNativePackages = newEnabled;
}

int32 UEdGraph_ExtDependencyViewer::GetSearchDepthLimit() const
{
	return MaxSearchDepth;
}

int32 UEdGraph_ExtDependencyViewer::GetSearchBreadthLimit() const
{
	return MaxSearchBreadth;
}

void UEdGraph_ExtDependencyViewer::SetSearchDepthLimit(int32 NewDepthLimit)
{
	MaxSearchDepth = NewDepthLimit;
}

void UEdGraph_ExtDependencyViewer::SetSearchBreadthLimit(int32 NewBreadthLimit)
{
	MaxSearchBreadth = NewBreadthLimit;
}

FName UEdGraph_ExtDependencyViewer::GetCurrentCollectionFilter() const
{
	return CurrentCollectionFilter;
}

void UEdGraph_ExtDependencyViewer::SetCurrentCollectionFilter(FName NewFilter)
{
	CurrentCollectionFilter = NewFilter;
}

bool UEdGraph_ExtDependencyViewer::GetEnableCollectionFilter() const
{
	return bEnableCollectionFilter;
}

void UEdGraph_ExtDependencyViewer::SetEnableCollectionFilter(bool bEnabled)
{
	bEnableCollectionFilter = bEnabled;
}

float UEdGraph_ExtDependencyViewer::GetNodeXSpacing() const
{
	return NodeXSpacing;
}

void UEdGraph_ExtDependencyViewer::SetNodeXSpacing(float NewNodeXSpacing)
{
#if ECB_FEA_REF_VIEWER_NODE_SPACING
	NodeXSpacing = FMath::Clamp<float>(NewNodeXSpacing, 200.f, 10000.f);
#endif
}

EAssetRegistryDependencyType::Type UEdGraph_ExtDependencyViewer::GetReferenceSearchFlags(bool bHardOnly) const
{
	int32 ReferenceFlags = 0;

	if (bIsShowSoftReferences && !bHardOnly)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::Soft;
	}
	if (bIsShowHardReferences)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::Hard;
	}
	if (bIsShowSearchableNames && !bHardOnly)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::SearchableName;
	}
	if (bIsShowManagementReferences)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::HardManage;
		if (!bHardOnly)
		{
			ReferenceFlags |= EAssetRegistryDependencyType::SoftManage;
		}
	}
	
	return (EAssetRegistryDependencyType::Type)ReferenceFlags;
}

UEdGraphNode_ExtDependency* UEdGraph_ExtDependencyViewer::ConstructNodes(const TArray<FExtAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin )
{
	UEdGraphNode_ExtDependency* RootNode = NULL;

	if (GraphRootIdentifiers.Num() > 0 )
	{
		TSet<FName> AllowedPackageNames;
		if (ShouldFilterByCollection())
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
			TArray<FSoftObjectPath> AssetPaths;
			CollectionManagerModule.Get().GetAssetsInCollection(CurrentCollectionFilter, ECollectionShareType::CST_All, AssetPaths);
			AllowedPackageNames.Reserve(AssetPaths.Num());
			for (FSoftObjectPath& AssetPath : AssetPaths)
			{
				AllowedPackageNames.Add(FName(*FPackageName::ObjectPathToPackageName(AssetPath.ToString())));
			}
		}

		TMap<FExtAssetIdentifier, int32> ReferencerNodeSizes;
		TSet<FExtAssetIdentifier> VisitedReferencerSizeNames;
		int32 ReferencerDepth = 1;
		RecursivelyGatherSizes(/*bReferencers=*/true, GraphRootIdentifiers, AllowedPackageNames, ReferencerDepth, VisitedReferencerSizeNames, ReferencerNodeSizes);

		TMap<FExtAssetIdentifier, int32> DependencyNodeSizes;
		TSet<FExtAssetIdentifier> VisitedDependencySizeNames;
		int32 DependencyDepth = 1;
		RecursivelyGatherSizes(/*bReferencers=*/false, GraphRootIdentifiers, AllowedPackageNames, DependencyDepth, VisitedDependencySizeNames, DependencyNodeSizes);

		TSet<FName> AllPackageNames;

		auto AddPackage = [](const FExtAssetIdentifier& AssetId, TSet<FName>& PackageNames)
		{ 
			// Only look for asset data if this is a package
			if (!AssetId.IsValue())
			{
				PackageNames.Add(AssetId.PackageName);
			}
		};

		for (const FExtAssetIdentifier& AssetId : VisitedReferencerSizeNames)
		{
			AddPackage(AssetId, AllPackageNames);
		}

		for (const FExtAssetIdentifier& AssetId : VisitedDependencySizeNames)
		{
			AddPackage(AssetId, AllPackageNames);
		}

		TMap<FName, FExtAssetData> PackagesToAssetDataMap;
		GatherAssetData(AllPackageNames, PackagesToAssetDataMap);

		// Create the root node
		RootNode = CreateReferenceNode();
		RootNode->SetupReferenceNode(GraphRootOrigin, GraphRootIdentifiers, PackagesToAssetDataMap.FindRef(GraphRootIdentifiers[0].PackageName));

		TSet<FExtAssetIdentifier> VisitedReferencerNames;
		int32 VisitedReferencerDepth = 1;
		RecursivelyConstructNodes(/*bReferencers=*/true, RootNode, GraphRootIdentifiers, GraphRootOrigin, ReferencerNodeSizes, PackagesToAssetDataMap, AllowedPackageNames, VisitedReferencerDepth, VisitedReferencerNames);

		TSet<FExtAssetIdentifier> VisitedDependencyNames;
		int32 VisitedDependencyDepth = 1;
		RecursivelyConstructNodes(/*bReferencers=*/false, RootNode, GraphRootIdentifiers, GraphRootOrigin, DependencyNodeSizes, PackagesToAssetDataMap, AllowedPackageNames, VisitedDependencyDepth, VisitedDependencyNames);
	}

	return RootNode;
}

int32 UEdGraph_ExtDependencyViewer::RecursivelyGatherSizes(bool bReferencers, const TArray<FExtAssetIdentifier>& Identifiers, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FExtAssetIdentifier>& VisitedNames, TMap<FExtAssetIdentifier, int32>& OutNodeSizes) const
{
	check(Identifiers.Num() > 0);

	VisitedNames.Append(Identifiers);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FExtAssetIdentifier> ReferenceNames;

	if ( bReferencers )
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FExtContentBrowserSingleton::GetAssetRegistry().GetReferencers(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}
	else
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FExtContentBrowserSingleton::GetAssetRegistry().GetDependencies(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}

	if (!bIsShowNativePackages)
	{
		auto RemoveNativePackage = [](const FExtAssetIdentifier& InAsset) { return InAsset.PackageName.ToString().StartsWith(TEXT("/Script")) && !InAsset.IsValue(); };

		ReferenceNames.RemoveAll(RemoveNativePackage);
	}

	int32 NodeSize = 0;
	if ( ReferenceNames.Num() > 0 && !ExceedsMaxSearchDepth(CurrentDepth) )
	{
		int32 NumReferencesMade = 0;
		int32 NumReferencesExceedingMax = 0;
#if ECB_LEGACY
		// Filter for our registry source
		IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(ReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
#endif
		// Since there are referencers, use the size of all your combined referencers.
		// Do not count your own size since there could just be a horizontal line of nodes
		for (FExtAssetIdentifier& AssetId : ReferenceNames)
		{
			if ( !VisitedNames.Contains(AssetId) && (!AssetId.IsPackage() || !ShouldFilterByCollection() || AllowedPackageNames.Contains(AssetId.PackageName)) )
			{
				if ( !ExceedsMaxSearchBreadth(NumReferencesMade) )
				{
					TArray<FExtAssetIdentifier> NewPackageNames;
					NewPackageNames.Add(AssetId);
					NodeSize += RecursivelyGatherSizes(bReferencers, NewPackageNames, AllowedPackageNames, CurrentDepth + 1, VisitedNames, OutNodeSizes);
					NumReferencesMade++;
				}
				else
				{
					NumReferencesExceedingMax++;
				}
			}
		}

		if ( NumReferencesExceedingMax > 0 )
		{
			// Add one size for the collapsed node
			NodeSize++;
		}
	}

	if ( NodeSize == 0 )
	{
		// If you have no valid children, the node size is just 1 (counting only self to make a straight line)
		NodeSize = 1;
	}

	OutNodeSizes.Add(Identifiers[0], NodeSize);
	return NodeSize;
}

void UEdGraph_ExtDependencyViewer::GatherAssetData(const TSet<FName>& AllPackageNames, TMap<FName, FExtAssetData>& OutPackageToAssetDataMap) const
{
	FARFilter Filter;
	for ( auto PackageIt = AllPackageNames.CreateConstIterator(); PackageIt; ++PackageIt )
	{
		const FString& PackageName = (*PackageIt).ToString();
		//const FString& PackagePath = PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
		//Filter.ObjectPaths.Add( FName(*PackagePath) );
		Filter.PackageNames.Add(FName(*PackageName));
	}

	TArray<FExtAssetData> AssetDataList;
	FExtContentBrowserSingleton::GetAssetRegistry().GetAssets(Filter, AssetDataList);
	for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		OutPackageToAssetDataMap.Add((*AssetIt).PackageName, *AssetIt);
	}
}

UEdGraphNode_ExtDependency* UEdGraph_ExtDependencyViewer::RecursivelyConstructNodes(bool bReferencers, UEdGraphNode_ExtDependency* RootNode, const TArray<FExtAssetIdentifier>& Identifiers, const FIntPoint& NodeLoc, const TMap<FExtAssetIdentifier, int32>& NodeSizes, const TMap<FName, FExtAssetData>& PackagesToAssetDataMap, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FExtAssetIdentifier>& VisitedNames)
{
	check(Identifiers.Num() > 0);

	VisitedNames.Append(Identifiers);

	UEdGraphNode_ExtDependency* NewNode = NULL;
	if ( RootNode->GetIdentifier() == Identifiers[0] )
	{
		// Don't create the root node. It is already created!
		NewNode = RootNode;
	}
	else
	{
		NewNode = CreateReferenceNode();
		NewNode->SetupReferenceNode(NodeLoc, Identifiers, PackagesToAssetDataMap.FindRef(Identifiers[0].PackageName));
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FExtAssetIdentifier> ReferenceNames;
	TArray<FExtAssetIdentifier> HardReferenceNames;
	if ( bReferencers )
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FExtContentBrowserSingleton::GetAssetRegistry().GetReferencers(AssetId, HardReferenceNames, GetReferenceSearchFlags(true));
			FExtContentBrowserSingleton::GetAssetRegistry().GetReferencers(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}
	else
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FExtContentBrowserSingleton::GetAssetRegistry().GetDependencies(AssetId, HardReferenceNames, GetReferenceSearchFlags(true));
			FExtContentBrowserSingleton::GetAssetRegistry().GetDependencies(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}

	if (!bIsShowNativePackages)
	{
		auto RemoveNativePackage = [](const FExtAssetIdentifier& InAsset) { return InAsset.PackageName.ToString().StartsWith(TEXT("/Script")) && !InAsset.IsValue(); };

		HardReferenceNames.RemoveAll(RemoveNativePackage);
		ReferenceNames.RemoveAll(RemoveNativePackage);
	}

	if ( ReferenceNames.Num() > 0 && !ExceedsMaxSearchDepth(CurrentDepth) )
	{
		FIntPoint ReferenceNodeLoc = NodeLoc;

		if ( bReferencers )
		{
			// Referencers go left
			ReferenceNodeLoc.X -= NodeXSpacing;
		}
		else
		{
			// Dependencies go right
			ReferenceNodeLoc.X += NodeXSpacing;
		}

		const int32 NodeSizeY = 200;
		const int32 TotalReferenceSizeY = NodeSizes.FindChecked(Identifiers[0]) * NodeSizeY;

		ReferenceNodeLoc.Y -= TotalReferenceSizeY * 0.5f;
		ReferenceNodeLoc.Y += NodeSizeY * 0.5f;

		int32 NumReferencesMade = 0;
		int32 NumReferencesExceedingMax = 0;

#if ECB_LEGACY
		// Filter for our registry source
		IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(ReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
		IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(HardReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
#endif

		for ( int32 RefIdx = 0; RefIdx < ReferenceNames.Num(); ++RefIdx )
		{
			FExtAssetIdentifier ReferenceName = ReferenceNames[RefIdx];

			if ( !VisitedNames.Contains(ReferenceName) && (!ReferenceName.IsPackage() || !ShouldFilterByCollection() || AllowedPackageNames.Contains(ReferenceName.PackageName)) )
			{
				bool bIsHardReference = HardReferenceNames.Contains(ReferenceName);

				if ( !ExceedsMaxSearchBreadth(NumReferencesMade) )
				{
					int32 ThisNodeSizeY = ReferenceName.IsValue() ? 100 : NodeSizeY;

					const int32 RefSizeY = NodeSizes.FindChecked(ReferenceName);
					FIntPoint RefNodeLoc;
					RefNodeLoc.X = ReferenceNodeLoc.X;
					RefNodeLoc.Y = ReferenceNodeLoc.Y + RefSizeY * ThisNodeSizeY * 0.5 - ThisNodeSizeY * 0.5;
					
					TArray<FExtAssetIdentifier> NewIdentifiers;
					NewIdentifiers.Add(ReferenceName);
					
					UEdGraphNode_ExtDependency* ReferenceNode = RecursivelyConstructNodes(bReferencers, RootNode, NewIdentifiers, RefNodeLoc, NodeSizes, PackagesToAssetDataMap, AllowedPackageNames, CurrentDepth + 1, VisitedNames);
					if (bIsHardReference)
					{
						if (bReferencers)
						{
							ReferenceNode->GetDependencyPin()->PinType.PinCategory = TEXT("hard");
						}
						else
						{
							ReferenceNode->GetReferencerPin()->PinType.PinCategory = TEXT("hard"); //-V595
						}
					}

					bool bIsMissingOrInvalid = ReferenceNode->IsMissingOrInvalid();
					if (bIsMissingOrInvalid)
					{
						if (bReferencers)
						{
							ReferenceNode->GetDependencyPin()->PinType.PinSubCategory = TEXT("invalid");
						}
						else
						{
							ReferenceNode->GetReferencerPin()->PinType.PinSubCategory = TEXT("invalid");
						}

						ReferenceNode->bHasCompilerMessage = true;
						if (!bIsHardReference)
						{
							ReferenceNode->ErrorType = EMessageSeverity::Warning;
						}
						else
						{
							ReferenceNode->ErrorType = EMessageSeverity::Error;
						}
					}
					
					if ( ensure(ReferenceNode) )
					{
						if ( bReferencers )
						{
							NewNode->AddReferencer( ReferenceNode );
						}
						else
						{
							ReferenceNode->AddReferencer( NewNode );
						}

						ReferenceNodeLoc.Y += RefSizeY * ThisNodeSizeY;
					}

					NumReferencesMade++;
				}
				else
				{
					NumReferencesExceedingMax++;
				}
			}
		}

		if ( NumReferencesExceedingMax > 0 )
		{
			// There are more references than allowed to be displayed. Make a collapsed node.
			UEdGraphNode_ExtDependency* ReferenceNode = CreateReferenceNode();
			FIntPoint RefNodeLoc;
			RefNodeLoc.X = ReferenceNodeLoc.X;
			RefNodeLoc.Y = ReferenceNodeLoc.Y;

			if ( ensure(ReferenceNode) )
			{
				ReferenceNode->SetReferenceNodeCollapsed(RefNodeLoc, NumReferencesExceedingMax);

				if ( bReferencers )
				{
					NewNode->AddReferencer( ReferenceNode );
				}
				else
				{
					ReferenceNode->AddReferencer( NewNode );
				}
			}
		}
	}

	return NewNode;
}

const TSharedPtr<FExtAssetThumbnailPool>& UEdGraph_ExtDependencyViewer::GetAssetThumbnailPool() const
{
	return AssetThumbnailPool;
}

bool UEdGraph_ExtDependencyViewer::ExceedsMaxSearchDepth(int32 Depth) const
{
	return bLimitSearchDepth && Depth > MaxSearchDepth;
}

bool UEdGraph_ExtDependencyViewer::ExceedsMaxSearchBreadth(int32 Breadth) const
{
	return bLimitSearchBreadth && Breadth > MaxSearchBreadth;
}

UEdGraphNode_ExtDependency* UEdGraph_ExtDependencyViewer::CreateReferenceNode()
{
	const bool bSelectNewNode = false;
	return Cast<UEdGraphNode_ExtDependency>(CreateNode(UEdGraphNode_ExtDependency::StaticClass(), bSelectNewNode));
}

void UEdGraph_ExtDependencyViewer::RemoveAllNodes()
{
	TArray<UEdGraphNode*> NodesToRemove = Nodes;
	for (int32 NodeIndex = 0; NodeIndex < NodesToRemove.Num(); ++NodeIndex)
	{
		RemoveNode(NodesToRemove[NodeIndex]);
	}
}

bool UEdGraph_ExtDependencyViewer::ShouldFilterByCollection() const
{
	return bEnableCollectionFilter && CurrentCollectionFilter != NAME_None;
}

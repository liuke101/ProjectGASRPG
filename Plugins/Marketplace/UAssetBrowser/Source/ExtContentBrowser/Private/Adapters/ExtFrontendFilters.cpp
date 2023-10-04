// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtFrontendFilters.h"

#include "IExtContentBrowserSingleton.h"
#include "ExtContentBrowserSettings.h"
#include "SExtAssetView.h"

#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Misc/ConfigCacheIni.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "Editor.h"
#include "AssetToolsModule.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "MRUFavoritesList.h"
#include "HAL/FileManager.h"

/** Helper functions for frontend filters */
namespace FrontendFilterHelper
{
	/**
	 * Get a set of dependencies as package name's from a list of assets found with the given Asset Registry Filter.
	 * @param InAssetRegistryFilter		The filter to find assets for in the asset registry.
	 * @param AssetRegistry				The Asset Registry to find assets and dependencies.
	 * @param OutDependencySet			The output of dependencies found from a set of assets.
	 */
	void GetDependencies(const FARFilter& InAssetRegistryFilter, const IAssetRegistry& AssetRegistry, TSet<FName>& OutDependencySet)
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssets(InAssetRegistryFilter, FoundAssets);

		for (const FAssetData& AssetData : FoundAssets)
		{
			// Store all the dependencies of all the levels
			TArray<FExtAssetIdentifier> AssetDependencies;
			//AssetRegistry.GetDependencies(FExtAssetIdentifier(AssetData.PackageName), AssetDependencies);
			for (const FExtAssetIdentifier& Dependency : AssetDependencies)
			{
				OutDependencySet.Add(Dependency.PackageName);
			}
		}
	}
}

/////////////////////////////////////////
// FFrontendFilter_Text
/////////////////////////////////////////

/** Mapping of asset property tag aliases that can be used by text searches */
class FFrontendFilter_AssetPropertyTagAliases
{
public:
	static FFrontendFilter_AssetPropertyTagAliases& Get()
	{
		static FFrontendFilter_AssetPropertyTagAliases Singleton;
		return Singleton;
	}

	/** Get the source tag for the given asset data and alias, or none if there is no match */
	FName GetSourceTagFromAlias(const FExtAssetData& InAssetData, const FName InAlias)
	{
		TSharedPtr<TMap<FName, FName>>& AliasToSourceTagMapping = ClassToAliasTagsMapping.FindOrAdd(InAssetData.AssetClass);

		if (!AliasToSourceTagMapping.IsValid())
		{
			static const FName NAME_DisplayName(TEXT("DisplayName"));

			AliasToSourceTagMapping = MakeShared<TMap<FName, FName>>();

			UClass* AssetClass = InAssetData.GetClass();
			if (AssetClass)
			{
				TMap<FName, UObject::FAssetRegistryTagMetadata> AssetTagMetaData;
				AssetClass->GetDefaultObject()->GetAssetRegistryTagMetadata(AssetTagMetaData);

				for (const auto& AssetTagMetaDataPair : AssetTagMetaData)
				{
					if (!AssetTagMetaDataPair.Value.DisplayName.IsEmpty())
					{
						const FName DisplayName = MakeObjectNameFromDisplayLabel(AssetTagMetaDataPair.Value.DisplayName.ToString(), NAME_None);
						AliasToSourceTagMapping->Add(DisplayName, AssetTagMetaDataPair.Key);
					}
				}

				for (const auto& KeyValuePair : InAssetData.TagsAndValues)
				{
					if (FProperty* Field = FindFProperty<FProperty>(AssetClass, KeyValuePair.Key))
					{
						if (Field->HasMetaData(NAME_DisplayName))
						{
							const FName DisplayName = MakeObjectNameFromDisplayLabel(Field->GetMetaData(NAME_DisplayName), NAME_None);
							AliasToSourceTagMapping->Add(DisplayName, KeyValuePair.Key);
						}
					}
				}
			}
		}

		return AliasToSourceTagMapping.IsValid() ? AliasToSourceTagMapping->FindRef(InAlias) : NAME_None;
	}

private:
	/** Mapping from class name -> (alias -> source) */
	TMap<FName, TSharedPtr<TMap<FName, FName>>> ClassToAliasTagsMapping;
};

/** Expression context which gathers up the names of any dynamic collections being referenced by the current query */
class FFrontendFilter_GatherDynamicCollectionsExpressionContext : public ITextFilterExpressionContext
{
public:
	FFrontendFilter_GatherDynamicCollectionsExpressionContext(TArray<FCollectionNameType>& OutReferencedDynamicCollections)
		: AvailableDynamicCollections()
		, ReferencedDynamicCollections(OutReferencedDynamicCollections)
		, CurrentRecursionDepth(0)
		, CollectionKeyName("Collection")
		, TagKeyName("Tag")
	{
		if (FCollectionManagerModule::IsModuleAvailable())
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			TArray<FCollectionNameType> AvailableCollections;
			CollectionManagerModule.Get().GetCollections(AvailableCollections);

			for (const FCollectionNameType& AvailableCollection : AvailableCollections)
			{
				// Only care about dynamic collections
				ECollectionStorageMode::Type StorageMode = ECollectionStorageMode::Static;
				CollectionManagerModule.Get().GetCollectionStorageMode(AvailableCollection.Name, AvailableCollection.Type, StorageMode);
				if (StorageMode != ECollectionStorageMode::Dynamic)
				{
					continue;
				}

				AvailableDynamicCollections.Add(AvailableCollection);
			}
		}
	}

	~FFrontendFilter_GatherDynamicCollectionsExpressionContext()
	{
		// Sort and populate the final list of referenced dynamic collections
		FoundDynamicCollections.Sort([](const FDynamicCollectionNameAndDepth& A, const FDynamicCollectionNameAndDepth& B)
		{
			return A.RecursionDepth > B.RecursionDepth;
		});

		ReferencedDynamicCollections.Reset();
		ReferencedDynamicCollections.Reserve(FoundDynamicCollections.Num());
		for (const auto& FoundDynamicCollection : FoundDynamicCollections)
		{
			ReferencedDynamicCollections.Add(FoundDynamicCollection.Collection);
		}
	}

	virtual bool TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const override
	{
		TestAgainstAvailableCollections(InValue, InTextComparisonMode);
		return false;
	}

	virtual bool TestComplexExpression(const FName& InKey, const FTextFilterString& InValue, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const override
	{
		// Special case for collections, as these aren't contained within the asset registry meta-data
		if (InKey == CollectionKeyName || InKey == TagKeyName)
		{
			// Collections can only work with Equal or NotEqual type tests
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			TestAgainstAvailableCollections(InValue, InTextComparisonMode);
		}

		return false;
	}

private:
	bool TestAgainstAvailableCollections(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const
	{
		for (const FCollectionNameType& DynamicCollection : AvailableDynamicCollections)
		{
			const FString DynamicCollectionNameStr = DynamicCollection.Name.ToString();
			if (TextFilterUtils::TestBasicStringExpression(DynamicCollectionNameStr, InValue, InTextComparisonMode))
			{
				const bool bCollectionAlreadyProcessed = FoundDynamicCollections.ContainsByPredicate([&](const FDynamicCollectionNameAndDepth& Other)
				{
					return DynamicCollection == Other.Collection;
				});

				if (!bCollectionAlreadyProcessed)
				{
					FoundDynamicCollections.Add(FDynamicCollectionNameAndDepth(DynamicCollection, CurrentRecursionDepth));

					if (FCollectionManagerModule::IsModuleAvailable())
					{
						FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
						
						// Also need to gather any collections referenced by this dynamic collection
						++CurrentRecursionDepth;
						bool bUnused = false;
						CollectionManagerModule.Get().TestDynamicQuery(DynamicCollection.Name, DynamicCollection.Type, *this, bUnused);
						--CurrentRecursionDepth;
					}
				}

				return true;
			}
		}

		return false;
	}

	/** Contains a collection name along with its recursion depth in the dynamic query - used so we can test them depth first */
	struct FDynamicCollectionNameAndDepth
	{
		FDynamicCollectionNameAndDepth(FCollectionNameType InCollection, const int32 InRecursionDepth)
			: Collection(InCollection)
			, RecursionDepth(InRecursionDepth)
		{
		}

		FCollectionNameType Collection;
		int32 RecursionDepth;
	};

	/** The currently available dynamic collections */
	TArray<FCollectionNameType> AvailableDynamicCollections;

	/** This will be populated with any dynamic collections that are being referenced by the current query - these collections may not all match when tested against the actual asset data */
	TArray<FCollectionNameType>& ReferencedDynamicCollections;

	/** Dynamic collections that have currently be found as part of the query (or recursive sub-query) */
	mutable TArray<FDynamicCollectionNameAndDepth> FoundDynamicCollections;

	/** Incremented when we test a sub-query, decremented once we're done */
	mutable int32 CurrentRecursionDepth;

	/** Keys used by TestComplexExpression */
	const FName CollectionKeyName;
	const FName TagKeyName;
};

/** Expression context to test the given asset data against the current text filter */
class FFrontendFilter_TextFilterExpressionContext : public ITextFilterExpressionContext
{
public:
	typedef TRemoveReference<FExtAssetFilterType>::Type* FExtAssetFilterTypePtr;

	FFrontendFilter_TextFilterExpressionContext(const TArray<FCollectionNameType>& InReferencedDynamicCollections)
		: ReferencedDynamicCollections(InReferencedDynamicCollections)
		, AssetPtr(nullptr)
		, bIncludeClassName(true)
		, bIncludeAssetPath(false)
		, bIncludeCollectionNames(true)
		, NameKeyName("Name")
		, PathKeyName("Path")
		, ClassKeyName("Class")
		, TypeKeyName("Type")
		, CollectionKeyName("Collection")
		, TagKeyName("Tag")
		, CollectionManager(nullptr)
	{
	}

	void SetAsset(FExtAssetFilterTypePtr InAsset)
	{
		AssetPtr = InAsset;

		if (bIncludeAssetPath)
		{
			// Get the full asset path, and also split it so we can compare each part in the filter
			AssetPtr->PackageName.AppendString(AssetFullPath);
			AssetFullPath.ParseIntoArray(AssetSplitPath, TEXT("/"));
			AssetFullPath.ToUpperInline();
#if ECB_LEGACY
			if (bIncludeClassName)
			{
				// Get the full export text path as people sometimes search by copying this (requires class and asset path search to be enabled in order to match)
				AssetPtr->GetExportTextName(AssetExportTextName);
				AssetExportTextName.ToUpperInline();
			}
#endif
		}

		if (CollectionManager == nullptr)
		{
			CollectionManager = &FCollectionManagerModule::GetModule().Get();
		}

		if (CollectionManager)
		{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
			CollectionManager->GetCollectionsContainingObject(AssetPtr->ObjectPath, ECollectionShareType::CST_All, AssetCollectionNames, ECollectionRecursionFlags::SelfAndChildren);
PRAGMA_ENABLE_DEPRECATION_WARNINGS

			// Test the dynamic collections from the active query against the current asset
			// We can do this as a flat list since FFrontendFilter_GatherDynamicCollectionsExpressionContext has already taken care of processing the recursion
			for (const FCollectionNameType& DynamicCollection : ReferencedDynamicCollections)
			{
				bool bPassesCollectionFilter = false;
				CollectionManager->TestDynamicQuery(DynamicCollection.Name, DynamicCollection.Type, *this, bPassesCollectionFilter);
				if (bPassesCollectionFilter)
				{
					AssetCollectionNames.AddUnique(DynamicCollection.Name);
				}
			}
		}
	}

	void ClearAsset()
	{
		AssetPtr = nullptr;
		AssetFullPath.Reset();
		AssetExportTextName.Reset();
		AssetSplitPath.Reset();
		AssetCollectionNames.Reset();
	}

	void SetIncludeClassName(const bool InIncludeClassName)
	{
		bIncludeClassName = InIncludeClassName;
	}

	bool GetIncludeClassName() const
	{
		return bIncludeClassName;
	}

	void SetIncludeAssetPath(const bool InIncludeAssetPath)
	{
		bIncludeAssetPath = InIncludeAssetPath;
	}

	bool GetIncludeAssetPath() const
	{
		return bIncludeAssetPath;
	}

	void SetIncludeCollectionNames(const bool InIncludeCollectionNames)
	{
		bIncludeCollectionNames = InIncludeCollectionNames;
	}

	bool GetIncludeCollectionNames() const
	{
		return bIncludeCollectionNames;
	}

	virtual bool TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const override
	{
		if (InValue.CompareName(AssetPtr->AssetName, InTextComparisonMode))
		{
			return true;
		}

		if (bIncludeAssetPath)
		{
			if (InValue.CompareFString(AssetFullPath, InTextComparisonMode))
			{
				return true;
			}

			for (const FString& AssetPathPart : AssetSplitPath)
			{
				if (InValue.CompareFString(AssetPathPart, InTextComparisonMode))
				{
					return true;
				}
			}
		}

		if (bIncludeClassName)
		{
			if (InValue.CompareName(AssetPtr->AssetClass, InTextComparisonMode))
			{
				return true;
			}
		}

		if (bIncludeClassName && bIncludeAssetPath)
		{
			// Only test this if we're searching the class name and asset path too, as the exported text contains the type and path in the string
			if (InValue.CompareFString(AssetExportTextName, InTextComparisonMode))
			{
				return true;
			}
		}

		if (bIncludeCollectionNames)
		{
			for (const FName& AssetCollectionName : AssetCollectionNames)
			{
				if (InValue.CompareName(AssetCollectionName, InTextComparisonMode))
				{
					return true;
				}
			}
		}

		return false;
	}

	virtual bool TestComplexExpression(const FName& InKey, const FTextFilterString& InValue, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const override
	{
		// Special case for the asset name, as this isn't contained within the asset registry meta-data
		if (InKey == NameKeyName)
		{
			// Names can only work with Equal or NotEqual type tests
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			const bool bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->AssetName, InValue, InTextComparisonMode);
			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bIsMatch : !bIsMatch;
		}

		// Special case for the asset path, as this isn't contained within the asset registry meta-data
		if (InKey == PathKeyName)
		{
			// Paths can only work with Equal or NotEqual type tests
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			// If the comparison mode is partial, then we only need to test the ObjectPath as that contains the other two as sub-strings
			bool bIsMatch = false;
			if (InTextComparisonMode == ETextFilterTextComparisonMode::Partial)
			{
				bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->ObjectPath, InValue, InTextComparisonMode);
			}
			else
			{
				bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->ObjectPath, InValue, InTextComparisonMode)
					|| TextFilterUtils::TestBasicStringExpression(AssetPtr->PackageName, InValue, InTextComparisonMode)
					|| TextFilterUtils::TestBasicStringExpression(AssetPtr->PackagePath, InValue, InTextComparisonMode);
			}
			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bIsMatch : !bIsMatch;
		}

		// Special case for the asset type, as this isn't contained within the asset registry meta-data
		if (InKey == ClassKeyName || InKey == TypeKeyName)
		{
			// Class names can only work with Equal or NotEqual type tests
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			const bool bIsMatch = TextFilterUtils::TestBasicStringExpression(AssetPtr->AssetClass, InValue, InTextComparisonMode);
			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bIsMatch : !bIsMatch;
		}

		// Special case for collections, as these aren't contained within the asset registry meta-data
		if (InKey == CollectionKeyName || InKey == TagKeyName)
		{
			// Collections can only work with Equal or NotEqual type tests
			if (InComparisonOperation != ETextFilterComparisonOperation::Equal && InComparisonOperation != ETextFilterComparisonOperation::NotEqual)
			{
				return false;
			}

			bool bFoundMatch = false;
			for (const FName& AssetCollectionName : AssetCollectionNames)
			{
				if (TextFilterUtils::TestBasicStringExpression(AssetCollectionName, InValue, InTextComparisonMode))
				{
					bFoundMatch = true;
					break;
				}
			}

			return (InComparisonOperation == ETextFilterComparisonOperation::Equal) ? bFoundMatch : !bFoundMatch;
		}

		// Generic handling for anything in the asset meta-data
		{
			auto GetMetaDataValue = [this, &InKey](FString& OutMetaDataValue) -> bool
			{
				// Check for a literal key
				if (AssetPtr->GetTagValue(InKey, OutMetaDataValue))
				{
					return true;
				}

				// Check for an alias key
				const FName LiteralKey = FFrontendFilter_AssetPropertyTagAliases::Get().GetSourceTagFromAlias(*AssetPtr, InKey);
				if (!LiteralKey.IsNone() && AssetPtr->GetTagValue(LiteralKey, OutMetaDataValue))
				{
					return true;
				}

				return false;
			};

			FString MetaDataValue;
			if (GetMetaDataValue(MetaDataValue))
			{
				return TextFilterUtils::TestComplexExpression(MetaDataValue, InValue, InComparisonOperation, InTextComparisonMode);
			}
		}

		return false;
	}

private:
	/** An array of dynamic collections that are being referenced by the current query. These should be tested against each asset when it's looking for collections that contain it */
	const TArray<FCollectionNameType>& ReferencedDynamicCollections;

	/** Pointer to the asset we're currently filtering */
	FExtAssetFilterTypePtr AssetPtr;

	/** Full path of the current asset */
	FString AssetFullPath;

	/** The export text name of the current asset */
	FString AssetExportTextName;

	/** Split path of the current asset */
	TArray<FString> AssetSplitPath;

	/** Names of the collections that the current asset is in */
	TArray<FName> AssetCollectionNames;

	/** Are we supposed to include the class name in our basic string tests? */
	bool bIncludeClassName;

	/** Search inside the entire asset path? */
	bool bIncludeAssetPath;

	/** Search collection names? */
	bool bIncludeCollectionNames;

	/** Keys used by TestComplexExpression */
	const FName NameKeyName;
	const FName PathKeyName;
	const FName ClassKeyName;
	const FName TypeKeyName;
	const FName CollectionKeyName;
	const FName TagKeyName;

	/** Cached Collection manager */
	ICollectionManager* CollectionManager;
};

FFrontendFilter_Text::FFrontendFilter_Text()
	: FExtFrontendFilter(nullptr)
	, ReferencedDynamicCollections()
	, TextFilterExpressionContext(MakeShareable(new FFrontendFilter_TextFilterExpressionContext(ReferencedDynamicCollections)))
	, TextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::Complex)
{
	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

	// We need to watch for collection changes so that we can keep ReferencedDynamicCollections up-to-date
	OnCollectionCreatedHandle = CollectionManagerModule.Get().OnCollectionCreated().AddRaw(this, &FFrontendFilter_Text::HandleCollectionCreated);
	OnCollectionDestroyedHandle = CollectionManagerModule.Get().OnCollectionDestroyed().AddRaw(this, &FFrontendFilter_Text::HandleCollectionDestroyed);
	OnCollectionRenamedHandle = CollectionManagerModule.Get().OnCollectionRenamed().AddRaw(this, &FFrontendFilter_Text::HandleCollectionRenamed);
	OnCollectionUpdatedHandle = CollectionManagerModule.Get().OnCollectionUpdated().AddRaw(this, &FFrontendFilter_Text::HandleCollectionUpdated);
}

FFrontendFilter_Text::~FFrontendFilter_Text()
{
	// Check IsModuleAvailable as we might be in the process of shutting down...
	if (FCollectionManagerModule::IsModuleAvailable())
	{
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

		CollectionManagerModule.Get().OnCollectionCreated().Remove(OnCollectionCreatedHandle);
		CollectionManagerModule.Get().OnCollectionDestroyed().Remove(OnCollectionDestroyedHandle);
		CollectionManagerModule.Get().OnCollectionRenamed().Remove(OnCollectionRenamedHandle);
		CollectionManagerModule.Get().OnCollectionUpdated().Remove(OnCollectionUpdatedHandle);
	}
}

bool FFrontendFilter_Text::PassesFilter(FExtAssetFilterType InItem) const
{
	TextFilterExpressionContext->SetAsset(&InItem);
	const bool bMatched = TextFilterExpressionEvaluator.TestTextFilter(*TextFilterExpressionContext);
	TextFilterExpressionContext->ClearAsset();
	return bMatched;
}

FText FFrontendFilter_Text::GetRawFilterText() const
{
	return TextFilterExpressionEvaluator.GetFilterText();
}

void FFrontendFilter_Text::SetRawFilterText(const FText& InFilterText)
{
	if (TextFilterExpressionEvaluator.SetFilterText(InFilterText))
	{
		RebuildReferencedDynamicCollections();

		// Will trigger a re-filter with the new text
		BroadcastChangedEvent();
	}
}

FText FFrontendFilter_Text::GetFilterErrorText() const
{
	return TextFilterExpressionEvaluator.GetFilterErrorText();
}

void FFrontendFilter_Text::SetIncludeClassName(const bool InIncludeClassName)
{
	if (TextFilterExpressionContext->GetIncludeClassName() != InIncludeClassName)
	{
		TextFilterExpressionContext->SetIncludeClassName(InIncludeClassName);

		// Will trigger a re-filter with the new setting
		BroadcastChangedEvent();
	}
}

void FFrontendFilter_Text::SetIncludeAssetPath(const bool InIncludeAssetPath)
{
	if (TextFilterExpressionContext->GetIncludeAssetPath() != InIncludeAssetPath)
	{
		TextFilterExpressionContext->SetIncludeAssetPath(InIncludeAssetPath);

		// Will trigger a re-filter with the new setting
		BroadcastChangedEvent();
	}
}

bool FFrontendFilter_Text::GetIncludeAssetPath() const
{
	return TextFilterExpressionContext->GetIncludeAssetPath();
}

void FFrontendFilter_Text::SetIncludeCollectionNames(const bool InIncludeCollectionNames)
{
	if (TextFilterExpressionContext->GetIncludeCollectionNames() != InIncludeCollectionNames)
	{
		TextFilterExpressionContext->SetIncludeCollectionNames(InIncludeCollectionNames);

		// Will trigger a re-filter with the new collections
		BroadcastChangedEvent();
	}
}

bool FFrontendFilter_Text::GetIncludeCollectionNames() const
{
	return TextFilterExpressionContext->GetIncludeCollectionNames();
}

void FFrontendFilter_Text::HandleCollectionCreated(const FCollectionNameType& Collection)
{
	RebuildReferencedDynamicCollections();

	// Will trigger a re-filter with the new collections
	BroadcastChangedEvent();
}

void FFrontendFilter_Text::HandleCollectionDestroyed(const FCollectionNameType& Collection)
{
	if (ReferencedDynamicCollections.Contains(Collection))
	{
		RebuildReferencedDynamicCollections();

		// Will trigger a re-filter with the new collections
		BroadcastChangedEvent();
	}
}

void FFrontendFilter_Text::HandleCollectionRenamed(const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection)
{
	int32 FoundIndex = INDEX_NONE;
	if (ReferencedDynamicCollections.Find(OriginalCollection, FoundIndex))
	{
		ReferencedDynamicCollections[FoundIndex] = NewCollection;
	}
}

void FFrontendFilter_Text::HandleCollectionUpdated(const FCollectionNameType& Collection)
{
	RebuildReferencedDynamicCollections();

	// Will trigger a re-filter with the new collections
	BroadcastChangedEvent();
}

void FFrontendFilter_Text::RebuildReferencedDynamicCollections()
{
	TextFilterExpressionEvaluator.TestTextFilter(FFrontendFilter_GatherDynamicCollectionsExpressionContext(ReferencedDynamicCollections));
}

/////////////////////////////////////////
// FFrontendFilter_Modified
/////////////////////////////////////////

FFrontendFilter_Modified::FFrontendFilter_Modified(TSharedPtr<FExtFrontendFilterCategory> InCategory)
	: FExtFrontendFilter(InCategory)
	, bIsCurrentlyActive(false)
{
	UPackage::PackageDirtyStateChangedEvent.AddRaw(this, &FFrontendFilter_Modified::OnPackageDirtyStateUpdated);
}

FFrontendFilter_Modified::~FFrontendFilter_Modified()
{
	UPackage::PackageDirtyStateChangedEvent.RemoveAll(this);
}

void FFrontendFilter_Modified::ActiveStateChanged(bool bActive)
{
	bIsCurrentlyActive = bActive;
}

bool FFrontendFilter_Modified::PassesFilter(FExtAssetFilterType InItem) const
{
	return DirtyPackageNames.Contains(InItem.PackageName);
}

void FFrontendFilter_Modified::OnPackageDirtyStateUpdated(UPackage* Package)
{
	if (bIsCurrentlyActive)
	{
		BroadcastChangedEvent();
	}
}

void FFrontendFilter_Modified::SetCurrentFilter(const FARFilter& InBaseFilter)
{
	DirtyPackageNames.Reset();

	const UPackage* TransientPackage = GetTransientPackage();
	if (TransientPackage)
	{
		for (TObjectIterator<UPackage> PackageIter; PackageIter; ++PackageIter)
		{
			UPackage* CurPackage = *PackageIter;
			if (CurPackage && CurPackage->IsDirty())
			{
				DirtyPackageNames.Add(CurPackage->GetFName());
			}
		}
	}
}


/////////////////////////////////////////
// FFrontendFilter_ArbitraryComparisonOperation
/////////////////////////////////////////

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

FFrontendFilter_ArbitraryComparisonOperation::FFrontendFilter_ArbitraryComparisonOperation(TSharedPtr<FExtFrontendFilterCategory> InCategory)
	: FExtFrontendFilter(InCategory)
	, TagName(TEXT("TagName"))
	, TargetTagValue(TEXT("Value"))
	, ComparisonOp(ETextFilterComparisonOperation::NotEqual)
{
}

FString FFrontendFilter_ArbitraryComparisonOperation::GetName() const
{
	return TEXT("CompareTags");
}

FText FFrontendFilter_ArbitraryComparisonOperation::GetDisplayName() const
{
	return FText::Format(LOCTEXT("FFrontendFilter_CompareOperation", "Compare Tags ({0} {1} {2})"),
		FText::FromName(TagName),
		FText::AsCultureInvariant(ConvertOperationToString(ComparisonOp)),
		FText::AsCultureInvariant(TargetTagValue));
}

FText FFrontendFilter_ArbitraryComparisonOperation::GetToolTipText() const
{
	return LOCTEXT("FFrontendFilter_CompareOperation_ToolTip", "Compares AssetRegistrySearchable values on assets with a target value.");
}

bool FFrontendFilter_ArbitraryComparisonOperation::PassesFilter(FExtAssetFilterType InItem) const
{
	FString TagValue;
	if (InItem.GetTagValue(TagName, TagValue))
	{
		return TextFilterUtils::TestComplexExpression(TagValue, TargetTagValue, ComparisonOp, ETextFilterTextComparisonMode::Exact);
	}
	else
	{
		// Failed to find the tag, can't pass the filter
		//@TODO: Maybe we should succeed here if the operation is !=
		return false;
	}
}

void FFrontendFilter_ArbitraryComparisonOperation::ModifyContextMenu(FMenuBuilder& MenuBuilder)
{
	FUIAction Action;

	MenuBuilder.BeginSection(TEXT("ComparsionSection"), LOCTEXT("ComparisonSectionHeading", "AssetRegistrySearchable Comparison"));

	TSharedRef<SWidget> KeyWidget =
		SNew(SEditableTextBox)
		.Text_Raw(this, &FFrontendFilter_ArbitraryComparisonOperation::GetKeyValueAsText)
		.OnTextCommitted_Raw(this, &FFrontendFilter_ArbitraryComparisonOperation::OnKeyValueTextCommitted)
		.MinDesiredWidth(100.0f);
	TSharedRef<SWidget> ValueWidget = SNew(SEditableTextBox)
		.Text_Raw(this, &FFrontendFilter_ArbitraryComparisonOperation::GetTargetValueAsText)
		.OnTextCommitted_Raw(this, &FFrontendFilter_ArbitraryComparisonOperation::OnTargetValueTextCommitted)
		.MinDesiredWidth(100.0f);

	MenuBuilder.AddWidget(KeyWidget, LOCTEXT("KeyMenuDesc", "Tag"));
	MenuBuilder.AddWidget(ValueWidget, LOCTEXT("ValueMenuDesc", "Target Value"));

#define UE_SET_COMP_OP(Operation) \
	MenuBuilder.AddMenuEntry(FText::AsCultureInvariant(ConvertOperationToString(Operation)), \
		LOCTEXT("SwitchOpsTooltip", "Switch comparsion type"), \
		FSlateIcon(), \
		FUIAction(FExecuteAction::CreateRaw(this, &FFrontendFilter_ArbitraryComparisonOperation::SetComparisonOperation, Operation), FCanExecuteAction(), FIsActionChecked::CreateRaw(this, &FFrontendFilter_ArbitraryComparisonOperation::IsComparisonOperationEqualTo, Operation)), \
		NAME_None, \
		EUserInterfaceActionType::RadioButton);

	UE_SET_COMP_OP(ETextFilterComparisonOperation::Equal);
	UE_SET_COMP_OP(ETextFilterComparisonOperation::NotEqual);
	UE_SET_COMP_OP(ETextFilterComparisonOperation::Less);
	UE_SET_COMP_OP(ETextFilterComparisonOperation::LessOrEqual);
	UE_SET_COMP_OP(ETextFilterComparisonOperation::Greater);
	UE_SET_COMP_OP(ETextFilterComparisonOperation::GreaterOrEqual);
#undef UE_SET_COMP_OP

	MenuBuilder.EndSection();
}

void FFrontendFilter_ArbitraryComparisonOperation::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".Key")), *TagName.ToString(), IniFilename);
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".Value")), *TargetTagValue, IniFilename);
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".Op")), *FString::FromInt((int32)ComparisonOp), IniFilename);
}

void FFrontendFilter_ArbitraryComparisonOperation::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	FString TagNameAsString;
	if (GConfig->GetString(*IniSection, *(SettingsString + TEXT(".Key")), TagNameAsString, IniFilename))
	{
		TagName = *TagNameAsString;
	}

	GConfig->GetString(*IniSection, *(SettingsString + TEXT(".Value")), TargetTagValue, IniFilename);

	int32 OpAsInteger;
	if (GConfig->GetInt(*IniSection, *(SettingsString + TEXT(".Op")), OpAsInteger, IniFilename))
	{
		ComparisonOp = (ETextFilterComparisonOperation)OpAsInteger;
	}
}

void FFrontendFilter_ArbitraryComparisonOperation::SetComparisonOperation(ETextFilterComparisonOperation NewOp)
{
	ComparisonOp = NewOp;
	BroadcastChangedEvent();
}

bool FFrontendFilter_ArbitraryComparisonOperation::IsComparisonOperationEqualTo(ETextFilterComparisonOperation TestOp) const
{
	return (ComparisonOp == TestOp);
}

FText FFrontendFilter_ArbitraryComparisonOperation::GetKeyValueAsText() const
{
	return FText::FromName(TagName);
}

FText FFrontendFilter_ArbitraryComparisonOperation::GetTargetValueAsText() const
{
	return FText::AsCultureInvariant(TargetTagValue);
}

void FFrontendFilter_ArbitraryComparisonOperation::OnKeyValueTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	if (!InText.IsEmpty())
	{
		TagName = *InText.ToString();
		BroadcastChangedEvent();
	}
}

void FFrontendFilter_ArbitraryComparisonOperation::OnTargetValueTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	TargetTagValue = InText.ToString();
	BroadcastChangedEvent();
}

FString FFrontendFilter_ArbitraryComparisonOperation::ConvertOperationToString(ETextFilterComparisonOperation Op)
{
	switch (Op)
	{
	case ETextFilterComparisonOperation::Equal:
		return TEXT("==");
	case ETextFilterComparisonOperation::NotEqual:
		return TEXT("!=");
	case ETextFilterComparisonOperation::Less:
		return TEXT("<");
	case ETextFilterComparisonOperation::LessOrEqual:
		return TEXT("<=");
	case ETextFilterComparisonOperation::Greater:
		return TEXT(">");
	case ETextFilterComparisonOperation::GreaterOrEqual:
		return TEXT(">=");
	default:
		check(false);
		return TEXT("op");
	};
}

/////////////////////////////////////////
// FFrontendFilter_ShowOtherDevelopers
/////////////////////////////////////////

FFrontendFilter_ShowOtherDevelopers::FFrontendFilter_ShowOtherDevelopers(TSharedPtr<FExtFrontendFilterCategory> InCategory)
	: FExtFrontendFilter(InCategory)
	, BaseDeveloperPath(TEXT("/Game/Developers/"))
	, BaseDeveloperPathAnsi()
	, UserDeveloperPath(BaseDeveloperPath + FPaths::GameUserDeveloperFolderName() + TEXT("/"))
	, bIsOnlyOneDeveloperPathSelected(false)
	, bShowOtherDeveloperAssets(false)
{
	TextFilterUtils::TryConvertWideToAnsi(BaseDeveloperPath, BaseDeveloperPathAnsi);
}

void FFrontendFilter_ShowOtherDevelopers::SetCurrentFilter(const FARFilter& InFilter)
{
	if ( InFilter.PackagePaths.Num() == 1 )
	{
		const FString PackagePath = InFilter.PackagePaths[0].ToString() + TEXT("/");
		
		// If the path starts with the base developer path, and is not the path itself then only one developer path is selected
		bIsOnlyOneDeveloperPathSelected = PackagePath.StartsWith(BaseDeveloperPath) && PackagePath.Len() != BaseDeveloperPath.Len();
	}
	else
	{
		// More or less than one path is selected
		bIsOnlyOneDeveloperPathSelected = false;
	}
}

bool FFrontendFilter_ShowOtherDevelopers::PassesFilter(FExtAssetFilterType InItem) const
{
	// Pass all assets if other developer assets are allowed
	if ( !bShowOtherDeveloperAssets )
	{
		// Never hide developer assets when a single developer folder is selected.
		if ( !bIsOnlyOneDeveloperPathSelected )
		{
			// If selecting multiple folders, the Developers folder/parent folder, or "All Assets", hide assets which are found in the development folder unless they are in the current user's folder
			const bool bPackageInDeveloperFolder = !TextFilterUtils::NameStrincmp(InItem.PackagePath, BaseDeveloperPath, BaseDeveloperPathAnsi, BaseDeveloperPath.Len());
			if ( bPackageInDeveloperFolder )
			{
				const FString PackagePath = InItem.PackagePath.ToString() + TEXT("/");
				const bool bPackageInUserDeveloperFolder = PackagePath.StartsWith(UserDeveloperPath);
				if ( !bPackageInUserDeveloperFolder )
				{
					return false;
				}
			}
		}
	}

	return true;
}

void FFrontendFilter_ShowOtherDevelopers::SetShowOtherDeveloperAssets(bool bValue)
{
	if ( bShowOtherDeveloperAssets != bValue )
	{
		bShowOtherDeveloperAssets = bValue;
		BroadcastChangedEvent();
	}
}

bool FFrontendFilter_ShowOtherDevelopers::GetShowOtherDeveloperAssets() const
{
	return bShowOtherDeveloperAssets;
}

/////////////////////////////////////////
// FFrontendFilter_ShowRedirectors
/////////////////////////////////////////

FFrontendFilter_ShowRedirectors::FFrontendFilter_ShowRedirectors(TSharedPtr<FExtFrontendFilterCategory> InCategory)
	: FExtFrontendFilter(InCategory)
{
	bAreRedirectorsInBaseFilter = false;
	RedirectorClassName = UObjectRedirector::StaticClass()->GetClassPathName();
}

void FFrontendFilter_ShowRedirectors::SetCurrentFilter(const FARFilter& InFilter)
{
	bAreRedirectorsInBaseFilter = InFilter.ClassPaths.Contains(RedirectorClassName);
}

bool FFrontendFilter_ShowRedirectors::PassesFilter(FExtAssetFilterType InItem) const
{
	// Never hide redirectors if they are explicitly searched for
	if ( !bAreRedirectorsInBaseFilter )
	{
		return InItem.AssetClass != RedirectorClassName.GetPackageName();
	}

	return true;
}

/////////////////////////////////////////
// FFrontendFilter_Recent
/////////////////////////////////////////

FFrontendFilter_Recent::FFrontendFilter_Recent(TSharedPtr<FExtFrontendFilterCategory> InCategory)
	: FExtFrontendFilter(InCategory)
	, bIsCurrentlyActive(false)
{
	UExtContentBrowserSettings::OnSettingChanged().AddRaw(this, &FFrontendFilter_Recent::ResetFilter);
}

FFrontendFilter_Recent::~FFrontendFilter_Recent()
{
	UExtContentBrowserSettings::OnSettingChanged().RemoveAll(this);
}

void FFrontendFilter_Recent::ActiveStateChanged(bool bActive)
{
	bIsCurrentlyActive = bActive;
}

bool FFrontendFilter_Recent::PassesFilter(FExtAssetFilterType InItem) const
{
	return RecentPackagePaths.Contains(InItem.PackageName);
}

void FFrontendFilter_Recent::SetCurrentFilter(const FARFilter& InBaseFilter)
{
	RefreshRecentPackagePaths();
}

void FFrontendFilter_Recent::RefreshRecentPackagePaths()
{
#if ECB_LEGACY
	static const FName ContentBrowserName(TEXT("ExtContentBrowser"));

	RecentPackagePaths.Reset();
	FExtContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FExtContentBrowserModule>(ContentBrowserName);
	FMainMRUFavoritesList* RecentlyOpenedAssets = CBModule.GetRecentlyOpenedAssets();
	if (RecentlyOpenedAssets)
	{
		RecentPackagePaths.Reserve(RecentlyOpenedAssets->GetNumItems());
		for (int32 i = 0; i < RecentlyOpenedAssets->GetNumItems(); ++i)
		{
			RecentPackagePaths.Add(FName(*RecentlyOpenedAssets->GetMRUItem(i)));
		}
	}
#endif
}

void FFrontendFilter_Recent::ResetFilter(FName InName)
{
#if ECB_LEGACY
	if (InName == FExtContentBrowserModule::NumberOfRecentAssetsName)
	{
		BroadcastChangedEvent();
	}
#endif
}

/////////////////////////////////////////
// FFrontendFilter_Invalid
/////////////////////////////////////////

FFrontendFilter_Invalid::FFrontendFilter_Invalid(TSharedPtr<FExtFrontendFilterCategory> InCategory)
	: FExtFrontendFilter(InCategory)
{
}

FFrontendFilter_Invalid::~FFrontendFilter_Invalid()
{
}

bool FFrontendFilter_Invalid::PassesFilter(FExtAssetFilterType InItem) const
{
	return !InItem.IsValid();
}

void FFrontendFilter_Invalid::ResetFilter(FName InName)
{
}

#undef LOCTEXT_NAMESPACE

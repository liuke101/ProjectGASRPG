// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "IExtContentBrowserSingleton.h"
#include "ExtFrontendFilterBase.h"

#include "CoreMinimal.h"
#include "CollectionManagerTypes.h"
#include "IAssetTools.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Misc/IFilter.h"

#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

class FMenuBuilder;

/** A filter for text search */
class FFrontendFilter_Text : public FExtFrontendFilter
{
public:
	FFrontendFilter_Text();
	~FFrontendFilter_Text();

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("TextFilter"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FrontendFilter_Text", "Text"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FrontendFilter_TextTooltip", "Show only assets that match the input text"); }

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

public:
	/** Returns the unsanitized and unsplit filter terms */
	FText GetRawFilterText() const;

	/** Set the Text to be used as the Filter's restrictions */
	void SetRawFilterText(const FText& InFilterText);

	/** Get the last error returned from lexing or compiling the current filter text */
	FText GetFilterErrorText() const;

	/** If bIncludeClassName is true, the text filter will include an asset's class name in the search */
	void SetIncludeClassName(const bool InIncludeClassName);

	/** If bIncludeAssetPath is true, the text filter will match against full Asset path */
	void SetIncludeAssetPath(const bool InIncludeAssetPath);

	bool GetIncludeAssetPath() const;

	/** If bIncludeCollectionNames is true, the text filter will match against collection names as well */
	void SetIncludeCollectionNames(const bool InIncludeCollectionNames);

	bool GetIncludeCollectionNames() const;
private:
	/** Handles an on collection created event */
	void HandleCollectionCreated(const FCollectionNameType& Collection);

	/** Handles an on collection destroyed event */
	void HandleCollectionDestroyed(const FCollectionNameType& Collection);

	/** Handles an on collection renamed event */
	void HandleCollectionRenamed(const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection);

	/** Handles an on collection updated event */
	void HandleCollectionUpdated(const FCollectionNameType& Collection);

	/** Rebuild the array of dynamic collections that are being referenced by the current query */
	void RebuildReferencedDynamicCollections();

	/** An array of dynamic collections that are being referenced by the current query. These should be tested against each asset when it's looking for collections that contain it */
	TArray<FCollectionNameType> ReferencedDynamicCollections;

	/** Transient context data, used when calling PassesFilter. Kept around to minimize re-allocations between multiple calls to PassesFilter */
	TSharedRef<class FFrontendFilter_TextFilterExpressionContext> TextFilterExpressionContext;

	/** Expression evaluator that can be used to perform complex text filter queries */
	FTextFilterExpressionEvaluator TextFilterExpressionEvaluator;

	/** Delegate handles */
	FDelegateHandle OnCollectionCreatedHandle;
	FDelegateHandle OnCollectionDestroyedHandle;
	FDelegateHandle OnCollectionRenamedHandle;
	FDelegateHandle OnCollectionUpdatedHandle;
};

/** A filter that displays only modified assets */
class FFrontendFilter_Modified : public FExtFrontendFilter
{
public:
	FFrontendFilter_Modified(TSharedPtr<FExtFrontendFilterCategory> InCategory);
	~FFrontendFilter_Modified();

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("Modified"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FrontendFilter_Modified", "Modified"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FrontendFilter_ModifiedTooltip", "Show only assets that have been modified and not yet saved."); }
	virtual void ActiveStateChanged(bool bActive) override;
	virtual void SetCurrentFilter(const FARFilter& InBaseFilter);

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

private:

	/** Handler for when a package's dirty state has changed */
	void OnPackageDirtyStateUpdated(UPackage* Package);

	bool bIsCurrentlyActive;

	/** Names of packages in memory that are dirty */
	TSet<FName> DirtyPackageNames;
};

/** A filter that compares the value of an asset registry tag to a target value */
class FFrontendFilter_ArbitraryComparisonOperation : public FExtFrontendFilter
{
public:
	FFrontendFilter_ArbitraryComparisonOperation(TSharedPtr<FExtFrontendFilterCategory> InCategory);

	// FFrontendFilter implementation
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual void ModifyContextMenu(FMenuBuilder& MenuBuilder) override;
	virtual void SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const override;
	virtual void LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) override;

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

protected:
	static FString ConvertOperationToString(ETextFilterComparisonOperation Op);
	
	void SetComparisonOperation(ETextFilterComparisonOperation NewOp);
	bool IsComparisonOperationEqualTo(ETextFilterComparisonOperation TestOp) const;

	FText GetKeyValueAsText() const;
	FText GetTargetValueAsText() const;
	void OnKeyValueTextCommitted(const FText& InText, ETextCommit::Type InCommitType);
	void OnTargetValueTextCommitted(const FText& InText, ETextCommit::Type InCommitType);

public:
	FName TagName;
	FString TargetTagValue;
	ETextFilterComparisonOperation ComparisonOp;
};

/** An inverse filter that allows display of content in developer folders that are not the current user's */
class FFrontendFilter_ShowOtherDevelopers : public FExtFrontendFilter
{
public:
	/** Constructor */
	FFrontendFilter_ShowOtherDevelopers(TSharedPtr<FExtFrontendFilterCategory> InCategory);

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("ShowOtherDevelopers"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FrontendFilter_ShowOtherDevelopers", "Other Developers"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FrontendFilter_ShowOtherDevelopersTooltip", "Allow display of assets in developer folders that aren't yours."); }
	virtual bool IsInverseFilter() const override { return true; }
	virtual void SetCurrentFilter(const FARFilter& InFilter) override;

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

public:
	/** Sets if we should filter out assets from other developers */
	void SetShowOtherDeveloperAssets(bool bValue);

	/** Gets if we should filter out assets from other developers */
	bool GetShowOtherDeveloperAssets() const;

private:
	FString BaseDeveloperPath;
	TArray<ANSICHAR> BaseDeveloperPathAnsi;
	FString UserDeveloperPath;
	bool bIsOnlyOneDeveloperPathSelected;
	bool bShowOtherDeveloperAssets;
};

/** An inverse filter that allows display of object redirectors */
class FFrontendFilter_ShowRedirectors : public FExtFrontendFilter
{
public:
	/** Constructor */
	FFrontendFilter_ShowRedirectors(TSharedPtr<FExtFrontendFilterCategory> InCategory);

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("ShowRedirectors"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FrontendFilter_ShowRedirectors", "Show Redirectors"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FrontendFilter_ShowRedirectorsToolTip", "Allow display of Redirectors."); }
	virtual bool IsInverseFilter() const override { return true; }
	virtual void SetCurrentFilter(const FARFilter& InFilter) override;

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

private:
	bool bAreRedirectorsInBaseFilter;
	FTopLevelAssetPath RedirectorClassName;
};

/** A filter that displays recently opened assets */
class FFrontendFilter_Recent : public FExtFrontendFilter
{
public:
	FFrontendFilter_Recent(TSharedPtr<FExtFrontendFilterCategory> InCategory);
	~FFrontendFilter_Recent();

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("RecentlyOpened"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FrontendFilter_Recent", "Recently Opened"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FrontendFilter_RecentTooltip", "Show only recently opened assets."); }
	virtual void ActiveStateChanged(bool bActive) override;
	virtual void SetCurrentFilter(const FARFilter& InBaseFilter);

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

	void ResetFilter(FName InName);

private:
	void RefreshRecentPackagePaths();

	TSet<FName> RecentPackagePaths;
	bool bIsCurrentlyActive;
};

/** A filter that displays invalid assets */
class FFrontendFilter_Invalid : public FExtFrontendFilter
{
public:
	FFrontendFilter_Invalid(TSharedPtr<FExtFrontendFilterCategory> InCategory);
	~FFrontendFilter_Invalid();

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("Invalid Asset"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FFrontendFilter_Invalid", "Invalid Asset"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FFrontendFilter_InvalidTooltip", "Show only invalid assets."); }

	// IFilter implementation
	virtual bool PassesFilter(FExtAssetFilterType InItem) const override;

	void ResetFilter(FName InName);
};

#undef LOCTEXT_NAMESPACE

// Copyright 2017-2021 marynate. All Rights Reserved.

#include "SExtFilterList.h"

#include "Styling/SlateTypes.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SBoxPanel.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SCheckBox.h"
#include "EditorStyleSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "ExtFrontendFilters.h"
#include "ContentBrowserFrontEndFilterExtension.h"

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

/** A class for check boxes in the filter list. If you double click a filter checkbox, you will enable it and disable all others */
class SFilterCheckBox : public SCheckBox
{
public:
	void SetOnFilterCtrlClicked(const FOnClicked& NewFilterCtrlClicked)
	{
		OnFilterCtrlClicked = NewFilterCtrlClicked;
	}

	void SetOnFilterAltClicked(const FOnClicked& NewFilteAltClicked)
	{
		OnFilterAltClicked = NewFilteAltClicked;
	}

	void SetOnFilterDoubleClicked( const FOnClicked& NewFilterDoubleClicked )
	{
		OnFilterDoubleClicked = NewFilterDoubleClicked;
	}

	void SetOnFilterMiddleButtonClicked( const FOnClicked& NewFilterMiddleButtonClicked )
	{
		OnFilterMiddleButtonClicked = NewFilterMiddleButtonClicked;
	}

	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override
	{
		if ( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnFilterDoubleClicked.IsBound() )
		{
			return OnFilterDoubleClicked.Execute();
		}
		else
		{
			return SCheckBox::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
		}
	}

	virtual FReply OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override
	{
		if( InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && OnFilterMiddleButtonClicked.IsBound() )
		{
			return OnFilterMiddleButtonClicked.Execute();
		}
		else
		{
			SCheckBox::OnMouseButtonUp(InMyGeometry, InMouseEvent);
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

private:
	FOnClicked OnFilterCtrlClicked;
	FOnClicked OnFilterAltClicked;
	FOnClicked OnFilterDoubleClicked;
	FOnClicked OnFilterMiddleButtonClicked;
};

/**
 * A single filter in the filter list. Can be removed by clicking the remove button on it.
 */
class SExtFilter : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam( FOnRequestRemove, const TSharedRef<SExtFilter>& /*FilterToRemove*/ );
	DECLARE_DELEGATE_OneParam( FOnRequestEnableOnly, const TSharedRef<SExtFilter>& /*FilterToEnable*/ );
	DECLARE_DELEGATE( FOnRequestEnableAll );
	DECLARE_DELEGATE( FOnRequestDisableAll );
	DECLARE_DELEGATE( FOnRequestRemoveAll );

	SLATE_BEGIN_ARGS( SExtFilter ){}

		/** The asset type actions that are associated with this filter */
		SLATE_ARGUMENT( TWeakPtr<IAssetTypeActions>, AssetTypeActions )

		/** If this is an front end filter, this is the filter object */
		SLATE_ARGUMENT( TSharedPtr<FExtFrontendFilter>, FrontendFilter )

		/** Invoked when the filter toggled */
		SLATE_EVENT( SExtFilterList::FOnFilterChanged, OnFilterChanged )

		/** Invoked when a request to remove this filter originated from within this filter */
		SLATE_EVENT( FOnRequestRemove, OnRequestRemove )

		/** Invoked when a request to enable only this filter originated from within this filter */
		SLATE_EVENT( FOnRequestEnableOnly, OnRequestEnableOnly )

		/** Invoked when a request to enable all filters originated from within this filter */
		SLATE_EVENT(FOnRequestEnableAll, OnRequestEnableAll)

		/** Invoked when a request to disable all filters originated from within this filter */
		SLATE_EVENT( FOnRequestDisableAll, OnRequestDisableAll )

		/** Invoked when a request to remove all filters originated from within this filter */
		SLATE_EVENT( FOnRequestRemoveAll, OnRequestRemoveAll )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs )
	{
		bEnabled = false;
		OnFilterChanged = InArgs._OnFilterChanged;
		AssetTypeActions = InArgs._AssetTypeActions;
		OnRequestRemove = InArgs._OnRequestRemove;
		OnRequestEnableOnly = InArgs._OnRequestEnableOnly;
		OnRequestEnableAll = InArgs._OnRequestEnableAll;
		OnRequestDisableAll = InArgs._OnRequestDisableAll;
		OnRequestRemoveAll = InArgs._OnRequestRemoveAll;
		FrontendFilter = InArgs._FrontendFilter;

		// Get the tooltip and color of the type represented by this filter
		TAttribute<FText> FilterToolTip;
		FilterColor = FLinearColor::White;
		if ( InArgs._AssetTypeActions.IsValid() )
		{
			TSharedPtr<IAssetTypeActions> TypeActions = InArgs._AssetTypeActions.Pin();
			FilterColor = FLinearColor( TypeActions->GetTypeColor() );

			// No tooltip for asset type filters
		}
		else if ( FrontendFilter.IsValid() )
		{
			FilterColor = FrontendFilter->GetColor();
			FilterToolTip = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(FrontendFilter.ToSharedRef(), &FExtFrontendFilter::GetToolTipText));
		}

		ChildSlot
		[
			SNew(SBorder)
			.Padding(1.0f)
			.BorderImage(FAppStyle::Get().GetBrush("ContentBrowser.FilterBackground"))
			[
				SAssignNew( ToggleButtonPtr, SFilterCheckBox )
				.Style(FAppStyle::Get(), "ContentBrowser.FilterButton")
				.ToolTipText(FilterToolTip)
				.Padding(0.0f)
				.IsChecked(this, &SExtFilter::IsChecked)
				.OnCheckStateChanged(this, &SExtFilter::FilterToggled)
				.OnGetMenuContent(this, &SExtFilter::GetRightClickMenuContent)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("ContentBrowser.FilterImage"))
						.ColorAndOpacity(this, &SExtFilter::GetFilterImageColorAndOpacity)
					]
					+SHorizontalBox::Slot()
					.Padding(TAttribute<FMargin>(this, &SExtFilter::GetFilterNamePadding))
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SExtFilter::GetFilterName)
						.IsEnabled_Lambda([this] {return bEnabled;})
					]
				]
			]
		];

		ToggleButtonPtr->SetOnFilterCtrlClicked(FOnClicked::CreateSP(this, &SExtFilter::FilterCtrlClicked));
		ToggleButtonPtr->SetOnFilterAltClicked(FOnClicked::CreateSP(this, &SExtFilter::FilterAltClicked));
		ToggleButtonPtr->SetOnFilterDoubleClicked( FOnClicked::CreateSP(this, &SExtFilter::FilterDoubleClicked) );
		ToggleButtonPtr->SetOnFilterMiddleButtonClicked( FOnClicked::CreateSP(this, &SExtFilter::FilterMiddleButtonClicked) );
	}

	/** Sets whether or not this filter is applied to the combined filter */
	void SetEnabled(bool InEnabled, bool InExecuteOnFilterChanged = true)
	{
		if ( InEnabled != bEnabled)
		{
			bEnabled = InEnabled;
			if (InExecuteOnFilterChanged)
			{
				OnFilterChanged.ExecuteIfBound();
			}
		}
	}

	/** Returns true if this filter contributes to the combined filter */
	bool IsEnabled() const
	{
		return bEnabled;
	}

	/** Returns this widgets contribution to the combined filter */
	FARFilter GetBackendFilter() const
	{
		FARFilter Filter;

		if ( AssetTypeActions.IsValid() )
		{
			if (AssetTypeActions.Pin()->CanFilter())
			{
				AssetTypeActions.Pin()->BuildBackendFilter(Filter);
			}
		}

		return Filter;
	}

	/** If this is an front end filter, this is the filter object */
	const TSharedPtr<FExtFrontendFilter>& GetFrontendFilter() const
	{
		return FrontendFilter;
	}

	/** Gets the asset type actions associated with this filter */
	const TWeakPtr<IAssetTypeActions>& GetAssetTypeActions() const
	{
		return AssetTypeActions;
	}

private:
	/** Handler for when the filter checkbox is clicked */
	void FilterToggled(ECheckBoxState NewState)
	{
		bEnabled = NewState == ECheckBoxState::Checked;
		OnFilterChanged.ExecuteIfBound();
	}

	/** Handler for when the filter checkbox is clicked and a control key is pressed */
	FReply FilterCtrlClicked()
	{
		OnRequestEnableAll.ExecuteIfBound();
		return FReply::Handled();
	}

	/** Handler for when the filter checkbox is clicked and an alt key is pressed */
	FReply FilterAltClicked()
	{
		OnRequestDisableAll.ExecuteIfBound();
		return FReply::Handled();
	}

	/** Handler for when the filter checkbox is double clicked */
	FReply FilterDoubleClicked()
	{
		// Disable all other filters and enable this one.
		OnRequestDisableAll.ExecuteIfBound();
		bEnabled = true;
		OnFilterChanged.ExecuteIfBound();

		return FReply::Handled();
	}

	/** Handler for when the filter checkbox is middle button clicked */
	FReply FilterMiddleButtonClicked()
	{
		RemoveFilter();
		return FReply::Handled();
	}

	/** Handler to create a right click menu */
	TSharedRef<SWidget> GetRightClickMenuContent()
	{
		FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, NULL);

		MenuBuilder.BeginSection("FilterOptions", LOCTEXT("FilterContextHeading", "Filter Options"));
		{
			MenuBuilder.AddMenuEntry(
				FText::Format( LOCTEXT("RemoveFilter", "Remove: {0}"), GetFilterName() ),
				LOCTEXT("RemoveFilterTooltip", "Remove this filter from the list. It can be added again in the filters menu."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP(this, &SExtFilter::RemoveFilter) )
				);

			MenuBuilder.AddMenuEntry(
				FText::Format( LOCTEXT("EnableOnlyThisFilter", "Enable this only: {0}"), GetFilterName() ),
				LOCTEXT("EnableOnlyThisFilterTooltip", "Enable only this filter from the list."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP(this, &SExtFilter::EnableOnly) )
				);

		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("FilterBulkOptions", LOCTEXT("BulkFilterContextHeading", "Bulk Filter Options"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("DisableAllFilters", "Disable All Filters"),
				LOCTEXT("DisableAllFiltersTooltip", "Disables all active filters."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP(this, &SExtFilter::DisableAllFilters) )
				);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("RemoveAllFilters", "Remove All Filters"),
				LOCTEXT("RemoveAllFiltersTooltip", "Removes all filters from the list."),
				FSlateIcon(),
				FUIAction( FExecuteAction::CreateSP(this, &SExtFilter::RemoveAllFilters) )
				);
		}
		MenuBuilder.EndSection();

		if (FrontendFilter.IsValid())
		{
			FrontendFilter->ModifyContextMenu(MenuBuilder);
		}

		return MenuBuilder.MakeWidget();
	}

	/** Removes this filter from the filter list */
	void RemoveFilter()
	{
		TSharedRef<SExtFilter> Self = SharedThis(this);
		OnRequestRemove.ExecuteIfBound( Self );
	}

	/** Enables only this filter from the filter list */
	void EnableOnly()
	{
		TSharedRef<SExtFilter> Self = SharedThis(this);
		OnRequestEnableOnly.ExecuteIfBound( Self );
	}

	/** Enables all filters in the list */
	void EnableAllFilters()
	{
		OnRequestEnableAll.ExecuteIfBound();
	}

	/** Disables all active filters in the list */
	void DisableAllFilters()
	{
		OnRequestDisableAll.ExecuteIfBound();
	}

	/** Removes all filters in the list */
	void RemoveAllFilters()
	{
		OnRequestRemoveAll.ExecuteIfBound();
	}

	/** Handler to determine the "checked" state of the filter checkbox */
	ECheckBoxState IsChecked() const
	{
		return bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/** Handler to determine the color of the checkbox when it is checked */
	FSlateColor GetFilterImageColorAndOpacity() const
	{
		return bEnabled ? FilterColor : FAppStyle::Get().GetSlateColor("Colors.Recessed");
	}

	/** Handler to determine the padding of the checkbox text when it is pressed */
	FMargin GetFilterNamePadding() const
	{
		return ToggleButtonPtr->IsPressed() ? FMargin(3,2,4,0) : FMargin(3,1,4,1);
	}

	/** Handler to determine the color of the checkbox text when it is hovered */
	FSlateColor GetFilterNameColorAndOpacity() const
	{
		const float DimFactor = 0.75f;
		return IsHovered() ? FLinearColor(DimFactor, DimFactor, DimFactor, 1.0f) : FLinearColor::White;
	}

	/** Returns the display name for this filter */
	FText GetFilterName() const
	{
		FText FilterName;
		if ( AssetTypeActions.IsValid() )
		{
			TSharedPtr<IAssetTypeActions> TypeActions = AssetTypeActions.Pin();
			FilterName = TypeActions->GetName();
		}
		else if ( FrontendFilter.IsValid() )
		{
			FilterName = FrontendFilter->GetDisplayName();
		}

		if ( FilterName.IsEmpty() )
		{
			FilterName = LOCTEXT("UnknownFilter", "???");
		}

		return FilterName;
	}

private:
	/** Invoked when the filter toggled */
	SExtFilterList::FOnFilterChanged OnFilterChanged;

	/** Invoked when a request to remove this filter originated from within this filter */
	FOnRequestRemove OnRequestRemove;

	/** Invoked when a request to enable only this filter originated from within this filter */
	FOnRequestEnableOnly OnRequestEnableOnly;

	/** Invoked when a request to enable all filters originated from within this filter */
	FOnRequestEnableAll OnRequestEnableAll;

	/** Invoked when a request to disable all filters originated from within this filter */
	FOnRequestDisableAll OnRequestDisableAll;

	/** Invoked when a request to remove all filters originated from within this filter */
	FOnRequestDisableAll OnRequestRemoveAll;

	/** true when this filter should be applied to the search */
	bool bEnabled;

	/** The asset type actions that are associated with this filter */
	TWeakPtr<IAssetTypeActions> AssetTypeActions;

	/** If this is an front end filter, this is the filter object */
	TSharedPtr<FExtFrontendFilter> FrontendFilter;

	/** The button to toggle the filter on or off */
	TSharedPtr<SFilterCheckBox> ToggleButtonPtr;

	/** The color of the checkbox for this filter */
	FLinearColor FilterColor;
};


/////////////////////
// SExtFilterList
/////////////////////


void SExtFilterList::Construct( const FArguments& InArgs )
{
	OnGetContextMenu = InArgs._OnGetContextMenu;
	OnFilterChanged = InArgs._OnFilterChanged;
	FrontendFilters = InArgs._FrontendFilters;
	InitialClassFilters = InArgs._InitialClassFilters;

#if ECB_WIP_MORE_FILTER
	TSharedPtr<FExtFrontendFilterCategory> DefaultCategory = MakeShareable( new FExtFrontendFilterCategory(LOCTEXT("FrontendFiltersCategory", "Other Filters"), LOCTEXT("FrontendFiltersCategoryTooltip", "Filter assets by all filters in this category.")) );
#if ECB_DISABLE
	// Add all built-in frontend filters here
	AllFrontendFilters.Add( MakeShareable(new FFrontendFilter_Modified(DefaultCategory)) );
	AllFrontendFilters.Add( MakeShareable(new FFrontendFilter_ShowOtherDevelopers(DefaultCategory)) );
	AllFrontendFilters.Add( MakeShareable(new FFrontendFilter_ShowRedirectors(DefaultCategory)) );
	AllFrontendFilters.Add( MakeShareable(new FFrontendFilter_ArbitraryComparisonOperation(DefaultCategory)) );
	AllFrontendFilters.Add(MakeShareable(new FFrontendFilter_Recent(DefaultCategory)));	
#endif
	AllFrontendFilters.Add(MakeShareable(new FFrontendFilter_Invalid(DefaultCategory)));
#endif

	// Add in filters specific to this invocation
	for (auto Iter = InArgs._ExtraFrontendFilters.CreateConstIterator(); Iter; ++Iter)
	{
		TSharedRef<FExtFrontendFilter> Filter = (*Iter);
		TSharedPtr<FExtFrontendFilterCategory> Category = Filter->GetCategory();
		if (Category.IsValid())
		{
			AllFrontendFilterCategories.AddUnique( Category );
		}

		AllFrontendFilters.Add(Filter);
	}

#if ECB_WIP_MORE_FILTER
	AllFrontendFilterCategories.AddUnique(DefaultCategory);
#endif

	// Auto add all inverse filters
	for (auto FilterIt = AllFrontendFilters.CreateConstIterator(); FilterIt; ++FilterIt)
	{
		SetFrontendFilterActive(*FilterIt, false);
	}

	FilterBox = SNew(SWrapBox)
		.UseAllottedWidth(true);

	ChildSlot
	[
		FilterBox.ToSharedRef()
	];
}

FReply SExtFilterList::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		if ( OnGetContextMenu.IsBound() )
		{
			FReply Reply = FReply::Handled().ReleaseMouseCapture();

			// Get the context menu content. If NULL, don't open a menu.
			TSharedPtr<SWidget> MenuContent = OnGetContextMenu.Execute();

			if ( MenuContent.IsValid() )
			{
				FVector2D SummonLocation = MouseEvent.GetScreenSpacePosition();
				FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
				FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuContent.ToSharedRef(), SummonLocation, FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
			}

			return Reply;
		}
	}

	return FReply::Unhandled();
}

const TArray<UClass*>& SExtFilterList::GetInitialClassFilters()
{
	return InitialClassFilters;
}

bool SExtFilterList::HasAnyFilters() const
{
	return Filters.Num() > 0;
}

FARFilter SExtFilterList::GetCombinedBackendFilter() const
{
	FARFilter CombinedFilter;

	// Add all selected filters
	for (int32 FilterIdx = 0; FilterIdx < Filters.Num(); ++FilterIdx)
	{
		if ( Filters[FilterIdx]->IsEnabled() )
		{
			CombinedFilter.Append(Filters[FilterIdx]->GetBackendFilter());
		}
	}

	if ( CombinedFilter.bRecursiveClasses )
	{
		// Add exclusions for AssetTypeActions NOT in the filter.
		// This will prevent assets from showing up that are both derived from an asset in the filter set and derived from an asset not in the filter set
		// Get the list of all asset type actions
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		TArray< TWeakPtr<IAssetTypeActions> > AssetTypeActionsList;
		AssetToolsModule.Get().GetAssetTypeActionsList(AssetTypeActionsList);
		for ( auto TypeActionsIt = AssetTypeActionsList.CreateConstIterator(); TypeActionsIt; ++TypeActionsIt )
		{
			const TWeakPtr<IAssetTypeActions>& WeakTypeActions = *TypeActionsIt;
			if ( WeakTypeActions.IsValid() )
			{
				const TSharedPtr<IAssetTypeActions> TypeActions = WeakTypeActions.Pin();
				if ( TypeActions->CanFilter() )
				{
					const UClass* TypeClass = TypeActions->GetSupportedClass();
// 					if (!CombinedFilter.ClassNames.Contains(TypeClass->GetFName()))
// 					{
// 						CombinedFilter.RecursiveClassesExclusionSet.Add(TypeClass->GetFName());
// 					}
					if (TypeClass && !CombinedFilter.ClassPaths.Contains(TypeClass->GetClassPathName()))
					{
						CombinedFilter.RecursiveClassPathsExclusionSet.Add(TypeClass->GetClassPathName());
					}
				}
			}
		}
	}

	// HACK: A blueprint can be shown as Blueprint or as BlueprintGeneratedClass, but we don't want to distinguish them while filtering.
	// This should be removed, once all blueprints are shown as BlueprintGeneratedClass.
// 	if(CombinedFilter.ClassNames.Contains(FName(TEXT("Blueprint"))))
// 	{
// 		CombinedFilter.ClassNames.AddUnique(FName(TEXT("BlueprintGeneratedClass")));
// 	}
	if (CombinedFilter.ClassPaths.Contains(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint"))))
	{
		CombinedFilter.ClassPaths.AddUnique(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("BlueprintGeneratedClass")));
	}

	return CombinedFilter;
}

TSharedRef<SWidget> SExtFilterList::ExternalMakeAddFilterMenu(EAssetTypeCategories::Type MenuExpansion)
{
	return MakeAddFilterMenu(MenuExpansion);
}

void SExtFilterList::EnableAllFilters()
{
	for (const TSharedRef<SExtFilter>& Filter : Filters)
	{
		Filter->SetEnabled(true, false);
		if (const TSharedPtr<FExtFrontendFilter>& FrontendFilter = Filter->GetFrontendFilter())
		{
			SetFrontendFilterActive(FrontendFilter.ToSharedRef(), true);
		}
	}

	OnFilterChanged.ExecuteIfBound();
}

void SExtFilterList::DisableAllFilters()
{
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
	{
		(*FilterIt)->SetEnabled(false, false);
	}

	OnFilterChanged.ExecuteIfBound();
}

void SExtFilterList::RemoveAllFilters()
{
	if ( HasAnyFilters() )
	{
		for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
		{
			const TSharedRef<SExtFilter>& FilterToRemove = *FilterIt;

			if (FilterToRemove->GetFrontendFilter().IsValid() )
			{
				// Update the frontend filters collection
				const TSharedRef<FExtFrontendFilter>& FrontendFilter = FilterToRemove->GetFrontendFilter().ToSharedRef();
				SetFrontendFilterActive(FrontendFilter, false);
			}
		}

		FilterBox->ClearChildren();
		Filters.Empty();

		// Notify that a filter has changed
		OnFilterChanged.ExecuteIfBound();
	}
}

void SExtFilterList::DisableFiltersThatHideAssets(const TArray<FExtAssetData>& AssetDataList)
{

#if ECB_WIP_SYNC_ASSET // todo: disable filters that might hide assets is syncing

	if ( HasAnyFilters() )
	{
		// Determine if we should disable backend filters. If any asset fails the combined backend filter, disable them all.
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FARFilter CombinedBackendFilter = GetCombinedBackendFilter();
		bool bDisableAllBackendFilters = false;
		TArray<FAssetData> LocalAssetDataList = AssetDataList;
		AssetRegistryModule.Get().RunAssetsThroughFilter(LocalAssetDataList, CombinedBackendFilter);
		if ( LocalAssetDataList.Num() != AssetDataList.Num() )
		{
			bDisableAllBackendFilters = true;
		}

		// Iterate over all enabled filters and disable any frontend filters that would hide any of the supplied assets
		// and disable all backend filters if it was determined that the combined backend filter hides any of the assets
		bool ExecuteOnFilteChanged = false;
		for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
		{
			const TSharedRef<SExtFilter>& Filter = *FilterIt;
			if ( Filter->IsEnabled() )
			{
				const TSharedPtr<FExtFrontendFilter>& FrontendFilter = Filter->GetFrontendFilter();
				if ( FrontendFilter.IsValid() )
				{
					for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
					{
						if (!FrontendFilter->IsInverseFilter() && !FrontendFilter->PassesFilter(*AssetIt))
						{
							// This is a frontend filter and at least one asset did not pass.
							Filter->SetEnabled(false, false);
							ExecuteOnFilteChanged = true;
						}
					}
				}

				if ( bDisableAllBackendFilters )
				{
					FARFilter BackendFilter = Filter->GetBackendFilter();
					if ( !BackendFilter.IsEmpty() )
					{
						Filter->SetEnabled(false, false);
						ExecuteOnFilteChanged = true;
					}
				}
			}
		}

		if (ExecuteOnFilteChanged)
		{
			OnFilterChanged.ExecuteIfBound();
		}
	}
#endif
}

void SExtFilterList::SaveSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString) const
{
	FString ActiveTypeFilterString;
	FString EnabledTypeFilterString;
	FString ActiveFrontendFilterString;
	FString EnabledFrontendFilterString;
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
	{
		const TSharedRef<SExtFilter>& Filter = *FilterIt;

		if ( Filter->GetAssetTypeActions().IsValid() )
		{
			if ( ActiveTypeFilterString.Len() > 0 )
			{
				ActiveTypeFilterString += TEXT(",");
			}

			const FString FilterName = Filter->GetAssetTypeActions().Pin()->GetSupportedClass()->GetName();
			ActiveTypeFilterString += FilterName;

			if ( Filter->IsEnabled() )
			{
				if ( EnabledTypeFilterString.Len() > 0 )
				{
					EnabledTypeFilterString += TEXT(",");
				}

				EnabledTypeFilterString += FilterName;
			}
		}
		else if ( Filter->GetFrontendFilter().IsValid() )
		{
			const TSharedPtr<FExtFrontendFilter>& FrontendFilter = Filter->GetFrontendFilter();
			if ( ActiveFrontendFilterString.Len() > 0 )
			{
				ActiveFrontendFilterString += TEXT(",");
			}

			const FString FilterName = FrontendFilter->GetName();
			ActiveFrontendFilterString += FilterName;

			if ( Filter->IsEnabled() )
			{
				if ( EnabledFrontendFilterString.Len() > 0 )
				{
					EnabledFrontendFilterString += TEXT(",");
				}

				EnabledFrontendFilterString += FilterName;
			}

			const FString CustomSettingsString = FString::Printf(TEXT("%s.CustomSettings.%s"), *SettingsString, *FilterName);
			FrontendFilter->SaveSettings(IniFilename, IniSection, CustomSettingsString);
		}
	}

	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".ActiveTypeFilters")), *ActiveTypeFilterString, IniFilename);
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".EnabledTypeFilters")), *EnabledTypeFilterString, IniFilename);
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".ActiveFrontendFilters")), *ActiveFrontendFilterString, IniFilename);
	GConfig->SetString(*IniSection, *(SettingsString + TEXT(".EnabledFrontendFilters")), *EnabledFrontendFilterString, IniFilename);
}

void SExtFilterList::LoadSettings(const FString& IniFilename, const FString& IniSection, const FString& SettingsString)
{
	{
		// Add all the type filters that were found in the ActiveTypeFilters
		FString ActiveTypeFilterString;
		FString EnabledTypeFilterString;
		GConfig->GetString(*IniSection, *(SettingsString + TEXT(".ActiveTypeFilters")), ActiveTypeFilterString, IniFilename);
		GConfig->GetString(*IniSection, *(SettingsString + TEXT(".EnabledTypeFilters")), EnabledTypeFilterString, IniFilename);

		// Parse comma delimited strings into arrays
		TArray<FString> TypeFilterNames;
		TArray<FString> EnabledTypeFilterNames;
		ActiveTypeFilterString.ParseIntoArray(TypeFilterNames, TEXT(","), /*bCullEmpty=*/true);
		EnabledTypeFilterString.ParseIntoArray(EnabledTypeFilterNames, TEXT(","), /*bCullEmpty=*/true);

		// Get the list of all asset type actions
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		TArray< TWeakPtr<IAssetTypeActions> > AssetTypeActionsList;
		AssetToolsModule.Get().GetAssetTypeActionsList(AssetTypeActionsList);

		// For each TypeActions, add any that were active and enable any that were previously enabled
		for ( auto TypeActionsIt = AssetTypeActionsList.CreateConstIterator(); TypeActionsIt; ++TypeActionsIt )
		{
			const TWeakPtr<IAssetTypeActions>& TypeActions = *TypeActionsIt;
			if ( TypeActions.IsValid() && TypeActions.Pin()->CanFilter() && !IsAssetTypeActionsInUse(TypeActions) )
			{
				const FString& ClassName = TypeActions.Pin()->GetSupportedClass()->GetName();
				if ( TypeFilterNames.Contains(ClassName) )
				{
					TSharedRef<SExtFilter> NewFilter = AddFilter(TypeActions);

					if ( EnabledTypeFilterNames.Contains(ClassName) )
					{
						NewFilter->SetEnabled(true, false);
					}
				}
			}
		}
	}

	{
		// Add all the frontend filters that were found in the ActiveFrontendFilters
		FString ActiveFrontendFilterString;	
		FString EnabledFrontendFilterString;
		GConfig->GetString(*IniSection, *(SettingsString + TEXT(".ActiveFrontendFilters")), ActiveFrontendFilterString, IniFilename);
		GConfig->GetString(*IniSection, *(SettingsString + TEXT(".EnabledFrontendFilters")), EnabledFrontendFilterString, IniFilename);

		// Parse comma delimited strings into arrays
		TArray<FString> FrontendFilterNames;
		TArray<FString> EnabledFrontendFilterNames;
		ActiveFrontendFilterString.ParseIntoArray(FrontendFilterNames, TEXT(","), /*bCullEmpty=*/true);
		EnabledFrontendFilterString.ParseIntoArray(EnabledFrontendFilterNames, TEXT(","), /*bCullEmpty=*/true);

		// For each FrontendFilter, add any that were active and enable any that were previously enabled
		for ( auto FrontendFilterIt = AllFrontendFilters.CreateIterator(); FrontendFilterIt; ++FrontendFilterIt )
		{
			TSharedRef<FExtFrontendFilter>& FrontendFilter = *FrontendFilterIt;
			const FString& FilterName = FrontendFilter->GetName();
			if (!IsFrontendFilterInUse(FrontendFilter))
			{
				if ( FrontendFilterNames.Contains(FilterName) )
				{
					TSharedRef<SExtFilter> NewFilter = AddFilter(FrontendFilter);

					if ( EnabledFrontendFilterNames.Contains(FilterName) )
					{
						NewFilter->SetEnabled(true, false);
						SetFrontendFilterActive(FrontendFilter, NewFilter->IsEnabled());
					}
				}
			}

			const FString CustomSettingsString = FString::Printf(TEXT("%s.CustomSettings.%s"), *SettingsString, *FilterName);
			FrontendFilter->LoadSettings(IniFilename, IniSection, CustomSettingsString);
		}
	}

	OnFilterChanged.ExecuteIfBound();
}

void SExtFilterList::SetFrontendFilterActive(const TSharedRef<FExtFrontendFilter>& Filter, bool bActive)
{
	if(Filter->IsInverseFilter())
	{
		//Inverse filters are active when they are "disabled"
		bActive = !bActive;
	}
	Filter->ActiveStateChanged(bActive);

	if ( bActive )
	{
		FrontendFilters->Add(Filter);
	}
	else
	{
		FrontendFilters->Remove(Filter);
	}
}

TSharedRef<SExtFilter> SExtFilterList::AddFilter(const TWeakPtr<IAssetTypeActions>& AssetTypeActions)
{
	TSharedRef<SExtFilter> NewFilter =
		SNew(SExtFilter)
		.AssetTypeActions(AssetTypeActions)
		.OnFilterChanged(OnFilterChanged)
		.OnRequestRemove(this, &SExtFilterList::RemoveFilterAndUpdate)
		.OnRequestEnableOnly(this, &SExtFilterList::EnableOnlyThisFilter)
		.OnRequestEnableAll(this, &SExtFilterList::EnableAllFilters)
		.OnRequestDisableAll(this, &SExtFilterList::DisableAllFilters)
		.OnRequestRemoveAll(this, &SExtFilterList::RemoveAllFilters);

	AddFilter( NewFilter );

	return NewFilter;
}

TSharedRef<SExtFilter> SExtFilterList::AddFilter(const TSharedRef<FExtFrontendFilter>& FrontendFilter)
{
	TSharedRef<SExtFilter> NewFilter =
		SNew(SExtFilter)
		.FrontendFilter(FrontendFilter)
		.OnFilterChanged( this, &SExtFilterList::FrontendFilterChanged, FrontendFilter )
		.OnRequestRemove(this, &SExtFilterList::RemoveFilterAndUpdate)
		.OnRequestEnableOnly(this, &SExtFilterList::EnableOnlyThisFilter)
		.OnRequestEnableAll(this, &SExtFilterList::EnableAllFilters)
		.OnRequestDisableAll(this, &SExtFilterList::DisableAllFilters)
		.OnRequestRemoveAll(this, &SExtFilterList::RemoveAllFilters);

	AddFilter( NewFilter );

	return NewFilter;
}

void SExtFilterList::AddFilter(const TSharedRef<SExtFilter>& FilterToAdd)
{
	Filters.Add(FilterToAdd);

	FilterBox->AddSlot()
	.Padding(3, 3)
	[
		FilterToAdd
	];
}

void SExtFilterList::RemoveFilter(const TWeakPtr<IAssetTypeActions>& AssetTypeActions, bool ExecuteOnFilterChanged)
{
	TSharedPtr<SExtFilter> FilterToRemove;
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
	{
		const TWeakPtr<IAssetTypeActions>& Actions = (*FilterIt)->GetAssetTypeActions();
		if ( Actions.IsValid() && Actions == AssetTypeActions)
		{
			FilterToRemove = *FilterIt;
			break;
		}
	}

	if ( FilterToRemove.IsValid() )
	{
		if (ExecuteOnFilterChanged)
		{
			RemoveFilterAndUpdate(FilterToRemove.ToSharedRef());
		}
		else
		{
			RemoveFilter(FilterToRemove.ToSharedRef());
		}
	}
}

void SExtFilterList::EnableOnlyThisFilter(const TSharedRef<SExtFilter>& FilterToEnable)
{
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
	{
		bool bEnable = *FilterIt == FilterToEnable;
		(*FilterIt)->SetEnabled(bEnable, false);
	}

	OnFilterChanged.ExecuteIfBound();
}

void SExtFilterList::RemoveFilter(const TSharedRef<FExtFrontendFilter>& FrontendFilter, bool ExecuteOnFilterChanged)
{
	TSharedPtr<SExtFilter> FilterToRemove;
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
	{
		TSharedPtr<FExtFrontendFilter> Filter = (*FilterIt)->GetFrontendFilter();
		if ( Filter.IsValid() && Filter == FrontendFilter )
		{
			FilterToRemove = *FilterIt;
			break;
		}
	}

	if ( FilterToRemove.IsValid() )
	{
		if (ExecuteOnFilterChanged)
		{
			RemoveFilterAndUpdate(FilterToRemove.ToSharedRef());
		}
		else
		{
			RemoveFilter(FilterToRemove.ToSharedRef());
		}
	}
}

void SExtFilterList::RemoveFilter(const TSharedRef<SExtFilter>& FilterToRemove)
{
	FilterBox->RemoveSlot(FilterToRemove);
	Filters.Remove(FilterToRemove);

	if (FilterToRemove->GetFrontendFilter().IsValid() )
	{
		// Update the frontend filters collection
		const TSharedRef<FExtFrontendFilter>& FrontendFilter = FilterToRemove->GetFrontendFilter().ToSharedRef();
		SetFrontendFilterActive(FrontendFilter, false);
		OnFilterChanged.ExecuteIfBound();
	}
}

void SExtFilterList::RemoveFilterAndUpdate(const TSharedRef<SExtFilter>& FilterToRemove)
{
	RemoveFilter(FilterToRemove);

	// Notify that a filter has changed
	OnFilterChanged.ExecuteIfBound();
}

void SExtFilterList::FrontendFilterChanged(TSharedRef<FExtFrontendFilter> FrontendFilter)
{
	TSharedPtr<SExtFilter> FilterToUpdate;
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt )
	{
		TSharedPtr<FExtFrontendFilter> Filter = (*FilterIt)->GetFrontendFilter();
		if ( Filter.IsValid() && Filter == FrontendFilter )
		{
			FilterToUpdate = *FilterIt;
			break;
		}
	}

	if ( FilterToUpdate.IsValid() )
	{
		SetFrontendFilterActive(FrontendFilter, FilterToUpdate->IsEnabled());

		OnFilterChanged.ExecuteIfBound();
	}
}

void SExtFilterList::CreateFiltersMenuCategory(FMenuBuilder& MenuBuilder, const TArray<TWeakPtr<IAssetTypeActions>> AssetTypeActionsList) const
{
	for (int32 ClassIdx = 0; ClassIdx < AssetTypeActionsList.Num(); ++ClassIdx)
	{
		const TWeakPtr<IAssetTypeActions>& WeakTypeActions = AssetTypeActionsList[ClassIdx];
		if ( WeakTypeActions.IsValid() )
		{
			TSharedPtr<IAssetTypeActions> TypeActions = WeakTypeActions.Pin();
			if ( TypeActions.IsValid() )
			{
				const FText& LabelText = TypeActions->GetName();
				MenuBuilder.AddMenuEntry(
					LabelText,
					FText::Format( LOCTEXT("FilterByTooltipPrefix", "Filter by {0}"), LabelText ),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP( const_cast<SExtFilterList*>(this), &SExtFilterList::FilterByTypeClicked, WeakTypeActions ),
						FCanExecuteAction(),
						FIsActionChecked::CreateSP(this, &SExtFilterList::IsAssetTypeActionsInUse, WeakTypeActions ) ),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
					);
			}
		}
	}
}

void SExtFilterList::CreateOtherFiltersMenuCategory(FMenuBuilder& MenuBuilder, TSharedPtr<FExtFrontendFilterCategory> MenuCategory) const
{	
	for ( auto FrontendFilterIt = AllFrontendFilters.CreateConstIterator(); FrontendFilterIt; ++FrontendFilterIt )
	{
		const TSharedRef<FExtFrontendFilter>& FrontendFilter = *FrontendFilterIt;

		if(FrontendFilter->GetCategory() == MenuCategory)
		{
			MenuBuilder.AddMenuEntry(
				FrontendFilter->GetDisplayName(),
				FrontendFilter->GetToolTipText(),
				FSlateIcon(FAppStyle::Get().GetStyleSetName(), FrontendFilter->GetIconName()),
				FUIAction(
				FExecuteAction::CreateSP( const_cast<SExtFilterList*>(this), &SExtFilterList::FrontendFilterClicked, FrontendFilter ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtFilterList::IsFrontendFilterInUse, FrontendFilter ) ),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
				);
		}
	}
}

bool IsFilteredByPicker(TArray<UClass*> FilterClassList, UClass* TestClass)
{
	if(FilterClassList.Num() == 0)
	{
		return false;
	}
	for(auto Iter = FilterClassList.CreateIterator(); Iter; ++Iter)
	{
		if(TestClass->IsChildOf(*Iter))
		{
			return false;
		}
	}
	return true;
}

TSharedRef<SWidget> SExtFilterList::MakeAddFilterMenu(EAssetTypeCategories::Type MenuExpansion)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	// A local struct to describe a category in the filter menu
	struct FCategoryMenu
	{
		FText Name;
		FText Tooltip;
		TArray<TWeakPtr<IAssetTypeActions>> Assets;

		//Menu section
		FName SectionExtensionHook;
		FText SectionHeading;

		FCategoryMenu(const FText& InName, const FText& InTooltip, const FName& InSectionExtensionHook, const FText& InSectionHeading)
			: Name(InName)
			, Tooltip(InTooltip)
			, Assets()
			, SectionExtensionHook(InSectionExtensionHook)
			, SectionHeading(InSectionHeading)
		{}
	};

	// Create a map of Categories to Menus
	TMap<EAssetTypeCategories::Type, FCategoryMenu> CategoryToMenuMap;

	// Add the Basic category
	CategoryToMenuMap.Add(EAssetTypeCategories::Basic, FCategoryMenu( LOCTEXT("BasicFilter", "Basic"), LOCTEXT("BasicFilterTooltip", "Filter by basic assets."), "ContentBrowserFilterBasicAsset", LOCTEXT("BasicAssetsMenuHeading", "Basic Assets") ) );

	// Add the advanced categories
	TArray<FAdvancedAssetCategory> AdvancedAssetCategories;
	AssetToolsModule.Get().GetAllAdvancedAssetCategories(/*out*/ AdvancedAssetCategories);

	for (const FAdvancedAssetCategory& AdvancedAssetCategory : AdvancedAssetCategories)
	{
		const FName ExtensionPoint = NAME_None;
		const FText SectionHeading = FText::Format(LOCTEXT("WildcardFilterHeadingHeadingTooltip", "{0} Assets."), AdvancedAssetCategory.CategoryName);
		const FText Tooltip = FText::Format(LOCTEXT("WildcardFilterTooltip", "Filter by {0}."), SectionHeading);
		CategoryToMenuMap.Add(AdvancedAssetCategory.CategoryType, FCategoryMenu(AdvancedAssetCategory.CategoryName, Tooltip, ExtensionPoint, SectionHeading));
	}

	// Get the browser type maps
	TArray<TWeakPtr<IAssetTypeActions>> AssetTypeActionsList;
	AssetToolsModule.Get().GetAssetTypeActionsList(AssetTypeActionsList);

	// Sort the list
	struct FCompareIAssetTypeActions
	{
		FORCEINLINE bool operator()( const TWeakPtr<IAssetTypeActions>& A, const TWeakPtr<IAssetTypeActions>& B ) const
		{
			return A.Pin()->GetName().CompareTo( B.Pin()->GetName() ) == -1;
		}
	};
	AssetTypeActionsList.Sort( FCompareIAssetTypeActions() );

	// For every asset type, move it into all the categories it should appear in
	for (int32 ClassIdx = 0; ClassIdx < AssetTypeActionsList.Num(); ++ClassIdx)
	{
		const TWeakPtr<IAssetTypeActions>& WeakTypeActions = AssetTypeActionsList[ClassIdx];
		if ( WeakTypeActions.IsValid() )
		{
			TSharedPtr<IAssetTypeActions> TypeActions = WeakTypeActions.Pin();
			if ( ensure(TypeActions.IsValid()) && TypeActions->CanFilter() )
			{
				if(!IsFilteredByPicker(InitialClassFilters, TypeActions->GetSupportedClass()))
				{
					for ( auto MenuIt = CategoryToMenuMap.CreateIterator(); MenuIt; ++MenuIt )
					{
						if ( TypeActions->GetCategories() & MenuIt.Key() )
						{
							// This is a valid asset type which can be filtered, add it to the correct category
							FCategoryMenu& Menu = MenuIt.Value();
							Menu.Assets.Add( WeakTypeActions );
						}
					}
				}
			}
		}
	}

	for (auto MenuIt = CategoryToMenuMap.CreateIterator(); MenuIt; ++MenuIt)
	{
		if (MenuIt.Value().Assets.Num() == 0)
		{
			CategoryToMenuMap.Remove(MenuIt.Key());
		}
	}

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr, nullptr, /*bCloseSelfOnly=*/true);

	MenuBuilder.BeginSection( "ContentBrowserResetFilters" );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "FilterListResetFilters", "Reset Filters" ),
			LOCTEXT( "FilterListResetToolTip", "Resets current filter selection" ),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateSP( this, &SExtFilterList::OnResetFilters ) )
			);
	}
	MenuBuilder.EndSection(); //ContentBrowserResetFilters

	// First add the expanded category, this appears as standard entries in the list (Note: intentionally not using FindChecked here as removing it from the map later would cause the ref to be garbage)
	FCategoryMenu* ExpandedCategory = CategoryToMenuMap.Find( MenuExpansion );
	check( ExpandedCategory );

	MenuBuilder.BeginSection(ExpandedCategory->SectionExtensionHook, ExpandedCategory->SectionHeading );
	{
		if(MenuExpansion == EAssetTypeCategories::Basic)
		{
			// If we are doing a full menu (i.e expanding basic) we add a menu entry which toggles all other categories
			MenuBuilder.AddMenuEntry(
				ExpandedCategory->Name,
				ExpandedCategory->Tooltip,
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP( this, &SExtFilterList::FilterByTypeCategoryClicked, MenuExpansion ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtFilterList::IsAssetTypeCategoryInUse, MenuExpansion ) ),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
				);
		}

		// Now populate with all the basic assets
		SExtFilterList::CreateFiltersMenuCategory( MenuBuilder, ExpandedCategory->Assets);
	}
	MenuBuilder.EndSection(); //ContentBrowserFilterBasicAsset

	// Remove the basic category from the map now, as this is treated differently and is no longer needed.
	ExpandedCategory = NULL;
	CategoryToMenuMap.Remove( EAssetTypeCategories::Basic );

	// If we have expanded Basic, assume we are in full menu mode and add all the other categories
	MenuBuilder.BeginSection("ContentBrowserFilterAdvancedAsset", LOCTEXT("AdvancedAssetsMenuHeading", "Other Assets") );
	{
		if(MenuExpansion == EAssetTypeCategories::Basic)
		{
			// For all the remaining categories, add them as submenus
			for ( auto MenuIt = CategoryToMenuMap.CreateIterator(); MenuIt; ++MenuIt )
			{
				FCategoryMenu& Menu = MenuIt.Value();

				MenuBuilder.AddSubMenu(
					Menu.Name,
					Menu.Tooltip,
					FNewMenuDelegate::CreateSP(this, &SExtFilterList::CreateFiltersMenuCategory, Menu.Assets),
					FUIAction(
					FExecuteAction::CreateSP( this, &SExtFilterList::FilterByTypeCategoryClicked, MenuIt.Key() ),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SExtFilterList::IsAssetTypeCategoryInUse, MenuIt.Key() ) ),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
					);
			}
		}

		// Now add the other filter which aren't assets
		for(auto Iter = AllFrontendFilterCategories.CreateIterator(); Iter; ++Iter)
		{
			TSharedPtr<FExtFrontendFilterCategory> Category = (*Iter);

			MenuBuilder.AddSubMenu(
				Category->Title,
				Category->Tooltip,
				FNewMenuDelegate::CreateSP(this, &SExtFilterList::CreateOtherFiltersMenuCategory, Category),
				FUIAction(
				FExecuteAction::CreateSP( this, &SExtFilterList::FrontendFilterCategoryClicked, Category ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SExtFilterList::IsFrontendFilterCategoryInUse, Category ) ),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
				);
		}
	}
	MenuBuilder.EndSection(); //ContentBrowserFilterAdvancedAsset

	MenuBuilder.BeginSection("ContentBrowserFilterMiscAsset", LOCTEXT("MiscAssetsMenuHeading", "Misc Options") );
	MenuBuilder.EndSection(); //ContentBrowserFilterMiscAsset

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetCachedDisplayMetrics( DisplayMetrics );

	const FVector2D DisplaySize(
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top );

	return 
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.MaxHeight(DisplaySize.Y * 0.9)
		[
			MenuBuilder.MakeWidget()
		];
}

void SExtFilterList::FilterByTypeClicked(TWeakPtr<IAssetTypeActions> AssetTypeActions)
{
	if ( AssetTypeActions.IsValid() )
	{
		if ( IsAssetTypeActionsInUse(AssetTypeActions) )
		{
			RemoveFilter(AssetTypeActions);
		}
		else
		{
			TSharedRef<SExtFilter> NewFilter = AddFilter(AssetTypeActions);
			NewFilter->SetEnabled(true);
		}
	}
}

bool SExtFilterList::IsAssetTypeActionsInUse(TWeakPtr<IAssetTypeActions> AssetTypeActions) const
{
	if ( !AssetTypeActions.IsValid() )
	{
		return false;
	}

	TSharedPtr<IAssetTypeActions> TypeActions = AssetTypeActions.Pin();
	if ( !TypeActions.IsValid() )
	{
		return false;
	}

	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt)
	{
		TWeakPtr<IAssetTypeActions> FilterActions = (*FilterIt)->GetAssetTypeActions();
		if ( FilterActions.IsValid() && FilterActions.Pin() == TypeActions )
		{
			return true;
		}
	}

	return false;
}

void SExtFilterList::FilterByTypeCategoryClicked(EAssetTypeCategories::Type Category)
{
	TArray<TWeakPtr<IAssetTypeActions>> TypeActionsList;
	GetTypeActionsForCategory(Category, TypeActionsList);

	bool bFullCategoryInUse = IsAssetTypeCategoryInUse(Category);
	bool ExecuteOnFilterChanged = false;
	for ( auto TypeIt = TypeActionsList.CreateConstIterator(); TypeIt; ++TypeIt )
	{
		auto AssetTypeActions = (*TypeIt);
		if ( AssetTypeActions.IsValid() )
		{
			if ( bFullCategoryInUse )
			{
				RemoveFilter(AssetTypeActions);
				ExecuteOnFilterChanged = true;
			}
			else if ( !IsAssetTypeActionsInUse(AssetTypeActions) )
			{
				TSharedRef<SExtFilter> NewFilter = AddFilter(AssetTypeActions);
				NewFilter->SetEnabled(true, false);
				ExecuteOnFilterChanged = true;
			}
		}
	}

	if (ExecuteOnFilterChanged)
	{
		OnFilterChanged.ExecuteIfBound();
	}
}

bool SExtFilterList::IsAssetTypeCategoryInUse(EAssetTypeCategories::Type Category) const
{
	TArray<TWeakPtr<IAssetTypeActions>> TypeActionsList;
	GetTypeActionsForCategory(Category, TypeActionsList);

	for ( auto TypeIt = TypeActionsList.CreateConstIterator(); TypeIt; ++TypeIt )
	{
		auto AssetTypeActions = (*TypeIt);
		if ( AssetTypeActions.IsValid() )
		{
			if ( !IsAssetTypeActionsInUse(AssetTypeActions) )
			{
				return false;
			}
		}
	}

	return true;
}

void SExtFilterList::GetTypeActionsForCategory(EAssetTypeCategories::Type Category, TArray< TWeakPtr<IAssetTypeActions> >& TypeActions) const
{
	// Load the asset tools module
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	TArray<TWeakPtr<IAssetTypeActions>> AssetTypeActionsList;
	AssetToolsModule.Get().GetAssetTypeActionsList(AssetTypeActionsList);

	// Find all asset type actions that match the category
	for (int32 ClassIdx = 0; ClassIdx < AssetTypeActionsList.Num(); ++ClassIdx)
	{
		const TWeakPtr<IAssetTypeActions>& WeakTypeActions = AssetTypeActionsList[ClassIdx];
		TSharedPtr<IAssetTypeActions> AssetTypeActions = WeakTypeActions.Pin();

		if ( ensure(AssetTypeActions.IsValid()) && AssetTypeActions->CanFilter() && AssetTypeActions->GetCategories() & Category)
		{
			TypeActions.Add(WeakTypeActions);
		}
	}
}

void SExtFilterList::FrontendFilterClicked(TSharedRef<FExtFrontendFilter> FrontendFilter)
{
	if ( IsFrontendFilterInUse(FrontendFilter) )
	{
		RemoveFilter(FrontendFilter);
	}
	else
	{
		TSharedRef<SExtFilter> NewFilter = AddFilter(FrontendFilter);
		NewFilter->SetEnabled(true);
	}
}

bool SExtFilterList::IsFrontendFilterInUse(TSharedRef<FExtFrontendFilter> FrontendFilter) const
{
	for ( auto FilterIt = Filters.CreateConstIterator(); FilterIt; ++FilterIt)
	{
		TSharedPtr<FExtFrontendFilter> Filter = (*FilterIt)->GetFrontendFilter();
		if ( Filter.IsValid() && Filter == FrontendFilter )
		{
			return true;
		}
	}

	return false;
}

void SExtFilterList::FrontendFilterCategoryClicked(TSharedPtr<FExtFrontendFilterCategory> MenuCategory)
{
	bool bFullCategoryInUse = IsFrontendFilterCategoryInUse(MenuCategory);
	bool ExecuteOnFilterChanged = false;
	for ( auto FrontendFilterIt = AllFrontendFilters.CreateConstIterator(); FrontendFilterIt; ++FrontendFilterIt )
	{
		const TSharedRef<FExtFrontendFilter>& FrontendFilter = *FrontendFilterIt;
		
		if(FrontendFilter->GetCategory() == MenuCategory)
		{
			if ( bFullCategoryInUse )
			{
				RemoveFilter( FrontendFilter, false );
				ExecuteOnFilterChanged = true;
			}
			else if ( !IsFrontendFilterInUse( FrontendFilter ) )
			{
				TSharedRef<SExtFilter> NewFilter = AddFilter( FrontendFilter );
				NewFilter->SetEnabled(true, false);
				SetFrontendFilterActive(FrontendFilter, NewFilter->IsEnabled());
				ExecuteOnFilterChanged = true;
			}
		}
	}

	if (ExecuteOnFilterChanged)
	{
		OnFilterChanged.ExecuteIfBound();
	}
}

bool SExtFilterList::IsFrontendFilterCategoryInUse(TSharedPtr<FExtFrontendFilterCategory> MenuCategory) const
{
	for ( auto FrontendFilterIt = AllFrontendFilters.CreateConstIterator(); FrontendFilterIt; ++FrontendFilterIt )
	{
		const TSharedRef<FExtFrontendFilter>& FrontendFilter = *FrontendFilterIt;

		if ( FrontendFilter->GetCategory() == MenuCategory && !IsFrontendFilterInUse( FrontendFilter ) )
		{
			return false;
		}
	}

	return true;
}

void SExtFilterList::OnResetFilters()
{
	RemoveAllFilters();
}

#undef LOCTEXT_NAMESPACE

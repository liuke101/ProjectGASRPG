#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"


static FName NAME_TagName(TEXT("TagName"));
static FName NAME_AbilitieAsset(TEXT("AbilitieAsset"));
static FName NAME_TriggerSource(TEXT("TriggerSource"));


class FGASLookAssetBase
{
public:

	virtual ~FGASLookAssetBase(){};

public:

	// 当前Tag名字
	// Current tag name
	virtual FName GetTagName() const = 0;

	// 当前资源名
	// Current asset name
	virtual FName GetAbilitieAsset() const = 0;

	// 资源指针
	// Resource pointer
	virtual UObject* GetAbilitieAssetObj() const = 0;

	// 当前Tag响应的事件发生器
	// Event generator for current tag response
	virtual FText GetTriggerSourceName() const = 0;

public:

	FGASLookAssetBase(){};
};

class SGASLookAssetTreeItem : public SMultiColumnTableRow<TSharedRef<FGASLookAssetBase>>
{
public:

	SLATE_BEGIN_ARGS(SGASLookAssetTreeItem)
		: _WidgetInfoToVisualize()
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASLookAssetBase>, WidgetInfoToVisualize)
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

public:

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

protected:
	void HandleHyperlinkNavigate();

protected:
	/** 关于我们正在可视化的小部件的信息 */
	// Information about the widget we are visualizing
	TSharedPtr<FGASLookAssetBase> WidgetInfo;

	FName TagName;
	FName AbilitieAsset;
	FText TriggerSourceName;
	UObject* LookAssObj;
};

class FGASLookAsset : public FGASLookAssetBase
{
public:
	virtual ~FGASLookAsset() override;

	static TSharedRef<FGASLookAsset> Create(UObject* AssObj, FAbilityTriggerData ActivationTag);

public:

	virtual FName GetTagName() const override;

	virtual FName GetAbilitieAsset() const override;

	virtual UObject* GetAbilitieAssetObj() const override;
	virtual FText GetTriggerSourceName() const override;
private:

	explicit FGASLookAsset(UObject* AssObj, FAbilityTriggerData ActivationTag);

protected:
	UObject* LookAssObj;
	FAbilityTriggerData ActivationTag;
};

DECLARE_DELEGATE_OneParam(FOnLookAssetDel,FGameplayTag)
class SGASTagViewItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASTagViewItem)
	{}
		SLATE_ATTRIBUTE( FGameplayTag, TagName)
		SLATE_EVENT(FOnLookAssetDel,OnLookAssetDel)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

public:

	TAttribute<FGameplayTag> TagName;

protected:

	FReply OnRemoveTagClicked();

	FText OnTipTag() const;

protected:
	FOnLookAssetDel OnLookAssetDel;
};
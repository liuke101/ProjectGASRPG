#pragma once
#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_PlayMontageAndWaitForGameplayEvent.generated.h"


class UMageAbilitySystemComponent;
/** 如果来自 Montage 回调，EventTag和Payload可能为空 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayMontageAndWaitForGameplayEventDelegate, FGameplayTag, EventTag, FGameplayEventData, Payload);

/**
 * 此 Task 将 PlayMontageAndWait 和 WaitForEvent 结合，因此您可以等待多种类型的激活，例如来自近战连击
 * 这些代码的大部分是从这两个 AbilityTask 中复制的
 * 这是一个很好的 Task ，当创建特定于游戏的 Task 时，可以查看它作为示例
 */
UCLASS()
class PROJECTGASRPG_API UAbilityTask_PlayMontageAndWaitForGameplayEvent : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool AbilityEnded) override;

	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForGameplayEventDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForGameplayEventDelegate OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForGameplayEventDelegate OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForGameplayEventDelegate OnCancelled;

	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForGameplayEventDelegate EventReceived;

	/**
	 * 播放 montage 并等待它结束。如果发生与EventTags匹配的游戏事件（或EventTags为空），则EventReceived委托将触发(附带event tag 和 payload)
	 * 如果StopWhenAbilityEnds为true，当 Ability 正常结束，会中止此 montage 。当能力被明确取消时，它总是会停止。
	 *
	 * 在正常执行时:
	 * - 当 montage 混合时，调用OnBlendOut，
	 * - 当 montage 完全播放时，调用OnCompleted
	 * - 如果另一个 montage 覆盖了此 montage ，调用OnInterrupted
	 * - 如果 Ability 或 Task 被取消，调用OnCancelled
	 * @param OwningAbility 此 Task 所属的 Ability
	 * @param TaskInstanceName 设置为覆盖此 Task 的名称，以供以后查询
	 * @param MontageToPlay 要在角色上播放的 montage
	 * @param EventTags 与此标记匹配的任何 GameplayEvent 都将激活EventReceived回调。如果为空，则所有事件都会触发回调
	 * @param Rate montage 播放速度
	 * * @param StartSection 如果不为空，则从命名的montage部分开始
	 * @param bStopWhenAbilityEnds 如果为true，则如果Ability 正常结束，则会中止此 montage 。当能力被明确取消时，它总是会停止
	 *  @param AnimRootMotionTranslationScale 更改以修改根运动的大小，或将其设置为0以完全阻止它
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_PlayMontageAndWaitForGameplayEvent* PlayMontageAndWaitForGameplayEvent(
			UGameplayAbility* OwningAbility,
			FName TaskInstanceName,
			UAnimMontage* MontageToPlay,
			FGameplayTagContainer EventTags,
			float Rate = 1.f,
			FName StartSection = NAME_None,
			bool bStopWhenAbilityEnds = true,
			float AnimRootMotionTranslationScale = 1.f);

private:
	/** 要播放的montage */
	UPROPERTY()
	UAnimMontage* MontageToPlay;

	/** 与 gameplay events 匹配的 Tag 列表  */
	UPROPERTY()
	FGameplayTagContainer EventTags;

	/** montage 播放速度 */
	UPROPERTY()
	float Rate = 1.0f;

	/** 蒙太奇的 Start Section  */
	UPROPERTY()
	FName StartSection;

	/** 修改根运动的大小 */
	UPROPERTY()
	float AnimRootMotionTranslationScale = 1.0f;

	/** 如果Ability结束，蒙太奇应该中止 */
	UPROPERTY()
	bool bStopWhenAbilityEnds = true;

	/** 检查 Ability 是否正在播放蒙太奇并停止该蒙太奇，如果蒙太奇已停止则返回 true，否则返回 false。*/
	bool StopPlayingMontage() const;

	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted) const;
	void OnAbilityCancelled();
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle CancelledHandle;
	FDelegateHandle EventHandle;
	
};

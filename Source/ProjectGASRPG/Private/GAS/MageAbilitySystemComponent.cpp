#include "GAS/MageAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "K2Node_InputKeyEvent.h"
#include "Character/MageCharacter.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"
#include "GAS/Data/AbilityDataAsset.h"


void UMageAbilitySystemComponent::AbilityInputTagStarted(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	ABILITYLIST_SCOPE_LOCK();
	
	for (auto& AbilitySpec : GetActivatableAbilities()) 
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) 
			{
				AbilitySpecInputPressed(AbilitySpec); // 通知AbilitySpec输入被按下
				InputConfirm();
			
				if (!AbilitySpec.IsActive()) //如果Ability没有激活
				{
					TryActivateAbility(AbilitySpec.Handle); //激活Ability
				}
				// Wait Input Press
				if (AbilitySpec.IsActive()) 
				{
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
				}
			}
	}
}

void UMageAbilitySystemComponent::AbilityInputTagOngoing(const FGameplayTag& InputTag)
{
}

void UMageAbilitySystemComponent::AbilityInputTagTriggered(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	ABILITYLIST_SCOPE_LOCK();

	for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) 
		{
			AbilitySpecInputPressed(AbilitySpec);
			InputConfirm();
			if (!AbilitySpec.IsActive()) 
			{
				TryActivateAbility(AbilitySpec.Handle); 
			}
			// Wait Input Press
			if (AbilitySpec.IsActive()) 
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
			}
		}
	}
}

void UMageAbilitySystemComponent::AbilityInputTagCanceled(const FGameplayTag& InputTag)
{
}

void UMageAbilitySystemComponent::AbilityInputTagCompleted(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	ABILITYLIST_SCOPE_LOCK();

	for (auto& AbilitySpec : GetActivatableAbilities()) 
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag) && AbilitySpec.IsActive()) 
		{
			AbilitySpecInputReleased(AbilitySpec); // 通知AbilitySpec输入被释放
			InputCancel();
			// Wait Input Release
			if (AbilitySpec.IsActive()) 
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
			}
		}
	}
}

void UMageAbilitySystemComponent::BindEffectAppliedCallback()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::BroadcastEffectAssetTags);
}

void UMageAbilitySystemComponent::GiveCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& CharacterAbilities)
{
	for (const auto AbilityClass : CharacterAbilities)
	{
		//创建GA Spec
		FGameplayAbilitySpec AbilitySpec(AbilityClass); 
		
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->StartupAbilityLevel; //设置技能等级
			
			/** 将 GA 的 Tag 添加到AbilitySpec, 这些 Tag 将与输入的 Tag 进行匹配*/
			AbilitySpec.DynamicAbilityTags.AddTag(MageGameplayAbility->StartupInputTag);

			/** 设置AbilityStateTag */
			AbilitySpec.DynamicAbilityTags.AddTag(FMageGameplayTags::Instance().Ability_State_Equipped);
			
			/** 授予Ability */
			GiveAbility(AbilitySpec); //授予后不激活
		}
	}

	/** 将所有授予的Ability的 AbilityInfo 广播给 OverlayWidgetController */
	bCharacterAbilitiesGiven = true;
	AbilitiesGiven.Broadcast();
}

void UMageAbilitySystemComponent::GivePassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassiveAbilities)
{
	for (const auto AbilityClass : PassiveAbilities)
	{
		
		FGameplayAbilitySpec AbilitySpec(AbilityClass); 
		
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->StartupAbilityLevel; //设置技能等级
			
			/** 授予后立即激活一次（被动技能持续激活，激活但不EndAbility） */
			GiveAbilityAndActivateOnce(AbilitySpec); 
		}
	}
}

void UMageAbilitySystemComponent::ForEachAbility(const FForEachAbilityDelegate& AbilityDelegate)
{
	/** 注意：Ability在运行时可以改变状态，例如取消激活，被某个Tag block。因此遍历 ActivatableAbilities 时, 要锁定该列表，直到遍历完成 */
	ABILITYLIST_SCOPE_LOCK();
	
	//遍历可激活的Ability
	for(const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		//执行委托,AbilitySpec为参数
		if(!AbilityDelegate.ExecuteIfBound(AbilitySpec))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,FString::Printf(TEXT("未能执行委托: %hs"), __FUNCTION__));
		}
	}
}

FGameplayTag UMageAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if(UGameplayAbility* Ability = AbilitySpec.Ability.Get())
	{
		for(FGameplayTag AbilityTag : Ability->AbilityTags)
		{
			if(AbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ability"))))
			{
				return AbilityTag;
			}
		}
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag UMageAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for(FGameplayTag InputTag : AbilitySpec.DynamicAbilityTags) //DynamicAbilityTags 在 GiveCharacterAbilities时添加
	{
		if(InputTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Input"))))
		{
			return InputTag;
		}
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag UMageAbilitySystemComponent::GetStateTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for(FGameplayTag AbilityStateTag : AbilitySpec.DynamicAbilityTags) //DynamicAbilityTags 在 GiveCharacterAbilities时添加
	{
		if(AbilityStateTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ability.State"))))
		{
			return AbilityStateTag;
		}
	}
	return FGameplayTag::EmptyTag;
}

FGameplayAbilitySpec* UMageAbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
	ABILITYLIST_SCOPE_LOCK();
	
	//遍历可激活的Ability
	for(FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		for (FGameplayTag Tag : AbilitySpec.Ability.Get()->AbilityTags) //记得设置GA的AbilityTag
		{
			if (Tag.MatchesTag(AbilityTag))
			{
				return &AbilitySpec;
			}
		}
	}
	return nullptr;
}

FGameplayTag UMageAbilitySystemComponent::GetInputTagFromAbilityTag(const FGameplayTag& AbilityTag)
{
	if(const FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(AbilityTag))
	{
		return GetInputTagFromSpec(*Spec);
	}
	return FGameplayTag::EmptyTag;
}


FGameplayTag UMageAbilitySystemComponent::GetStateTagFromAbilityTag(const FGameplayTag& AbilityTag)
{
	if(const FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(AbilityTag))
	{
		return GetStateTagFromSpec(*Spec);
	}
	return FGameplayTag::EmptyTag;
}

void UMageAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag) const
{
	if(IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		// 如果有属性点
		if(PlayerInterface->GetAttributePoint() > 0 )
		{
			// 减少属性点
			PlayerInterface->AddToAttributePoint(-1);
			
			// 使用GameplayEvent升级属性
			FGameplayEventData Payload;
			Payload.EventTag = AttributeTag;
			Payload.EventMagnitude = 1;

			//发送到GA_ListenForEvent
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(),AttributeTag,Payload);

			
		}
	}
}


void UMageAbilitySystemComponent::UpdateAbilityState(int32 Level)
{
	/** 等级达到技能要求时，将技能状态设置为可学习 */
	UAbilityDataAsset* AbilityDataAsset = UMageAbilitySystemLibrary::GetAbilityDataAsset(GetAvatarActor());
	for(auto Info :AbilityDataAsset->AbilityInfos)
	{
		if(!Info.AbilityTag.IsValid() || Level < Info.LevelRequirement) continue;
		
		if(GetSpecFromAbilityTag(Info.AbilityTag) == nullptr)
		{
			FGameplayAbilitySpec AbilitySpec(Info.AbilityClass, Info.AbilityClass.GetDefaultObject()->GetAbilityLevel()); //设置初始技能等级，由GA的StartupAbilityLevel设置，一般为1
			
			AbilitySpec.DynamicAbilityTags.AddTag(FMageGameplayTags::Instance().Ability_State_Trainable); // 可学习
			GiveAbility(AbilitySpec);
			MarkAbilitySpecDirty(AbilitySpec); //强制复制到客户端，不用等待下一次更新
			
			//广播 AbilityTag,StateTag 到 SkillTreeWidgetController
			AbilityStateChanged.Broadcast(Info.AbilityTag, FMageGameplayTags::Instance().Ability_State_Trainable, AbilitySpec.Level);
			//ClientUpdateAbilityState(Info.AbilityTag, FMageGameplayTags::Get().Ability_State_Trainable, AbilitySpec.Level);
		}
	}
}

void UMageAbilitySystemComponent::LearnSkill(const FGameplayTag& AbilityTag)
{
	if(FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		// 技能加点需要消耗技能点
		if(IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
		{
			PlayerInterface->AddToSkillPoint(-1);
		}
		
		const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
		FGameplayTag StateTag = GetStateTagFromSpec(*AbilitySpec);
		
		if(StateTag.MatchesTagExact(MageGameplayTags.Ability_State_Trainable))
		{
			AbilitySpec->DynamicAbilityTags.RemoveTag(MageGameplayTags.Ability_State_Trainable);
			AbilitySpec->DynamicAbilityTags.AddTag(MageGameplayTags.Ability_State_Unlocked);
			StateTag = MageGameplayTags.Ability_State_Unlocked; //更新StateTag
		}
		else if(StateTag.MatchesTagExact(MageGameplayTags.Ability_State_Unlocked) || StateTag.MatchesTagExact(MageGameplayTags.Ability_State_Equipped))
		{
			++AbilitySpec->Level;
		}

		MarkAbilitySpecDirty(*AbilitySpec);

		//广播 AbilityTag,StateTag 到 SkillTreeWidgetController
		AbilityStateChanged.Broadcast(AbilityTag, StateTag, AbilitySpec->Level);
	}
}

bool UMageAbilitySystemComponent::GetDescriptionByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription,
	FString& OutNextLevelDescription)
{
	if(const FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		if(UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec->Ability))
		{
			OutDescription = MageGameplayAbility->GetDescription(AbilitySpec->Level);
			OutNextLevelDescription = MageGameplayAbility->GetNextLevelDescription(AbilitySpec->Level);
			return true;	
		}
	}
	
	//BUG:客户端无法执行
	// 没有找到AbilitySpec，说明是未解锁的技能
	if(GetOwnerRole() == ROLE_Authority)
	{
		const UAbilityDataAsset* AbilityDataAsset = UMageAbilitySystemLibrary::GetAbilityDataAsset(GetAvatarActor());

		// 如果没有AbilityTag，什么也不显示
		if(!AbilityTag.IsValid() || AbilityTag.MatchesTagExact(FMageGameplayTags::Instance().Ability_None))
		{
			OutDescription = FString();
		}
		else // 如果有AbilityTag，显示解锁等级
		{
			OutDescription = UMageGameplayAbility::GetLockedDescription(AbilityDataAsset->FindAbilityInfoForTag(AbilityTag).LevelRequirement); //将LevelRequirement作为参数传入
		}
		
		OutNextLevelDescription = FString(); //无下一级描述
		return false;
	}
	return false;
}

void UMageAbilitySystemComponent::EquipSkill(const FGameplayTag& AbilityTag,
	const FGameplayTag& SlotInputTag)
{
	//BUG:装备技能时，无法立刻切换AbilityState显示的图标
	if(FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
		
		const FGameplayTag& PrevSlotInputTag = GetInputTagFromSpec(*AbilitySpec);
		const FGameplayTag& StateTag = GetStateTagFromSpec(*AbilitySpec);

		// 如果技能可装备
		if(StateTag.MatchesTagExact(MageGameplayTags.Ability_State_Unlocked) || StateTag.MatchesTagExact(MageGameplayTags.Ability_State_Equipped))
		{
			// Slot不为空，即该Slot已经装备了技能
			if(!SlotIsEmpty(SlotInputTag))
			{
				// 获取已经装备的技能
				if(FGameplayAbilitySpec* SpecWithSlot = GetSpecWithSlot(SlotInputTag)) 
				{
					// 如果该技能已经装备, 不做处理，直接返回
					if(AbilityTag.MatchesTagExact(GetAbilityTagFromSpec(*SpecWithSlot)))
					{
						// 广播信息到WBP_EquippedSkillTree
						SkillEquipped.Broadcast(AbilityTag, StateTag, SlotInputTag, PrevSlotInputTag);
						return;
					}

					// 如果该技能还没装备，需要对已经装备的技能进行处理
					// 如果已装备的是被动技能, 取消激活该技能
					if(IsPassiveAbility(*SpecWithSlot))
					{
						DeactivatePassiveAbility.Broadcast(GetAbilityTagFromSpec(*SpecWithSlot));
					}
					
					// 更新已装备技能的AbilityState: Equipped -> Unlocked
					if(GetStateTagFromSpec(*SpecWithSlot).MatchesTagExact(MageGameplayTags.Ability_State_Equipped))
					{
						SpecWithSlot->DynamicAbilityTags.RemoveTag(MageGameplayTags.Ability_State_Equipped);
						SpecWithSlot->DynamicAbilityTags.AddTag(MageGameplayTags.Ability_State_Unlocked);
						AbilityStateChanged.Broadcast(GetAbilityTagFromSpec(*SpecWithSlot), MageGameplayTags.Ability_State_Unlocked, SpecWithSlot->Level);
					}

					// 清空已经分配的SlotInputTag
					ClearSlotInputTag(SpecWithSlot);
				}
			}
			
			// Slot为空，即该Slot未装备技能
			else
			{
				// 如果技能还没有任何SlotInputTag(没有激活)
				if(!AbilityHasAnySlot(*AbilitySpec))
				{
					// 如果是被动技能，激活
					if(IsPassiveAbility(*AbilitySpec))
					{
						TryActivateAbility(AbilitySpec->Handle); 
					}
				}
				
				//更新该技能的AbilityState
				if(StateTag.MatchesTagExact(MageGameplayTags.Ability_State_Unlocked))
				{
					AbilitySpec->DynamicAbilityTags.RemoveTag(MageGameplayTags.Ability_State_Unlocked);
					AbilitySpec->DynamicAbilityTags.AddTag(MageGameplayTags.Ability_State_Equipped);
					AbilityStateChanged.Broadcast(AbilityTag, MageGameplayTags.Ability_State_Equipped, AbilitySpec->Level);
				}

				// 将该SlotInputTag分配给该Ability
				AssignSlotToAbility(SlotInputTag, *AbilitySpec);
			}
			MarkAbilitySpecDirty(*AbilitySpec);
		}

		// ClientRPC 广播信息到WBP_EquippedSkillTree
		SkillEquipped.Broadcast(AbilityTag, StateTag, SlotInputTag, PrevSlotInputTag);
	}
}

bool UMageAbilitySystemComponent::SlotIsEmpty(const FGameplayTag& SlotInputTag)
{
	ABILITYLIST_SCOPE_LOCK();
	
	for(const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if(AbilityHasExactSlot(Spec, SlotInputTag))
		{
			return false;
		}
	}
	return true;
}

bool UMageAbilitySystemComponent::AbilityHasExactSlot(const FGameplayAbilitySpec& Spec, const FGameplayTag& SlotInputTag)
{
	return Spec.DynamicAbilityTags.HasTagExact(SlotInputTag);
}

bool UMageAbilitySystemComponent::AbilityHasAnySlot(const FGameplayAbilitySpec& Spec)
{
	return Spec.DynamicAbilityTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Input")));
}

FGameplayAbilitySpec* UMageAbilitySystemComponent::GetSpecWithSlot(const FGameplayTag& SlotInputTag)
{
	ABILITYLIST_SCOPE_LOCK();
	
	for(FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if(AbilityHasExactSlot(Spec, SlotInputTag))
		{
			return &Spec;
		}
	}
	return nullptr;
}

bool UMageAbilitySystemComponent::IsPassiveAbility(const FGameplayAbilitySpec& Spec) const
{
	const UAbilityDataAsset* AbilityDataAsset = UMageAbilitySystemLibrary::GetAbilityDataAsset(GetAvatarActor());
	const FGameplayTag AbilityTag = GetAbilityTagFromSpec(Spec);
	const FGameplayTag AbilityTypeTag = AbilityDataAsset->FindAbilityInfoForTag(AbilityTag).TypeTag;
	if(AbilityTypeTag.MatchesTagExact(FMageGameplayTags::Instance().Ability_Type_Passive))
	{
		return true;
	}
	return false;
}

void UMageAbilitySystemComponent::AssignSlotToAbility(const FGameplayTag& SlotInputTag, FGameplayAbilitySpec& Spec)
{
	ClearSlotInputTag(&Spec);
	Spec.DynamicAbilityTags.AddTag(SlotInputTag);
}

void UMageAbilitySystemComponent::ClearSlotInputTag(FGameplayAbilitySpec* Spec)
{
	const FGameplayTag& SlotInputTag = GetInputTagFromSpec(*Spec);
	Spec->DynamicAbilityTags.RemoveTag(SlotInputTag);
}

void UMageAbilitySystemComponent::ClearAbilityOfSlotInputTag(const FGameplayTag& SlotInputTag)
{
	ABILITYLIST_SCOPE_LOCK();
	
	for(FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if(AbilityHasSlotInputTag(&Spec, SlotInputTag))
		{
			ClearSlotInputTag(&Spec);
		}
	}
}

bool UMageAbilitySystemComponent::AbilityHasSlotInputTag(FGameplayAbilitySpec* Spec, const FGameplayTag& SlotInputTag)
{
	for(FGameplayTag Tag : Spec->DynamicAbilityTags)
	{
		if(Tag.MatchesTagExact(SlotInputTag))
		{
			return true;
		}
	}
	return false;
}

void UMageAbilitySystemComponent::BroadcastEffectAssetTags(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer); 
	
	/* 广播 Tag 到 OverlayWidgetController */
	EffectAssetTags.Broadcast(TagContainer);

	//debug
	for (const FGameplayTag& Tag : TagContainer)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green, FString::Printf(TEXT("Effect Applied! GE Tag: %s"), *Tag.ToString()));
	}
}

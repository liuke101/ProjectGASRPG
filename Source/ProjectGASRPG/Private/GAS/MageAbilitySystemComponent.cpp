#include "GAS/MageAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/MageCharacter.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"
#include "GAS/Data/AbilityDataAsset.h"


void UMageAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);
	
	for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) //标签匹配
			{
				AbilitySpecInputPressed(AbilitySpec); // 通知AbilitySpec输入被按下
		
				if (AbilitySpec.IsActive()) //如果Ability激活
				{
					// 调用 InputPressed 事件。这里不进行复制。如果有人在监听，他们可能会将 InputPressed 事件复制到服务器。
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey()); 
				}
			}
	}
}

void UMageAbilitySystemComponent::AbilityInputTagHold(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);

	for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) //标签匹配
		{
			AbilitySpecInputPressed(AbilitySpec); // 通知AbilitySpec输入被按下
			if (!AbilitySpec.IsActive()) //如果Ability没有激活
			{
				TryActivateAbility(AbilitySpec.Handle); //激活Ability
			}
		}
	}
}

void UMageAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLoc(*this);

	for (auto& AbilitySpec : GetActivatableAbilities()) 
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag) && AbilitySpec.IsActive() && AbilitySpec.IsActive()) 
		{
			AbilitySpecInputReleased(AbilitySpec); // 通知AbilitySpec输入被释放

			//调用 InputReleased 事件
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey()); 
		}
	}
}

void UMageAbilitySystemComponent::BindEffectCallbacks()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::ClientEffectAppliedToSelfCallback);
}

void UMageAbilitySystemComponent::GiveCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& CharacterAbilities)
{
	for (const auto AbilityClass : CharacterAbilities)
	{
		//AbilityClass转化为MageGameplayAbility
		FGameplayAbilitySpec AbilitySpec(AbilityClass); 
		
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->StartupAbilityLevel; //设置技能等级
			
			/** 将 GA 的 Tag 添加到AbilitySpec, 这些 Tag 将与输入的 Tag 进行匹配*/
			AbilitySpec.DynamicAbilityTags.AddTag(MageGameplayAbility->StartupInputTag);

			/** 设置AbilityStateTag */
			AbilitySpec.DynamicAbilityTags.AddTag(FMageGameplayTags::Get().Ability_State_Equipped);
			
			/** 授予Ability */
			GiveAbility(AbilitySpec); //授予后不激活
		}
	}

	/**
	 * 将所有授予的Ability的 AbilityInfo 广播给 OverlayWidgetController
	 *  - 由于GiveCharacterAbilities函数只在服务器运行, 我们重载 OnRep_ActivateAbilities() 实现在客户端广播
	 */
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
			
			/** 授予后立即激活一次（被动技能持续激活，激活一次但不EndAbility） */
			GiveAbilityAndActivateOnce(AbilitySpec); 
		}
	}
}

void UMageAbilitySystemComponent::ForEachAbility(const FForEachAbilityDelegate& AbilityDelegate)
{
	/** 注意：Ability在运行时可以改变状态，例如取消激活，被某个Tag block。因此循环【激活列表】时, 要锁定该列表，直到循环完成 */
	FScopedAbilityListLock ActiveScopeLock(*this);
	
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
	/** 注意：Ability在运行时可以改变状态，例如取消激活，被某个Tag block。因此循环【激活列表】时, 要锁定该列表，直到循环完成 */
	FScopedAbilityListLock ActiveScopeLock(*this);
	
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

void UMageAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if(const IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		// 如果有属性点
		if(PlayerInterface->GetAttributePoint() > 0 )
		{
			ServerUpgradeAttribute(AttributeTag); // 服务器执行升级属性
		}
	}
}

void UMageAbilitySystemComponent::ServerUpgradeAttribute(const FGameplayTag& AttributeTag)
{
	// 使用GameplayEvent升级属性
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(),AttributeTag,Payload);

	// 减少属性点
	if(IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		PlayerInterface->AddToAttributePoint(-1);
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
			
			AbilitySpec.DynamicAbilityTags.AddTag(FMageGameplayTags::Get().Ability_State_Trainable); // 可学习
			GiveAbility(AbilitySpec);
			MarkAbilitySpecDirty(AbilitySpec); //强制复制到客户端，不用等待下一次更新
			ClientUpdateAbilityState(Info.AbilityTag, FMageGameplayTags::Get().Ability_State_Trainable, AbilitySpec.Level);
		}
	}
}

void UMageAbilitySystemComponent::ServerLearnSkill_Implementation(const FGameplayTag& AbilityTag)
{
	if(FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		// 技能加点需要消耗技能点
		if(IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
		{
			PlayerInterface->AddToSkillPoint(-1);
		}
		
		const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Get();
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
		ClientUpdateAbilityState(AbilityTag, StateTag, AbilitySpec->Level);
	}
}

bool UMageAbilitySystemComponent::ServerLearnSkill_Validate(const FGameplayTag& AbilityTag)
{
	return true;
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
		if(!AbilityTag.IsValid() || AbilityTag.MatchesTagExact(FMageGameplayTags::Get().Ability_None))
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

void UMageAbilitySystemComponent::ServerEquipSkill_Implementation(const FGameplayTag& AbilityTag,
	const FGameplayTag& SlotInputTag)
{
	//BUG:装备技能时，无法立刻切换AbilityState显示的图标
	if(FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
	{
		const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Get();
		
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
						// ClientRPC 广播信息到WBP_EquippedSkillTree
						ClientEquipSkill(AbilityTag, StateTag, SlotInputTag, PrevSlotInputTag);
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
		ClientEquipSkill(AbilityTag, StateTag, SlotInputTag, PrevSlotInputTag);
	}
}

bool UMageAbilitySystemComponent::ServerEquipSkill_Validate(const FGameplayTag& AbilityTag, const FGameplayTag& SlotInputTag)
{
	return true;
}

void UMageAbilitySystemComponent::ClientEquipSkill_Implementation(const FGameplayTag& AbilityTag,
	const FGameplayTag& AbilityStateTag, const FGameplayTag& SlotInputTag, const FGameplayTag& PreSlotInputTag)
{
	SkillEquipped.Broadcast(AbilityTag, AbilityStateTag, SlotInputTag, PreSlotInputTag);
}

bool UMageAbilitySystemComponent::ClientEquipSkill_Validate(const FGameplayTag& AbilityTag,
	const FGameplayTag& AbilityStateTag, const FGameplayTag& SlotInputTag, const FGameplayTag& PreSlotInputTag)
{
	return	true;
}

bool UMageAbilitySystemComponent::SlotIsEmpty(const FGameplayTag& SlotInputTag)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
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
	FScopedAbilityListLock ActiveScopeLock(*this);
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
	if(AbilityTypeTag.MatchesTagExact(FMageGameplayTags::Get().Ability_Type_Passive))
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
	FScopedAbilityListLock ActiveScopeLock(*this);
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


void UMageAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	if(!bCharacterAbilitiesGiven)
	{
		bCharacterAbilitiesGiven = true;
		AbilitiesGiven.Broadcast();
	}
}

void UMageAbilitySystemComponent::ClientUpdateAbilityState_Implementation(const FGameplayTag& AbilityTag,
                                                                          const FGameplayTag& StateTag, int32 AbilityLevel) const
{
	//广播 AbilityTag,StateTag 到 SkillTreeWidgetController
	AbilityStateChanged.Broadcast(AbilityTag, StateTag, AbilityLevel);
}


void UMageAbilitySystemComponent::ClientEffectAppliedToSelfCallback_Implementation(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const
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

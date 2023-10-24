// 


#include "GAS/MageAbilitySystemLibrary.h"
#include "Game/MageGameMode.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/Ability/MageGameplayAbility.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MagePlayerState.h"
#include "UI/HUD/MageHUD.h"
#include "UI/WidgetController/MageWidgetController.h"\


UOverlayWidgetController* UMageAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if(APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject,0))
	{
		if(AMageHUD* MageHUD = Cast<AMageHUD>(PC->GetHUD()))
		{
			AMagePlayerState* PS = Cast<AMagePlayerState>(PC->PlayerState);
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AttributeSet = PS->GetAttributeSet();
			return MageHUD->GetOverlayWidgetController(FWidgetControllerParams(PC, PS, ASC, AttributeSet));	
		}
	}
	return nullptr;
}

UAttributeMenuWidgetController* UMageAbilitySystemLibrary::GetAttributeMenuWidgetController(
	const UObject* WorldContextObject)
{
	if(APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject,0))
	{
		if(AMageHUD* MageHUD = Cast<AMageHUD>(PC->GetHUD()))
		{
			AMagePlayerState* PS = Cast<AMagePlayerState>(PC->PlayerState);
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			UAttributeSet* AttributeSet = PS->GetAttributeSet();
			return MageHUD->GetAttributeMenuWidgetController(FWidgetControllerParams(PC, PS, ASC, AttributeSet));	
		}
	}
	return nullptr;
}

void UMageAbilitySystemLibrary::InitDefaultAttributes(const UObject* WorldContextObject,
	ECharacterClass CharacterClass, const int32 Level, UAbilitySystemComponent* ASC)
{
	/** 使用GE初始化Attribute */
	UCharacterClassDataAsset* CharacterClassDataAsset = GetCharacterClassDataAsset(WorldContextObject);
		
	const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetClassDefaultInfo(CharacterClass);
	
	ApplyEffectToSelf(ASC, CharacterClassDefaultInfo.PrimaryAttribute.Get(), Level);
	ApplyEffectToSelf(ASC, CharacterClassDefaultInfo.SecondaryAttribute.Get(), Level);
	ApplyEffectToSelf(ASC, CharacterClassDataAsset->VitalAttribute.Get(), Level); // VitalAttribute基于SecondaryAttribute生成初始值，所以先让SecondaryAttribute初始化
}

void UMageAbilitySystemLibrary::ApplyEffectToSelf(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> GameplayEffectClass, const float Level)
{
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(ASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
	const FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);
	const FActiveGameplayEffectHandle ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
}

void UMageAbilitySystemLibrary::GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC)
{
	UCharacterClassDataAsset* CharacterClassDataAsset = GetCharacterClassDataAsset(WorldContextObject);

	//授予所有CommonAbilities
	for(const auto CommonAbility:CharacterClassDataAsset->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(CommonAbility); 
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->AbilityLevel; //设置技能等级
			
			/** 授予Ability */
			ASC->GiveAbility(AbilitySpec); //授予后不激活
			//GiveAbilityAndActivateOnce(AbilitySpec); //授予并立即激活一次
		}

		//另一种方式：
		// if (UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(CommonAbility.GetDefaultObject()))
		// {
		// 	FGameplayAbilitySpec AbilitySpec(MageGameplayAbility, MageGameplayAbility->AbilityLevel); //设置技能等级
		// 	
		// 	/** 授予Ability */
		// 	ASC->GiveAbility(AbilitySpec); //授予后不激活
		// }
	}
}

UCharacterClassDataAsset* UMageAbilitySystemLibrary::GetCharacterClassDataAsset(const UObject* WorldContextObject)
{
	if(const AMageGameMode* MageGameMode = Cast<AMageGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		checkf(MageGameMode->CharacterClassDataAsset, TEXT("CharacterClassDataAsset为空, 请在 MageGameMode 中设置"));
		
		return MageGameMode->CharacterClassDataAsset;
	}
	
	return nullptr;
}

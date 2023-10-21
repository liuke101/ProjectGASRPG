// 


#include "GAS/MageAbilitySystemLibrary.h"
#include "Game/MageGameMode.h"
#include "GAS/MageAbilitySystemComponent.h"
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
	ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	/** 使用GE初始化Attribute */
	if(const AMageGameMode* MageGameMode = Cast<AMageGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		UCharacterClassDataAsset* CharacterClassDataAsset = MageGameMode->CharacterClassDataAsset;
		checkf(CharacterClassDataAsset, TEXT("CharacterClassDataAsset为空, 请在 MageGameMode 中设置"));
		
		const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetClassDefaultInfo(CharacterClass);
		
		// Primary Attributes
		FGameplayEffectContextHandle PrimaryAttributeGEContextHandle = ASC->MakeEffectContext();
		PrimaryAttributeGEContextHandle.AddSourceObject(ASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
		const FGameplayEffectSpecHandle PrimaryAttributeGESpecHandle = ASC->MakeOutgoingSpec(CharacterClassDefaultInfo.PrimaryAttribute.Get(), Level, PrimaryAttributeGEContextHandle);
		ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributeGESpecHandle.Data.Get());
		
		// Secondary Attributes
		FGameplayEffectContextHandle SecondaryAttributeGEContextHandle = ASC->MakeEffectContext();
		SecondaryAttributeGEContextHandle.AddSourceObject(ASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
		const FGameplayEffectSpecHandle SecondaryAttributeGESpecHandle = ASC->MakeOutgoingSpec(CharacterClassDefaultInfo.SecondaryAttribute.Get(), Level, SecondaryAttributeGEContextHandle);
		ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributeGESpecHandle.Data.Get());
	
		// Vital Attributes
		FGameplayEffectContextHandle VitalAttributeGEContextHandle = ASC->MakeEffectContext();
		VitalAttributeGEContextHandle.AddSourceObject(ASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
		const FGameplayEffectSpecHandle VitalAttributeGESpecHandle = ASC->MakeOutgoingSpec(CharacterClassDataAsset->VitalAttribute.Get(), Level, VitalAttributeGEContextHandle);
		ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributeGESpecHandle.Data.Get());
		
	}
}

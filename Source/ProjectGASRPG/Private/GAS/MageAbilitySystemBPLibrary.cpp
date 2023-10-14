// 


#include "GAS/MageAbilitySystemBPLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MagePlayerState.h"
#include "UI/HUD/MageHUD.h"
#include "UI/WidgetController/MageWidgetController.h"

UOverlayWidgetController* UMageAbilitySystemBPLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
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

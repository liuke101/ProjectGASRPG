#include "Game/MageAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "GAS/MageGameplayTags.h"

UMageAssetManager& UMageAssetManager::Get()
{
	check(GEngine);
	UMageAssetManager* MageAssetManager = Cast<UMageAssetManager>(GEngine->AssetManager);
	return *MageAssetManager;
}

void UMageAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	FMageGameplayTags::InitNativeGameplayTags();

	/** TargetData必须执行该操作，否则造成ScriptStructCache错误 */
	UAbilitySystemGlobals::Get().InitGlobalData();
}

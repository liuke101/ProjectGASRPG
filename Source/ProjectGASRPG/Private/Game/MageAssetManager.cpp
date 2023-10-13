#include "Game/MageAssetManager.h"
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
}

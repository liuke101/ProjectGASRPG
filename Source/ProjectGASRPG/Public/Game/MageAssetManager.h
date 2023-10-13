// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "MageAssetManager.generated.h"

UCLASS()
class PROJECTGASRPG_API UMageAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UMageAssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};

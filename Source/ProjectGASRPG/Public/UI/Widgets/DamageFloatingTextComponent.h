// 

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "DamageFloatingTextComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UDamageFloatingTextComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDamageFloatingText(float Damage);
};

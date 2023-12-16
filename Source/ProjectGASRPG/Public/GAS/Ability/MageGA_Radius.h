#pragma once

#include "CoreMinimal.h"
#include "MageGA_Damage.h"
#include "MageGA_Radius.generated.h"

/** 球形技能 */
UCLASS()
class PROJECTGASRPG_API UMageGA_Radius : public UMageGA_Damage
{
	GENERATED_BODY()

public:
	//拖拽Actor，可实现推、拉、击退等效果
	UFUNCTION(BlueprintCallable, Category = "Mage_GA|Radius")
	void DragActor(AActor* TargetActor,FVector Direction,float ForceMagnitude ) const;
};

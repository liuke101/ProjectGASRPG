#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NiagaraComponent.h"
#include "DebuffNiagaraComponent.generated.h"

struct FGameplayTag;

UCLASS()
class PROJECTGASRPG_API UDebuffNiagaraComponent : public UNiagaraComponent
{
	GENERATED_BODY()
public:
	UDebuffNiagaraComponent();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MageComponent|Component|VFX")
	FGameplayTag DebuffTag;

protected:
	virtual void BeginPlay() override;

	/** 拥有者DebuffTag改变时回调 */
	UFUNCTION()
	void DebuffChangedCallback(const FGameplayTag DebuffTypeTag, int32 NewCount);

	/** 拥有者死亡时回调 */
	UFUNCTION()
	void OwnerDeathCallback(AActor* DeadActor);
};

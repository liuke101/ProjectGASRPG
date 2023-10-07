#include "GAS/GameplayEffect/MageEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "GAS/MageAttributeSet.h"


AMageEffectActor::AMageEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;


	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SetRootComponent(StaticMeshComponent);
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetupAttachment(RootComponent);

	
}

void AMageEffectActor::OnSphereComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(OtherActor))
	{
		
		if(const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(ASInterface->GetAbilitySystemComponent()->GetAttributeSet(UMageAttributeSet::StaticClass())))
		{
			//TODO: change this to apply a Gameplay Effect.For now，using const_cast as a hack!
			UMageAttributeSet* MutableMageAttributeSet = const_cast<UMageAttributeSet*>(MageAttributeSet);
			MutableMageAttributeSet->SetHealth(MageAttributeSet->GetHealth() + 10.0f);
			Destroy();
		}
	}
	
}

void AMageEffectActor::OnSphereComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

void AMageEffectActor::BeginPlay()
{
	Super::BeginPlay();
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this,&AMageEffectActor::OnSphereComponentBeginOverlap);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this,&AMageEffectActor::OnSphereComponentEndOverlap);
}



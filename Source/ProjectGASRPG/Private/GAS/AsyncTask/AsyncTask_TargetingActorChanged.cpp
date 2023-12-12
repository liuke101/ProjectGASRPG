#include "GAS/AsyncTask/AsyncTask_TargetingActorChanged.h"

#include "Interface/CombatInterface.h"
#include "Player/MagePlayerController.h"


UAsyncTask_TargetingActorChanged* UAsyncTask_TargetingActorChanged::ListenForTargetingActorChanged(
	APlayerController* PlayerController)
{
	UAsyncTask_TargetingActorChanged* AsyncTargetingActorChanged = NewObject<UAsyncTask_TargetingActorChanged>();
	AsyncTargetingActorChanged->PlayerController = PlayerController;

	
	if(!IsValid(PlayerController))
	{
		AsyncTargetingActorChanged->EndTask();
		return nullptr;
	}

	// 绑定属性变化委托
	if(AMagePlayerController* MagePlayerController = Cast<AMagePlayerController>(PlayerController))
	{
		MagePlayerController->OnTargetingActorChanged.AddUObject(AsyncTargetingActorChanged, &UAsyncTask_TargetingActorChanged::TargetingActorChangedCallback);
	}
	
	return AsyncTargetingActorChanged;
}



void UAsyncTask_TargetingActorChanged::EndTask()
{
	if(AMagePlayerController* MagePlayerController = Cast<AMagePlayerController>(PlayerController))
	{
		MagePlayerController->OnTargetingActorChanged.RemoveAll(this);
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTask_TargetingActorChanged::TargetingActorChangedCallback(AActor* NewTargetingActor,
	AActor* OldTargetingActor) const
{
	OnTargetingChanged.Broadcast(NewTargetingActor, OldTargetingActor);
	
}

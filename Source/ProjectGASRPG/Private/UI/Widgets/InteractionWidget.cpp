#include "UI/Widgets/InteractionWidget.h"
#include "Components/ProgressBar.h"

void UInteractionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	InteractionProgressBar->PercentDelegate.BindUFunction(this, "UpdateInteractionProgress");
}

void UInteractionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentInteractionDuration = 0.f;
}

float UInteractionWidget::UpdateInteractionProgress()
{
	return 0.0f;
}



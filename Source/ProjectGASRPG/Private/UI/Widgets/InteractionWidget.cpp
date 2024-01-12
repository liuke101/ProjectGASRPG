#include "UI/Widgets/InteractionWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UInteractionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	InteractionProgressBar->PercentDelegate.BindUFunction(this, "UpdateInteractionProgress");
}

void UInteractionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	KerPressText->SetText(FText::FromString("Press"));
	CurrentInteractionDuration = 0.f;
}

void UInteractionWidget::UpdateWidget(const FInteractableData& InteractableData)
{
	switch (InteractableData.InteractableType)
	{
	case EInteractableType::Pickup:
		KerPressText->SetText(FText::FromString("Press"));
		InteractionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		if(InteractableData.Quantity >= 1)
		{
			QuantityText->SetText(FText::Format(FText::FromString("x{0}"), InteractableData.Quantity));
			QuantityText->SetVisibility(ESlateVisibility::Visible);
		}
		
		break;
	case EInteractableType::Device:
		break;
	case EInteractableType::NPC:
		break;
	case EInteractableType::Toggle:
		break;
	case EInteractableType::Container:
		break;
	default: ;
	}

	ActionText->SetText(InteractableData.Action);
	NameText->SetText(InteractableData.Name);
}

float UInteractionWidget::UpdateInteractionProgress()
{
	return 0.0f;
}



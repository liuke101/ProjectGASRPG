// 


#include "UI/Widgets/MageUserWidget.h"

void UMageUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	OnSetWidgetController(); 
}

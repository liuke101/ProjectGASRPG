// Description:switch the language displayed in the editor
// Author:Jiecool
// Date:2022/9/16
// Email:jiecool@qq.com

#include "SwitchLanguageCommands.h"

#define LOCTEXT_NAMESPACE "FSwitchLanguageModule"

void FSwitchLanguageCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "SwitchLanguage", "Execute SwitchLanguage action", EUserInterfaceActionType::Button, FInputChord(EKeys::L));
}

#undef LOCTEXT_NAMESPACE

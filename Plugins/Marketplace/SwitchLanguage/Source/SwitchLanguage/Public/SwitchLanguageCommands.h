// Description:switch the language displayed in the editor
// Author:Jiecool
// Date:2022/9/16
// Email:jiecool@qq.com

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SwitchLanguageStyle.h"

class FSwitchLanguageCommands : public TCommands<FSwitchLanguageCommands>
{
public:

	FSwitchLanguageCommands()
		: TCommands<FSwitchLanguageCommands>(TEXT("SwitchLanguage"), NSLOCTEXT("Contexts", "SwitchLanguage", "SwitchLanguage Plugin"), NAME_None, FSwitchLanguageStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};

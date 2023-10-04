// Description:switch the language displayed in the editor
// Author:Jiecool
// Date:2022/9/16
// Email:jiecool@qq.com

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FSwitchLanguageModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	void RefreshBlueprints();
	TArray<UObject*>EditedAssets;
	
private:

	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};

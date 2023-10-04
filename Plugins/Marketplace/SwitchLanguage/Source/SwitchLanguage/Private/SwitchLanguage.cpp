// Description:switch the language displayed in the editor
// Author:Jiecool
// Date:2022/9/16
// Email:jiecool@qq.com

#include "SwitchLanguage.h"
#include "SwitchLanguageStyle.h"
#include "SwitchLanguageCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "LevelEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"


static const FName SwitchLanguageTabName("SwitchLanguage");

#define LOCTEXT_NAMESPACE "FSwitchLanguageModule"

void FSwitchLanguageModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FSwitchLanguageStyle::Initialize();
	FSwitchLanguageStyle::ReloadTextures();

	FSwitchLanguageCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSwitchLanguageCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FSwitchLanguageModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedRef<FUICommandList> ExPluginCommands = LevelEditorModule.GetGlobalLevelEditorActions();
	ExPluginCommands->Append(PluginCommands.ToSharedRef());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSwitchLanguageModule::RegisterMenus));
}

void FSwitchLanguageModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSwitchLanguageStyle::Shutdown();

	FSwitchLanguageCommands::Unregister();
}

void FSwitchLanguageModule::PluginButtonClicked()
{
	if (UKismetInternationalizationLibrary::GetCurrentLanguage() == "en")
	{
		UKismetInternationalizationLibrary::SetCurrentLanguage("zh-Hans");
	}
	else if (UKismetInternationalizationLibrary::GetCurrentLanguage().StartsWith("zh"))
	{
		UKismetInternationalizationLibrary::SetCurrentLanguage("en");
	}
	else
	{
		UKismetInternationalizationLibrary::SetCurrentLanguage("en");
	}

	RefreshBlueprints();
}

void FSwitchLanguageModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FSwitchLanguageCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
				Entry.Name = "SwitchLanguageButton";
				Entry.Label = FText::FromString(TEXT("SwitchLanguage"));
				Entry.ToolTip = FText::FromString(TEXT("点击按钮，可切换中英文显示"));
			}
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("AssetEditor.DefaultToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FSwitchLanguageCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
				Entry.Name = "SwitchLanguageButton";
				Entry.Label = FText::FromString(TEXT(""));
				Entry.ToolTip = FText::FromString(TEXT("点击按钮，切换编辑器的显示语言"));
			}
		}
	}

}

void FSwitchLanguageModule::RefreshBlueprints()
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();

	if (EditedAssets.Num() > 0)
	{
		for (UObject* Data : EditedAssets)
		{
			TWeakObjectPtr<UBlueprint> Blueprint = Cast<UBlueprint>(Data);
			FBlueprintEditorUtils::RefreshAllNodes(Blueprint.Get());
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSwitchLanguageModule, SwitchLanguage)
// Description:switch the language displayed in the editor
// Author:Jiecool
// Date:2022/9/16
// Email:jiecool@qq.com

#include "SwitchLanguageStyle.h"
#include "SwitchLanguage.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FSwitchLanguageStyle::StyleInstance = nullptr;

void FSwitchLanguageStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSwitchLanguageStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSwitchLanguageStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SwitchLanguageStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FSwitchLanguageStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SwitchLanguageStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SwitchLanguage")->GetBaseDir() / TEXT("Resources"));

	Style->Set("SwitchLanguage.PluginAction", new IMAGE_BRUSH_SVG(TEXT("SwitchLanIcon"), Icon20x20));
	return Style;
}

void FSwitchLanguageStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSwitchLanguageStyle::Get()
{
	return *StyleInstance;
}

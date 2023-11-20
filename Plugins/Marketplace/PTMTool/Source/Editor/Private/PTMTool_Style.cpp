/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
///			Copyright 2018 (C) Bruno Xavier B. Leite
//////////////////////////////////////////////////////////////

#include "PTMTool_Style.h"
#include "PTMTool_Shared.h"

#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUGIN_BRUSH(RelativePath,...) FSlateImageBrush(FPTM_Style::InContent(RelativePath,".png"),__VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TSharedPtr<FSlateStyleSet> FPTM_Style::StyleSet = nullptr;
TSharedPtr<ISlateStyle> FPTM_Style::Get() {return StyleSet;}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FString FPTM_Style::InContent(const FString &RelativePath, const ANSICHAR* Extension) {
	static FString Content = IPluginManager::Get().FindPlugin(PLUGIN_NAME)->GetContentDir();
	return (Content/RelativePath)+Extension;
}

FName FPTM_Style::GetStyleSetName() {
	static FName StyleName(TEXT("FPTM_Style"));
	return StyleName;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FPTM_Style::Initialize() {
	if (StyleSet.IsValid()) {return;}
	//
	const FVector2D Icon16x16(16.0f,16.0f);
	const FVector2D Icon20x20(20.0f,20.0f);
	const FVector2D Icon40x40(40.0f,40.0f);
	const FVector2D Icon128x128(128.0f,128.0f);
	//
	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	StyleSet->Set("PTMTool_Window.Tab",new PLUGIN_BRUSH(TEXT("Icons/PTMTab_16x"),Icon16x16));
	StyleSet->Set("PTMTool.SpawnPTMToolWindow",new PLUGIN_BRUSH(TEXT("Icons/PTMTab_128x"),Icon40x40));
	StyleSet->Set("PTMTool.SpawnPTMToolWindow.Small",new PLUGIN_BRUSH(TEXT("Icons/PTMTab_128x"),Icon20x20));
	//
	//
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

void FPTM_Style::Shutdown() {
	if (StyleSet.IsValid()) {
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef PLUGIN_BRUSH

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:switch the language displayed in the editor
// Author:Jiecool
// Date:2022/9/16
// Email:jiecool@qq.com

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FSwitchLanguageStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Shooter game */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};
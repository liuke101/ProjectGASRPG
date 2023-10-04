// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**  */
class FExtContentBrowserStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Shooter game */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

	static FSlateFontInfo GetFontStyle(FName PropertyName, const ANSICHAR* Specifier = NULL);

	static FDocumentationStyle GetChangLogDocumentationStyle();


public:
	// For debug
	static FLinearColor CustomContentBrowserBorderBackgroundColor;
	static FLinearColor CustomToolbarBackgroundColor;
	static FLinearColor CustomSourceViewBackgroundColor;
	static FLinearColor CustomAssetViewBackgroundColor;

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};

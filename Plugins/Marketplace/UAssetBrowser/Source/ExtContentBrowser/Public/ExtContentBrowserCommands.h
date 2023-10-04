// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ExtContentBrowserStyle.h"

class FExtContentBrowserCommands : public TCommands<FExtContentBrowserCommands>
{
public:

	FExtContentBrowserCommands()
		: TCommands<FExtContentBrowserCommands>(TEXT("UAssetBrowser"), NSLOCTEXT("Contexts", "UAssetBrowser", "UAsset Browser"), NAME_None, FExtContentBrowserStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenUAssetBrowser;
	TSharedPtr< FUICommandInfo > ImportSelectedUAsset;
	TSharedPtr< FUICommandInfo > ToggleDependencyViewer;
};

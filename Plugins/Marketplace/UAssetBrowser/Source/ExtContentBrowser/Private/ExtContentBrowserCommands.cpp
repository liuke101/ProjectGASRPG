// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtContentBrowserCommands.h"

#define LOCTEXT_NAMESPACE "FExtContentBrowserModule"

void FExtContentBrowserCommands::RegisterCommands()
{
	UI_COMMAND(OpenUAssetBrowser, "UAsset", "Open UAsset Browser", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ImportSelectedUAsset, "Import Selected UAsset", "Import Selected UAsset File", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleDependencyViewer, "Toggle Dependency Viewer", "Toggle the display of Dependency Viewer", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE

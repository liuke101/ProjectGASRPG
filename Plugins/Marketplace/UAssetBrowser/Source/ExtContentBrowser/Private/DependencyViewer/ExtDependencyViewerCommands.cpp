// Copyright 2017-2021 marynate. All Rights Reserved.

#include "ExtDependencyViewerCommands.h"

//////////////////////////////////////////////////////////////////////////
// FExtDependencyViewerCommands

#define LOCTEXT_NAMESPACE "ExtDependencyViewerCommands"

FExtDependencyViewerCommands::FExtDependencyViewerCommands() : TCommands<FExtDependencyViewerCommands>(
	"ExtDependencyViewerCommands",
	NSLOCTEXT("Contexts", "ExtDependencyViewerCommands", "Dependency Viewer Commands"),
	NAME_None, 
	FAppStyle::GetAppStyleSetName())
{
}

void FExtDependencyViewerCommands::RegisterCommands()
{
	UI_COMMAND(ZoomToFitSelected, "Zoom to Fit", "Zoom in and center the view on the selected item", EUserInterfaceActionType::Button, FInputChord(EKeys::Home));
}

#undef LOCTEXT_NAMESPACE

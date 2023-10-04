// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

// Actions that can be invoked in the dependency viewer
class FExtDependencyViewerCommands : public TCommands<FExtDependencyViewerCommands>
{
public:
	FExtDependencyViewerCommands();

	// TCommands<> interface
	virtual void RegisterCommands() override;
	// End of TCommands<> interface

	/** Zoom in to fit the selected objects in the window */
	TSharedPtr<FUICommandInfo> ZoomToFitSelected;
};

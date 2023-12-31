// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "AssetRegistry/AssetData.h"

#include "ExtAssetData.h"

/** Common Content Browser drag-drop handler logic */
namespace DragDropHandler
{
	DECLARE_DELEGATE_ThreeParams(FExecuteCopyOrMove, TArray<FAssetData> /*AssetList*/, TArray<FString> /*AssetPaths*/, FString /*TargetPath*/);

	/** Used by OnDragEnter, OnDragOver, and OnDrop to check and update the validity of a drag-drop operation on an asset folder in the Content Browser */
	bool ValidateDragDropOnAssetFolder(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, const FString& TargetPath, bool& OutIsKnownDragOperation);

	/** Handle assets or asset paths being dropped onto an asset folder in the Content Browser - this drop should have been externally validated by ValidateDragDropOnAssetFolder */
	void HandleDropOnAssetFolder(const TSharedRef<SWidget>& ParentWidget, const TArray<FAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& TargetPath, const FText& TargetDisplayName, FExecuteCopyOrMove CopyActionHandler, FExecuteCopyOrMove MoveActionHandler, FExecuteCopyOrMove AdvancedCopyActionHandler);

	///////////////////////////////////////////
	// DragDropHandler for ExtAssetData
	//

	DECLARE_DELEGATE_ThreeParams(FExecuteDropToImport, TArray<FExtAssetData> /*AssetList*/, TArray<FString> /*AssetPaths*/, FString /*TargetPath*/);

	/** Handle assets or asset paths being dropped onto an asset folder in the Content Browser - this drop should have been externally validated by ValidateDragDropOnAssetFolder */
	void HandleDropOnAssetFolderOfContentBrowser(const TSharedRef<SWidget>& ParentWidget, const TArray<FExtAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& TargetPath, const FText& TargetDisplayName, FExecuteDropToImport ImportActionHandler, FExecuteDropToImport ImportFlatActionHandler);
}

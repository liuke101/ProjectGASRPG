// Copyright 2017-2021 marynate. All Rights Reserved.

#include "DragDropHandler.h"
#include "ExtContentBrowserUtils.h"

#include "Layout/WidgetPath.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "DragAndDrop/AssetDragDropOp.h"

#define LOCTEXT_NAMESPACE "ExtContentBrowser"

bool DragDropHandler::ValidateDragDropOnAssetFolder(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, const FString& TargetPath, bool& OutIsKnownDragOperation)
{
	OutIsKnownDragOperation = false;

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (!Operation.IsValid())
	{
		return false;
	}

	bool bIsValidDrag = false;
	TOptional<EMouseCursor::Type> NewDragCursor;

	const bool bIsAssetPath = !ExtContentBrowserUtils::IsClassPath(TargetPath);

	if (Operation->IsOfType<FAssetDragDropOp>())
	{
		TSharedPtr<FAssetDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(Operation);
		const TArray<FAssetData>& DroppedAssets = DragDropOp->GetAssets();
		const TArray<FString>& DroppedAssetPaths = DragDropOp->GetAssetPaths();

		OutIsKnownDragOperation = true;

		int32 NumAssetItems, NumClassItems;
		ExtContentBrowserUtils::CountItemTypes(DroppedAssets, NumAssetItems, NumClassItems);

		int32 NumAssetPaths, NumClassPaths;
		ExtContentBrowserUtils::CountPathTypes(DroppedAssetPaths, NumAssetPaths, NumClassPaths);

		if (DroppedAssetPaths.Num() == 1 && DroppedAssetPaths[0] == TargetPath)
		{
			DragDropOp->SetToolTip(LOCTEXT("OnDragFoldersOverFolder_CannotSelfDrop", "Cannot move or copy a folder onto itself"), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error")));
		}
		else if (bIsAssetPath)
		{
			const int32 TotalAssetDropItems = NumAssetItems + NumAssetPaths;
			const int32 TotalClassDropItems = NumClassItems + NumClassPaths;

			if (TotalAssetDropItems > 0)
			{
				bIsValidDrag = true;

				const FText FirstItemText = DroppedAssets.Num() > 0 ? FText::FromName(DroppedAssets[0].AssetName) : FText::FromString(DroppedAssetPaths[0]);
				const FText MoveOrCopyText = (TotalAssetDropItems > 1)
					? FText::Format(LOCTEXT("OnDragAssetsOverFolder_MultipleAssetItems", "Move or copy '{0}' and {1} {1}|plural(one=other,other=others)"), FirstItemText, TotalAssetDropItems - 1)
					: FText::Format(LOCTEXT("OnDragAssetsOverFolder_SingularAssetItems", "Move or copy '{0}'"), FirstItemText);

				if (TotalClassDropItems > 0)
				{
					DragDropOp->SetToolTip(FText::Format(LOCTEXT("OnDragAssetsOverFolder_AssetAndClassItems", "{0}\n\n{1} C++ {1}|plural(one=item,other=items) will be ignored as they cannot be moved or copied"), MoveOrCopyText, NumClassItems), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OKWarn")));
				}
				else
				{
					DragDropOp->SetToolTip(MoveOrCopyText, FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")));
				}
			}
			else if (TotalClassDropItems > 0)
			{
				DragDropOp->SetToolTip(LOCTEXT("OnDragAssetsOverFolder_OnlyClassItems", "C++ items cannot be moved or copied"), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error")));
			}
		}
		else
		{
			DragDropOp->SetToolTip(FText::Format(LOCTEXT("OnDragAssetsOverFolder_InvalidFolder", "'{0}' is not a valid place to drop assets or folders"), FText::FromString(TargetPath)), FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error")));
		}
	}
	else if (Operation->IsOfType<FExternalDragOperation>())
	{
		TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>(Operation);
		OutIsKnownDragOperation = true;
		bIsValidDrag = DragDropOp->HasFiles() && bIsAssetPath;
	}

	// Set the default slashed circle if this drag is invalid and a drag operation hasn't set NewDragCursor to something custom
	if (!bIsValidDrag && !NewDragCursor.IsSet())
	{
		NewDragCursor = EMouseCursor::SlashedCircle;
	}
	Operation->SetCursorOverride(NewDragCursor);

	return bIsValidDrag;
}

void DragDropHandler::HandleDropOnAssetFolder(const TSharedRef<SWidget>& ParentWidget, const TArray<FAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& TargetPath, const FText& TargetDisplayName, FExecuteCopyOrMove CopyActionHandler, FExecuteCopyOrMove MoveActionHandler, FExecuteCopyOrMove AdvancedCopyActionHandler)
{
	// Remove any classes from the asset list
	TArray<FAssetData> FinalAssetList = AssetList;
	FinalAssetList.RemoveAll([](const FAssetData& AssetData)
	{
		return AssetData.AssetClassPath == FTopLevelAssetPath(TEXT("/Script/CoreUObject"), NAME_Class);
	});

	// Remove any class paths from the list
	TArray<FString> FinalAssetPaths = AssetPaths;
	FinalAssetPaths.RemoveAll([](const FString& AssetPath)
	{
		return ExtContentBrowserUtils::IsClassPath(AssetPath);
	});

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);
	const FText MoveCopyHeaderString = FText::Format(LOCTEXT("AssetViewDropMenuHeading", "Move/Copy to {0}"), TargetDisplayName);
	MenuBuilder.BeginSection("PathAssetMoveCopy", MoveCopyHeaderString);
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DragDropMove", "Move Here"),
			LOCTEXT("DragDropMoveTooltip", "Move the dragged items to this folder, preserving the structure of any copied folders."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([=]() { MoveActionHandler.ExecuteIfBound(FinalAssetList, FinalAssetPaths, TargetPath); }))
		);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DragDropCopy", "Copy Here"),
			LOCTEXT("DragDropCopyTooltip", "Copy the dragged items to this folder, preserving the structure of any copied folders."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([=]() { CopyActionHandler.ExecuteIfBound(FinalAssetList, FinalAssetPaths, TargetPath); }))
			);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DragDropAdvancedCopy", "Advanced Copy Here"),
			LOCTEXT("DragDropAdvancedCopyTooltip", "Copy the dragged items and any specified dependencies to this folder, afterwards fixing up any dependencies on copied files to the new files."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([=]() { AdvancedCopyActionHandler.ExecuteIfBound(FinalAssetList, FinalAssetPaths, TargetPath); }))
		);
	}
	MenuBuilder.EndSection();

	FSlateApplication::Get().PushMenu(
		ParentWidget,
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
		);
}

void DragDropHandler::HandleDropOnAssetFolderOfContentBrowser(const TSharedRef<SWidget>& ParentWidget, const TArray<FExtAssetData>& AssetList, const TArray<FString>& AssetPaths, const FString& TargetPath, const FText& TargetDisplayName, FExecuteDropToImport ImportActionHandler, FExecuteDropToImport ImportFlatActionHandler)
{
	// Remove any classes from the asset list
	TArray<FExtAssetData> FinalAssetList = AssetList;
	FinalAssetList.RemoveAll([](const FExtAssetData& AssetData)
	{
		//return AssetData.AssetClass == NAME_Class;
		return !AssetData.IsValid();
	});

	// Remove any class paths from the list
	TArray<FString> FinalAssetPaths = AssetPaths;
#if ECB_LEGACY
	FinalAssetPaths.RemoveAll([](const FString& AssetPath)
	{
		return ExtContentBrowserUtils::IsClassPath(AssetPath);
	});
#endif

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);
	
	const FText FlatImportHeaderString = FText::Format(LOCTEXT("ExtAssetViewDropMenuHeadingFlatImport", "Flat Import to {0}"), TargetDisplayName);
	MenuBuilder.BeginSection("AssetFlatImport", FlatImportHeaderString);
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DragDropFlatImport", "Flat Import to Current Folder"),
			LOCTEXT("DragDropFlatImportTooltip", "Import selected uassets and collect all dependencies to current folder."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([=]() { ImportFlatActionHandler.ExecuteIfBound(FinalAssetList, FinalAssetPaths, TargetPath); }))
		);
	}
	MenuBuilder.EndSection();

	const FText NormalImportHeaderString = FText::Format(LOCTEXT("ExtAssetViewDropMenuHeadingImport", "Import to Project"), TargetDisplayName);
	MenuBuilder.BeginSection("AssetImport", NormalImportHeaderString);
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DragDropImport", "Import to Current Project"),
			LOCTEXT("DragDropImportTooltip", "Import selected uassets to current project, keeping original asset paths."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([=]() { ImportActionHandler.ExecuteIfBound(FinalAssetList, FinalAssetPaths, TargetPath); }))
		);
	}
	MenuBuilder.EndSection();

	FSlateApplication::Get().PushMenu(
		ParentWidget,
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);
}

#undef LOCTEXT_NAMESPACE

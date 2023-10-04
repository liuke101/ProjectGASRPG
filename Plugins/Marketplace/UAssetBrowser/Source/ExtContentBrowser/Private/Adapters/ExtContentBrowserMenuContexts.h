// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "ExtContentBrowserMenuContexts.generated.h"

class FAssetContextMenu;
class IAssetTypeActions;

UCLASS()
class  UExtContentBrowserAssetContextMenuContext : public UObject
{
	GENERATED_BODY()

public:

	TWeakPtr<FAssetContextMenu> AssetContextMenu;

	TWeakPtr<IAssetTypeActions> CommonAssetTypeActions;
	
	UPROPERTY()
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

	UPROPERTY()
	UClass* CommonClass;

	UFUNCTION(BlueprintCallable, Category="Tool Menus")
	TArray<UObject*> GetSelectedObjects() const
	{
		TArray<UObject*> Result;
		Result.Reserve(SelectedObjects.Num());
		for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
		{
			Result.Add(Object.Get());
		}
		return Result;
	}

public:

	// Todo: GetSelectedObjects for FExtAssetData
	void SetNumSelectedObjects(int32 InNum)
	{
		NumSelectedObjects = InNum;
	}

	int32 GetNumSelectedObjects() const 
	{
		return NumSelectedObjects;
	}

private:
	int32 NumSelectedObjects = 0;
};


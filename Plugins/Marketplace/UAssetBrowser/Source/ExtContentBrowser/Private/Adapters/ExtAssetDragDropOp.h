// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Input/DragAndDrop.h"
#include "Layout/Visibility.h"
#include "DragAndDrop/DecoratedDragDropOp.h"

struct FExtAssetData;
class FExtAssetThumbnail;
class FExtAssetThumbnailPool;
class UActorFactory;

class FExtAssetDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FExtAssetDragDropOp, FDecoratedDragDropOp)

	static TSharedRef<FExtAssetDragDropOp> New(const FExtAssetData& InAssetData, UActorFactory* ActorFactory = nullptr);

	static TSharedRef<FExtAssetDragDropOp> New(TArray<FExtAssetData> InAssetData, UActorFactory* ActorFactory = nullptr);

	static TSharedRef<FExtAssetDragDropOp> New(FString InAssetPath);

	static TSharedRef<FExtAssetDragDropOp> New(TArray<FString> InAssetPaths);

	static TSharedRef<FExtAssetDragDropOp> New(TArray<FExtAssetData> InAssetData, TArray<FString> InAssetPaths, UActorFactory* ActorFactory = nullptr);

	/** @return true if this drag operation contains assets */
	bool HasAssets() const
	{
		return AssetData.Num() > 0;
	}

	/** @return true if this drag operation contains asset paths */
	bool HasAssetPaths() const
	{
		return AssetPaths.Num() > 0;
	}

	/** @return The assets from this drag operation */
	const TArray<FExtAssetData>& GetAssets() const
	{
		return AssetData;
	}

	/** @return The asset paths from this drag operation */
	const TArray<FString>& GetAssetPaths() const
	{
		return AssetPaths;
	}

	/** @return The actor factory to use if converting this asset to an actor */
	UActorFactory* GetActorFactory() const
	{
		return ActorFactory.Get();
	}

public:
	virtual ~FExtAssetDragDropOp();

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;

	FText GetDecoratorText() const;

private:
	void Init();

	/** Data for the assets this item represents */
	TArray<FExtAssetData> AssetData;

	/** Data for the asset paths this item represents */
	TArray<FString> AssetPaths;

	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<FExtAssetThumbnailPool> ThumbnailPool;

	/** Handle to the thumbnail resource */
	TSharedPtr<FExtAssetThumbnail> AssetThumbnail;

	/** The actor factory to use if converting this asset to an actor */
	TWeakObjectPtr<UActorFactory> ActorFactory;

	/** The size of the thumbnail */
	int32 ThumbnailSize;
};

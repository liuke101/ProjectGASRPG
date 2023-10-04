// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtAssetData.h"

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/GCObject.h"
#include "Misc/Paths.h"
#include "Misc/TextFilterUtils.h"

class UFactory;

namespace EExtAssetItemType
{
	enum Type
	{
		Normal,
		Folder,
		Creation,
		Duplication
	};
}

/** Base class for items displayed in the asset view */
struct FExtAssetViewItem
{
	FExtAssetViewItem()
		: bRenameWhenScrolledIntoview(false)
	{
	}

	virtual ~FExtAssetViewItem() {}

	/** Get the type of this asset item */
	virtual EExtAssetItemType::Type GetType() const = 0;

	/** Get whether this is a temporary item */
	virtual bool IsTemporaryItem() const = 0;

	/** Updates cached custom column data, does nothing by default */
	virtual void CacheCustomColumns(const TArray<FAssetViewCustomColumn>& CustomColumns, bool bUpdateSortData, bool bUpdateDisplayText, bool bUpdateExisting) {}

	/** Broadcasts whenever a rename is requested */
	FSimpleDelegate RenamedRequestEvent;

	/** Broadcasts whenever a rename is canceled */
	FSimpleDelegate RenameCanceledEvent;

	/** An event to fire when the asset data for this item changes */
	DECLARE_MULTICAST_DELEGATE( FOnAssetDataChanged );
	FOnAssetDataChanged OnAssetDataChanged;

	/** True if this item will enter inline renaming on the next scroll into view */
	bool bRenameWhenScrolledIntoview;
};

/** Item that represents an asset */
struct FExtAssetViewAsset : public FExtAssetViewItem
{
	/** The asset registry data associated with this item */
	FExtAssetData Data;

	/** Map of values for custom columns */
	TMap<FName, FString> CustomColumnData;

	/** Map of display text for custom columns */
	TMap<FName, FText> CustomColumnDisplayText;

	TCHAR FirstFewAssetNameCharacters[8];

	explicit FExtAssetViewAsset(const FExtAssetData& AssetData)
		: Data(AssetData)
	{
		SetFirstFewAssetNameCharacters();
	}

	void SetFirstFewAssetNameCharacters()
	{
		TextFilterUtils::FNameBufferWithNumber NameBuffer(Data.AssetName);
		if (NameBuffer.IsWide())
		{
			FCStringWide::Strncpy(FirstFewAssetNameCharacters, NameBuffer.GetWideNamePtr(), UE_ARRAY_COUNT(FirstFewAssetNameCharacters));
		}
		else
		{
			int32 NumChars = FMath::Min<int32>(FCStringAnsi::Strlen(NameBuffer.GetAnsiNamePtr()), UE_ARRAY_COUNT(FirstFewAssetNameCharacters) - 1);
			FPlatformString::Convert(FirstFewAssetNameCharacters, NumChars, NameBuffer.GetAnsiNamePtr(), NumChars);
			FirstFewAssetNameCharacters[NumChars] = 0;
		}
		FCStringWide::Strupr(FirstFewAssetNameCharacters);
	}

	void SetAssetData(const FExtAssetData& NewData)
	{
		Data = NewData;
		SetFirstFewAssetNameCharacters();

		OnAssetDataChanged.Broadcast();
	}

	bool GetTagValue(FName Tag, FString& OutString) const
	{
		const FString* FoundString = CustomColumnData.Find(Tag);

		if (FoundString)
		{
			OutString = *FoundString;
			return true;
		}

		return Data.GetTagValue(Tag, OutString);
	}

	// FExtAssetViewItem interface
	virtual EExtAssetItemType::Type GetType() const override
	{
		return EExtAssetItemType::Normal;
	}

	virtual bool IsTemporaryItem() const override
	{
		return false;
	}

	virtual void CacheCustomColumns(const TArray<FAssetViewCustomColumn>& CustomColumns, bool bUpdateSortData, bool bUpdateDisplayText, bool bUpdateExisting) override
	{
#if ECB_WIP_MORE_VIEWTYPE
		for (const FAssetViewCustomColumn& Column : CustomColumns)
		{
			if (bUpdateSortData)
			{
				if (bUpdateExisting ? CustomColumnData.Contains(Column.ColumnName) : !CustomColumnData.Contains(Column.ColumnName))
				{
					CustomColumnData.Add(Column.ColumnName, Column.OnGetColumnData.Execute(Data, Column.ColumnName));
				}
			}

			if (bUpdateDisplayText)
			{
				if (bUpdateExisting ? CustomColumnDisplayText.Contains(Column.ColumnName) : !CustomColumnDisplayText.Contains(Column.ColumnName))
				{
					if (Column.OnGetColumnDisplayText.IsBound())
					{
						CustomColumnDisplayText.Add(Column.ColumnName, Column.OnGetColumnDisplayText.Execute(Data, Column.ColumnName));
					}
					else
					{
						CustomColumnDisplayText.Add(Column.ColumnName, FText::AsCultureInvariant(Column.OnGetColumnData.Execute(Data, Column.ColumnName)));
					}
				}
			}
		}
#endif
	}
};

/** Item that represents a folder */
struct FExtAssetViewFolder : public FExtAssetViewItem
{
	/** The folder this item represents */
	FString FolderPath;

	/** The folder this item represents, minus the preceding path */
	FText FolderName;

	FExtAssetViewFolder(const FString& InPath)
		: FolderPath(InPath)
	{
		FolderName = FText::FromString(FPaths::GetBaseFilename(FolderPath));
	}

	/** Set the name of this folder (without path) */
	void SetFolderName(const FString& InName)
	{
		FolderPath = FPaths::GetPath(FolderPath) / InName;
		FolderName = FText::FromString(InName);
		OnAssetDataChanged.Broadcast();
	}

	// FExtAssetViewItem interface
	virtual EExtAssetItemType::Type GetType() const override
	{
		return EExtAssetItemType::Folder;
	}

	virtual bool IsTemporaryItem() const override
	{
		return false;
	}
};


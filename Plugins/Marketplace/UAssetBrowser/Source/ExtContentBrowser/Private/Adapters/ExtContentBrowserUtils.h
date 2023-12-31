// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Framework/SlateDelegates.h"
#include "AssetRegistry/AssetData.h"
#include "CollectionManagerTypes.h"
#include "Interfaces/IPluginManager.h"

#include "ExtContentBrowser.h"

struct FExtAssetData;
class SExtAssetView;
class SExtPathView;
class FViewport;

namespace ExtContentBrowserUtils
{
	enum class ECBFolderCategory : uint8
	{
		GameContent,
		EngineContent,
		PluginContent,
		DeveloperContent,

		GameClasses,
		EngineClasses,
		PluginClasses,

		ProjectContent,
		VaultCacheContent,
		OrphanContent,
	};

	/** Loads the specified object if needed and opens the asset editor for it */
	bool OpenEditorForAsset(const FString& ObjectPath);

	/** Opens the asset editor for the specified asset */
	bool OpenEditorForAsset(UObject* Asset);

	/** Opens the asset editor for the specified assets */
	bool OpenEditorForAsset(const TArray<UObject*>& Assets);

	/**
	  * Makes sure the specified assets are loaded into memory.
	  * 
	  * @param ObjectPaths The paths to the objects to load.
	  * @param LoadedObjects The returned list of objects that were already loaded or loaded by this method.
	  * @return false if user canceled after being warned about loading very many packages.
	  */
	bool LoadAssetsIfNeeded(const TArray<FString>& ObjectPaths, TArray<UObject*>& LoadedObjects, bool bAllowedToPromptToLoadAssets = true, bool bLoadRedirects = false);

	/**
	 * Determines the unloaded assets that need loading
	 *
	 * @param ObjectPaths		Paths to assets that may need to be loaded
	 * @param OutUnloadedObjects	List of the unloaded object paths
	 * @return true if the user should be prompted to load assets
	 */
	void GetUnloadedAssets(const TArray<FString>& ObjectPaths, TArray<FString>& OutUnloadedObjects);

	/**
	 * Prompts the user to load the list of unloaded objects
 	 *
	 * @param UnloadedObjects	The list of unloaded objects that we should prompt for loading
	 * @param true if the user allows the objects to be loaded
	 */
	bool PromptToLoadAssets(const TArray<FString>& UnloadedObjects);

	/** Checks to see if the given folder can be renamed */
	bool CanRenameFolder(const FString& InFolderPath);

	/** Checks to see if the given asset can be renamed */
	bool CanRenameAsset(const FAssetData& InAssetData);

	/** Renames an asset */
	void RenameAsset(UObject* Asset, const FString& NewName, FText& ErrorMessage);

	/** 
	 * Copies assets to a new path 
	 * @param Assets The assets to copy
	 * @param DestPath The destination folder in which to copy the assets
	 */
	void CopyAssets(const TArray<UObject*>& Assets, const FString& DestPath);

	/**
	  * Moves assets to a new path
	  * 
	  * @param Assets The assets to move
	  * @param DestPath The destination folder in which to move the assets
	  * @param SourcePath If non-empty, this will specify the base folder which will cause the move to maintain folder structure
	  */
	void MoveAssets(const TArray<UObject*>& Assets, const FString& DestPath, const FString& SourcePath = FString());

	/** Attempts to deletes the specified assets. Returns the number of assets deleted */
	int32 DeleteAssets(const TArray<UObject*>& AssetsToDelete);

	/** Attempts to delete the specified folders and all assets inside them. Returns true if the operation succeeded. */
	bool DeleteFolders(const TArray<FString>& PathsToDelete);

	/** Gets an array of assets inside the specified folders */
	void GetAssetsInPaths(const TArray<FString>& InPaths, TArray<FExtAssetData>& OutAssetDataList);

	/** Saves all the specified packages */
	bool SavePackages(const TArray<UPackage*>& Packages);

	/** Prompts to save all modified packages */
	bool SaveDirtyPackages();

	/** Loads all the specified packages */
	TArray<UPackage*> LoadPackages(const TArray<FString>& PackageNames);

	void NotifyMessage(const FText& Message,  bool bAlsoPrintToSonle = false, float InDuration = 3.f);

	void NotifyMessage(const FString& Message, float InDuration = 3.f);

	void DisplayMessagePopup(const FText& Message);

	/** Displays a modeless message at the specified anchor. It is fine to specify a zero-size anchor, just use the top and left fields */
	void DisplayMessage(const FText& Message, const FSlateRect& ScreenAnchor, const TSharedRef<SWidget>& ParentContent);

	/** Displays a modeless message asking yes or no type question */
	void DisplayConfirmationPopup(const FText& Message, const FText& YesString, const FText& NoString, const TSharedRef<SWidget>& ParentContent, const FOnClicked& OnYesClicked, const FOnClicked& OnNoClicked = FOnClicked());

#if ECB_LEGACY
	/** Moves all assets from the source path to the destination path, preserving path structure, deletes source path afterwards if possible */
	bool RenameFolder(const FString& DestPath, const FString& SourcePath);
#endif
	/** Copies all assets in all source paths to the destination path, preserving path structure */
	bool CopyFolders(const TArray<FString>& InSourcePathNames, const FString& DestPath);
#if ECB_LEGACY
	/** Moves all assets in all source paths to the destination path, preserving path structure */
	bool MoveFolders(const TArray<FString>& InSourcePathNames, const FString& DestPath);
#endif

	/**
	  * A helper function for folder drag/drop which loads all assets in a path (including sub-paths) and returns the assets found
	  * 
	  * @param SourcePathNames				The paths to the folders to drag/drop
	  * @param OutSourcePathToLoadedAssets	The map of source folder paths to assets found
	  */
	bool PrepareFoldersForDragDrop(const TArray<FString>& SourcePathNames, TMap< FString, TArray<UObject*> >& OutSourcePathToLoadedAssets);

	/** Copies references to the specified assets to the clipboard */
	void CopyAssetReferencesToClipboard(const TArray<FAssetData>& AssetsToCopy);

	/** Copies file paths on disk to the specified assets to the clipboard */
	void CopyFilePathsToClipboard(const TArray<FExtAssetData>& AssetsToCopy);

	/**
	 * Capture active viewport to thumbnail and assigns that thumbnail to incoming assets
	 *
	 * @param InViewport - viewport to sample from
	 * @param InAssetsToAssign - assets that should receive the new thumbnail ONLY if they are assets that use GenericThumbnails
	 */
	void CaptureThumbnailFromViewport(FViewport* InViewport, const TArray<FAssetData>& InAssetsToAssign);

	/**
	 * Clears custom thumbnails for the selected assets
	 *
	 * @param InAssetsToAssign - assets that should have their thumbnail cleared
	 */
	void ClearCustomThumbnails(const TArray<FAssetData>& InAssetsToAssign);

	/** Returns true if the specified asset that uses shared thumbnails has a thumbnail assigned to it */
	bool AssetHasCustomThumbnail( const FAssetData& AssetData );

	/** Extract the category of the given path */
	ECBFolderCategory GetFolderCategory( const FString& InPath );

	ECBFolderCategory GetRootFolderCategory(const FString& InPath);

	/** Returns true if the passed-in path is a engine folder */
	bool IsEngineFolder( const FString& InPath );

	/** Returns true if the passed-in path is a developers folder */
	bool IsDevelopersFolder( const FString& InPath );

	/** Returns true if the passed-in path is a plugin folder, optionally reporting where the plugin was loaded from */
	bool IsPluginFolder(const FString& InPath, EPluginLoadedFrom* OutPluginSource = nullptr);

	/** Returns true if the passed-in path is a plugin folder, optionally reporting where the plugin was loaded from.
	 *  Pass in a prefiltered list of plugins to consider -- more efficient when called many times.
	 */
	bool IsPluginFolder(const FString& InPath, const TArray<TSharedRef<IPlugin>>& InPlugins, EPluginLoadedFrom* OutPluginSource = nullptr);

	/** Returns true if the passed-in path is a C++ classes folder */
	bool IsClassesFolder( const FString& InPath );

	/** Returns true if the passed-in path is a localization folder */
	bool IsLocalizationFolder( const FString& InPath );

	/** Get all the objects in a list of asset data */
	void GetObjectsInAssetData(const TArray<FAssetData>& AssetList, TArray<UObject*>& OutDroppedObjects);

	/** Returns true if the supplied folder name can be used as part of a package name */
	bool IsValidFolderName(const FString& FolderName, FText& Reason);

	/** Returns true if the path specified exists as a folder in the asset registry */
	bool DoesFolderExist(const FString& FolderPath);

	/**
	 * @return true if the path specified is an empty folder (contains no assets or classes).
	 * @note Does *not* test whether the folder is empty on disk, so do not use it to validate filesystem deletion!
	 */
	bool IsEmptyFolder(const FString& FolderPath, const bool bRecursive = false);

	/** Check to see whether the given path is a root directory (either for asset or classes) */
	bool IsRootDir(const FString& FolderPath);

	/** Check to see whether the given path is a root asset directory */
	bool IsAssetRootDir(const FString& FolderPath);

	/** Check to see whether the given path is a root class directory */
	bool IsClassRootDir(const FString& FolderPath);

	/** Get the localized display name to use for the given root directory */
	FText GetRootDirDisplayName(const FString& FolderPath);

	/** If the folder is currently in background gathering progress */
	bool IsFolderBackgroundGathering(const FString& InFolder);

	FName GetCurrentGatheringFolder();

	/** Check to see whether the given path is rooted against a class directory */
	bool IsClassPath(const FString& InPath);

	/** Check to see whether the given path is rooted against a collection directory, optionally extracting the collection name and share type from the path */
	bool IsCollectionPath(const FString& InPath, FName* OutCollectionName = nullptr, ECollectionShareType::Type* OutCollectionShareType = nullptr);

	/** Given an array of paths, work out how many are rooted against class roots, and how many are rooted against asset roots */
	void CountPathTypes(const TArray<FString>& InPaths, int32& OutNumAssetPaths, int32& OutNumClassPaths);

	/** Given an array of paths, work out how many are rooted against class roots, and how many are rooted against asset roots */
	void CountPathTypes(const TArray<FName>& InPaths, int32& OutNumAssetPaths, int32& OutNumClassPaths);

	/** Given an array of "asset" data, work out how many are assets, and how many are classes */
	void CountItemTypes(const TArray<FAssetData>& InItems, int32& OutNumAssetItems, int32& OutNumClassItems);

	/** Check to see whether the given path is a valid location in which to create new classes */
	bool IsValidPathToCreateNewClass(const FString& InPath);

	/** Check to see whether the given path is a valid location in which to create a new folder */
	bool IsValidPathToCreateNewFolder(const FString& InPath);

	/**
	 * Loads the color of this path from the config
	 *
	 * @param FolderPath - The path to the folder
	 * @return The color the folder should appear as, will be NULL if not customized
	 */
	const TSharedPtr<FLinearColor> LoadColor(const FString& FolderPath, bool bNoCache = false);
	bool LoadFolderColor(const FString& FolderPath, FLinearColor& OutFolderColor);

	/**
	 * Saves the color of the path to the config
	 *
	 * @param FolderPath - The path to the folder
	 * @param FolderColor - The color the folder should appear as
	 * @param bForceAdd - If true, force the color to be added for the path
	 */
	void SaveColor(const FString& FolderPath, const TSharedPtr<FLinearColor>& FolderColor, bool bForceAdd = false);

	/**
	 * Checks to see if any folder has a custom color, optionally outputs them to a list
	 *
	 * @param OutColors - If specified, returns all the custom colors being used
	 * @return true, if custom colors are present
	 */
	bool HasCustomColors( TArray< FLinearColor >* OutColors = NULL );

	/** Gets the default color the folder should appear as */
	FLinearColor GetDefaultColor();

	/** Gets the platform specific text for the "explore" command (FPlatformProcess::ExploreFolder) */
	FText GetExploreFolderText();

#if ECB_DISABLE
	/** Returns true if the specified path is available for object creation */
	bool IsValidObjectPathForCreate(const FString& ObjectPath, FText& OutErrorMessage, bool bAllowExistingAsset = false);
#endif

	/** Returns true if the specified folder name in the specified path is available for folder creation */
	bool IsValidFolderPathForCreate(const FString& FolderPath, const FString& NewFolderName, FText& OutErrorMessage);

	/** Returns the length of the computed cooked package name and path wether it's run on a build machine or locally */
	int32 GetPackageLengthForCooking(const FString& PackageName, bool IsInternalBuild);

	/** Checks to see whether the path is within the size restrictions for cooking */
	bool IsValidPackageForCooking(const FString& PackageName, FText& OutErrorMessage);

	/** Returns if this folder has been marked as a favorite folder */
	bool IsFavoriteFolder(const FString& FolderPath);

	void AddFavoriteFolder(const FString& FolderPath, bool bFlushConfig = true);

	void RemoveFavoriteFolder(const FString& FolderPath, bool bFlushConfig = true);

	const TArray<FString>& GetFavoriteFolders();

	/** Gets the maximum path length for a cooked file. Changes behavior based on whether the editor experimental setting for long paths is enabled. */
	int32 GetMaxCookPathLen();

	void BeginAdvancedCopyPackages(TArray<FAssetData>& AssetList, TArray<FString>& AssetPaths, FString& DestinationPath);
}


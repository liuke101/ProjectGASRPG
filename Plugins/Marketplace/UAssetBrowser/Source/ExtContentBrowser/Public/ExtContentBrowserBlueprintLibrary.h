// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "ExtContentBrowser.h"
#include "ExtContentBrowserTypes.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "ExtContentBrowserBlueprintLibrary.generated.h"

UCLASS(meta = (ScriptName = "UAssetBrowserLibrary"))
class EXTCONTENTBROWSER_API UExtContentBrowserBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (Category = "UAssetBrowser|Import", AutoCreateRefTerm = "AssetFilePaths, ImportSetting"))
	static void ImportUAssets(const TArray<FString>& AssetFilePaths, const FUAssetImportSetting& ImportSetting, bool bOneByOne = false);

	UFUNCTION(BlueprintCallable, meta = (Category = "UAssetBrowser|Import", AutoCreateRefTerm = "ImportSetting"))
	static void ImportUAsset(const FString& AssetFilePath, const FUAssetImportSetting& ImportSetting);

	UFUNCTION(BlueprintCallable, meta = (Category = "UAssetBrowser|Import", AutoCreateRefTerm = "ImportSetting"))
	static void ImportFromFileList(const FString& FilePath, const FUAssetImportSetting& ImportSetting, bool bOneByOne = false);

	UFUNCTION(BlueprintPure, meta = (Category = "UAssetBrowser|Get"))
	static void GetGlobalImportSetting(FUAssetImportSetting& ImportSetting);

	UFUNCTION(BlueprintPure, meta = (Category = "UAssetBrowser|Get"))
	static void GetSelectedUAssetFilePaths(TArray<FString>& AssetFilePaths);

#if ECB_TODO
	//UFUNCTION(BlueprintPure, meta = (Category = "UAssetBrowser|Get"))
	static void GetUAssetFilePathsInFolder(FName FolderPath, TArray<FString>& AssetFilePaths, bool bRecursive = false);
#endif
};

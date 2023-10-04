// Copyright 2017-2021 marynate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "IDocumentationProvider.h"

#define ECB_WIP 0
#define ECB_DEBUG 0

#define ENABLE_ECB_LOG ECB_DEBUG & 1
#define ECB_FOLD 1					// for folding code section only
#define ECB_DISABLE 0				// for disable code block temporarily
#define ECB_TODO 0					// todo list
#define ECB_LEGACY 0				// legacy code holder, wait for review and remove

////////////////////////////////
// Finished Features
//
#define ECB_FEA_SEARCH 1
#define ECB_FEA_FILTER 1
#define ECB_FEA_ASYNC_FOLDER_DISCOVERY 1

// Import
#define ECB_FEA_IMPORT_ROLLBACK 1
#define ECB_FEA_IMPORT_OPTIONS 1
#define ECB_FEA_IMPORT_IGNORE_SOFT_ERROR 1
#define ECB_WIP_MULTI_IMPORT 1
#define ECB_WIP_IMPORT_ADD_TO_COLLECTION 1

// Dependency Viewer
#define ECB_FEA_REF_VIEWER 1
#define ECB_FEA_REF_VIEWER_OPTIONS ECB_FEA_REF_VIEWER & 1
#define ECB_FEA_REF_VIEWER_NODE_SPACING 1

// Asset Viewer
#define ECB_FEA_ENGINE_VERSION_OVERLAY 1
#define ECB_FEA_SHOW_INVALID  1

#define ECB_FEA_SYNC_IN_CB 1
#define ECB_FEA_VALIDATE 1
#define ECB_FEA_VALIDATE_OVERLAY 1

#define ECB_FEA_IMPORT_FLATMODE 1

#define ECB_FEA_TOOLBAR_BUTTON_AUTOHIDE 1

#define ECB_FEA_RELOAD_ROOT_FOLDER 1

// Redirector Support
#define ECB_FEA_REDIRECTOR_SUPPORT 1

// Import Plugin Content
#define ECB_FEA_IMPORT_PLUGIN 1

// Asset Viewer
#define ECB_FEA_ASSET_DRAG_DROP 1
#define ECB_FEA_IMPORT_ADD_PLACE 1

// Ignore Folders
#define ECB_FEA_IGNORE_FOLDERS 1

// Async
#define ECB_FEA_ASYNC_ASSET_DISCOVERY 1

////////////////////////////////
// Almost Done
//

// Import to Plugin instead of Project Content Folder
#define ECB_WIP_IMPORT_TO_PLUGIN_FOLDER 1

// Import Folder Color
#define ECB_WIP_IMPORT_FOLDER_COLOR 1
#define ECB_WIP_IMPORT_FOLDER_COLOR_OVERRIDE ECB_WIP_IMPORT_FOLDER_COLOR & 1

// Orphan
#define ECB_WIP_IMPORT_ORPHAN 1

// Asset Viewer
#define ECB_WIP_CONTENT_TYPE_OVERLAY 1
#define ECB_WIP_REPARSE_ASSET 1

// Cache Mode
#define ECB_WIP_CACHEDB 1
#define ECB_WIP_CACHEDB_LOADNAMEBATCH ECB_WIP_CACHEDB & 1 // 4.27+
#define ECB_WIP_CACHEDB_SWITCH ECB_WIP_CACHEDB & 1

// ThumbnailPool
#define ECB_WIP_OBJECT_THUMB_POOL 1

// Browser feature
#define ECB_WIP_MORE_FILTER ECB_FEA_FILTER & 1

////////////////////////////////
// WIP
//

// WIP: Show Hide Toolbar Button
#define ECB_WIP_TOGGLE_TOOLBAR_BUTTON ECB_WIP & 0

// WIP: Dependency Viewer
#define ECB_WIP_REF_VIEWER_JUMP ECB_WIP & 0
#define ECB_WIP_REF_VIEWER_HISTORY ECB_WIP & 0
#define ECB_WIP_REF_VIEWER_HIGHLIGHT_ERROR ECB_WIP & 0

// WIP: Browser feature
#define ECB_WIP_SEARCH_RECURSE_TOGGLE ECB_WIP & 0
#define ECB_WIP_MORE_VIEWTYPE ECB_WIP & 0
#define ECB_WIP_INITIAL_ASSET ECB_WIP & 0
#define ECB_WIP_FAVORITE ECB_WIP & 0
#define ECB_WIP_COLLECTION ECB_WIP & 0
#define ECB_WIP_BREADCRUMB ECB_WIP & 0
#define ECB_WIP_HISTORY ECB_WIP & 0
#define ECB_WIP_PATHPICKER ECB_WIP & 0
#define ECB_WIP_LOCK ECB_WIP & 0
#define ECB_WIP_MULTI_INSTANCES ECB_WIP & 0
#define ECB_WIP_DELEGATES ECB_WIP & 0
#define ECB_WIP_SYNC_ASSET ECB_WIP & 0

#define ECB_WIP_FOLDER_ICONS ECB_WIP & 0

// WIP: Asset Registry Serialization
#define ECB_WIP_ASSETREGISTRY_SERIALIZATION ECB_WIP & 0

// WIP: IMPORT
#define ECB_WIP_IMPORT_SANDBOX ECB_WIP & 0
#define ECB_WIP_IMPORT_FOR_DUMP ECB_WIP & 1

// WIP: EXPORT
#define ECB_WIP_EXPORT ECB_WIP & 0
#define ECB_WIP_EXPORT_ZIP ECB_WIP_EXPORT & 1

// WIP: Asset Viewer

////////////////////////////////
// Experimental
//

// Thumbnail cache
#define ECB_WIP_THUMB_CACHE ECB_WIP & 0


DECLARE_LOG_CATEGORY_EXTERN(LogECB, Verbose, All);

#if ENABLE_ECB_LOG
#define ECB_LOG(Verbosity, Format, ...) \
	UE_LOG(LogECB, Verbosity, Format, ##__VA_ARGS__);
#else
#define ECB_LOG(Verbosity, Format, ...)
#endif

#define ECB_INFO(Verbosity, Format, ...) \
	UE_LOG(LogECB, Verbosity, Format, ##__VA_ARGS__);

class IExtContentBrowserSingleton;
struct FExtAssetData;
class FExtDependencyGraphPanelNodeFactory;

class FToolBarBuilder;
class FMenuBuilder;
class FExtender;

/** Delegate called when selection changed. Provides more context than FOnAssetSelected */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExtAssetSelectionChanged, const TArray<FExtAssetData>& /*NewSelectedAssets*/, bool /*bIsPrimaryBrowser*/);

class FExtContentBrowserModule : public IModuleInterface, public IDocumentationProvider
{
public:
	DECLARE_DELEGATE_RetVal(TSharedRef<FExtender>, FExtContentBrowserMenuExtender);

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Gets the content browser singleton */
	virtual IExtContentBrowserSingleton& Get() const;

	virtual TArray<FExtContentBrowserMenuExtender>& GetAllAssetViewViewMenuExtenders() { return AssetViewViewMenuExtenders; }
	
public:

	FOnExtAssetSelectionChanged& GetOnAssetSelectionChanged() { return OnAssetSelectionChanged; }

public:
	// IDocumentationProvider interface
	virtual TSharedRef<IDocumentation> GetDocumentation() const override;
private:
	TSharedPtr<IDocumentation> Documentation;

private:

	IExtContentBrowserSingleton* ContentBrowserSingleton;
	TArray<FExtContentBrowserMenuExtender> AssetViewViewMenuExtenders;
	FOnExtAssetSelectionChanged OnAssetSelectionChanged;
};

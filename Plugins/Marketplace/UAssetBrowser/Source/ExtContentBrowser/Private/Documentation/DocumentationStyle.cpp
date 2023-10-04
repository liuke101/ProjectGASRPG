// Copyright 2017-2021 marynate. All Rights Reserved.

#include "DocumentationStyle.h"

#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "EditorStyleSet.h"

#include "DocumentationDefines.h"
using namespace EXT_DOC_NAMESPACE;

const ISlateStyle& FExtDocumentationStyle::Get()
{
	if (const ISlateStyle* AppStyle = FSlateStyleRegistry::FindSlateStyle(DocumentationStyleSetName))
	{
		return *AppStyle;
	}

	return FAppStyle::Get();
}

FDocumentationStyle FExtDocumentationStyle::GetDefaultDocumentationStyle()
{
	FDocumentationStyle DocumentationStyle;
	{
		DocumentationStyle
			.ContentStyle(TEXT("Tutorials.Content.Text"))
			.BoldContentStyle(TEXT("Tutorials.Content.TextBold"))
			.NumberedContentStyle(TEXT("Tutorials.Content.Text"))
			.Header1Style(TEXT("Tutorials.Content.HeaderText1"))
			.Header2Style(TEXT("Tutorials.Content.HeaderText2"))
			.HyperlinkStyle(TEXT("Tutorials.Content.Hyperlink"))
			.HyperlinkTextStyle(TEXT("Tutorials.Content.HyperlinkText"))
			.SeparatorStyle(TEXT("Tutorials.Separator"));
	}
	return DocumentationStyle;
}

#ifdef EXT_DOC_NAMESPACE
#undef EXT_DOC_NAMESPACE
#endif
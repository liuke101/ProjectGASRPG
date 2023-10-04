// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class IDocumentationPage;
class SButton;
class SImage;
struct FSlateBrush;

#include "DocumentationDefines.h"

namespace EXT_DOC_NAMESPACE
{

class SExtDocumentationAnchor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SExtDocumentationAnchor )
	{}
		SLATE_ARGUMENT( FString, PreviewLink )
		SLATE_ARGUMENT( FString, PreviewExcerptName )

		/** The string for the link to follow when clicked  */
		SLATE_ATTRIBUTE( FString, Link )
		/** The base URL for the Link, if any is needed  */
		SLATE_ATTRIBUTE(FString, BaseUrlId)

	SLATE_END_ARGS()


	void Construct(const FArguments& InArgs);

private:
	
	const FSlateBrush* GetButtonImage() const;

	FReply OnClicked() const;

private:

	TAttribute<FString> Link;
	TSharedPtr< SButton > Button;
	TSharedPtr< SImage > ButtonImage;
	const FSlateBrush* Default; 
	const FSlateBrush* Hovered; 
	const FSlateBrush* Pressed; 

	TSharedPtr< IDocumentationPage > DocumentationPage;
};

}

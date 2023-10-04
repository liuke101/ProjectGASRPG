// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SDocumentationAnchor.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "EditorStyleSet.h"
#include "Widgets/SToolTip.h"
#include "IDocumentation.h"

#include "DocumentationLink.h"

//#include "ExtDocumentation.h"

void SExtDocumentationAnchor::Construct(const FArguments& InArgs )
{
	Link = InArgs._Link;
	
	TAttribute<FText> ToolTipText = InArgs._ToolTipText;
	if ( !ToolTipText.IsBound() && ToolTipText.Get().IsEmpty() )
	{
		ToolTipText = NSLOCTEXT("DocumentationAnchor", "DefaultToolTip", "Click to open documentation");
	}

	Default = FAppStyle::GetBrush( "HelpIcon" );
	Hovered = FAppStyle::GetBrush( "HelpIcon.Hovered" );
	Pressed = FAppStyle::GetBrush( "HelpIcon.Pressed" );

	FString PreviewLink = InArgs._PreviewLink;
	if (!PreviewLink.IsEmpty())
	{
		// All in-editor udn documents must live under the Shared/ folder
		ensure(PreviewLink.StartsWith(TEXT("Shared/")));
	}

	ChildSlot
	[
		SAssignNew( Button, SButton )
		.ContentPadding( 0 )
		.ButtonStyle(FAppStyle::Get(), "HelpButton" )
		.OnClicked( this, &SExtDocumentationAnchor::OnClicked )
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		.ToolTip( IDocumentation::Get()->CreateToolTip( ToolTipText, NULL, PreviewLink, InArgs._PreviewExcerptName ) )
		[
			SAssignNew(ButtonImage, SImage)
			.Image( this, &SExtDocumentationAnchor::GetButtonImage )
		]
	];
}

const FSlateBrush* SExtDocumentationAnchor::GetButtonImage() const
{
	if ( Button->IsPressed() )
	{
		return Pressed;
	}

	if ( ButtonImage->IsHovered() )
	{
		return Hovered;
	}

	return Default;
}

FReply SExtDocumentationAnchor::OnClicked() const
{
	IDocumentation::Get()->Open(Link.Get(), FDocumentationSourceInfo(TEXT("doc_anchors")));
	return FReply::Handled();
}

#ifdef EXT_DOC_NAMESPACE
#undef EXT_DOC_NAMESPACE
#endif
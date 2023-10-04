// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DocumentationPage.h"

#include "ExtDocumentation.h"

TSharedRef< IDocumentationPage > FExtDocumentationPage::Create( const FString& Link, const TSharedRef< FExtUDNParser >& Parser ) 
{
	return MakeShareable( new FExtDocumentationPage( Link, Parser ) );
}

FExtDocumentationPage::~FExtDocumentationPage() 
{

}

bool FExtDocumentationPage::GetExcerptContent( FExcerpt& Excerpt )
{
	for (int32 Index = 0; Index < StoredExcerpts.Num(); ++Index)
	{
		if ( Excerpt.Name == StoredExcerpts[ Index ].Name )
		{
			Parser->GetExcerptContent( Link, StoredExcerpts[ Index ] );
			Excerpt.Content = StoredExcerpts[ Index ].Content;
			Excerpt.RichText = StoredExcerpts[ Index ].RichText;
			return true;
		}
	}

	return false;
}

bool FExtDocumentationPage::HasExcerpt( const FString& ExcerptName )
{
	return StoredMetadata.ExcerptNames.Contains( ExcerptName );
}

int32 FExtDocumentationPage::GetNumExcerpts() const
{
	return StoredExcerpts.Num();
}

bool FExtDocumentationPage::GetExcerpt(const FString& ExcerptName, FExcerpt& Excerpt)
{
	for (const FExcerpt& StoredExcerpt : StoredExcerpts)
	{
		if (StoredExcerpt.Name == ExcerptName)
		{
			Excerpt = StoredExcerpt;
			return true;
		}
	}

	return false;
}

void FExtDocumentationPage::GetExcerpts( /*OUT*/ TArray< FExcerpt >& Excerpts )
{
	Excerpts.Empty();
	for (int32 i = 0; i < StoredExcerpts.Num(); ++i)
	{
		Excerpts.Add(StoredExcerpts[i]);
	}
}

FText FExtDocumentationPage::GetTitle()
{
	return StoredMetadata.Title;
}

void FExtDocumentationPage::Reload()
{
	StoredExcerpts.Empty();
	StoredMetadata = FUDNPageMetadata();
	Parser->Parse( Link, StoredExcerpts, StoredMetadata );
}

void FExtDocumentationPage::SetTextWrapAt( TAttribute<float> WrapAt )
{
	Parser->SetWrapAt( WrapAt );
}

bool FExtDocumentationPage::GetSimpleExcerptContent(FExcerpt& Excerpt)
{
	for (int32 Index = 0; Index < StoredExcerpts.Num(); ++Index)
	{
		if (Excerpt.Name == StoredExcerpts[Index].Name)
		{
			Parser->GetExcerptContent(Link, StoredExcerpts[Index], /*bInSimpleText*/ true);
			Excerpt.Content = StoredExcerpts[Index].Content;
			Excerpt.RichText = StoredExcerpts[Index].RichText;
			return true;
		}
	}

	return false;
}

FExtDocumentationPage::FExtDocumentationPage( const FString& InLink, const TSharedRef< FExtUDNParser >& InParser ) 
	: Link( InLink )
	, Parser( InParser )
{
	Parser->Parse( Link, StoredExcerpts, StoredMetadata );
}

#ifdef EXT_DOC_NAMESPACE
#undef EXT_DOC_NAMESPACE
#endif
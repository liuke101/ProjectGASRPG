// Copyright 2018-2020 marynate. All Rights Reserved.

#pragma once

#include "IDocumentation.h"

#include "DocumentationDefines.h"

namespace EXT_DOC_NAMESPACE
{

class IDocumentationProvider
{
public:
	virtual TSharedRef<IDocumentation> GetDocumentation() const = 0;
};

struct FDocumentationProvider
{
	static FDocumentationProvider& Get()
	{
		static FDocumentationProvider Instance;
		return Instance;
	}

	const IDocumentationProvider* GetProvider(const FName& InProviderName)
	{
		if (const IDocumentationProvider** ProviderPtr = DocumentationProviders.Find(InProviderName))
		{
			return *ProviderPtr;
		}
		return NULL;
	}

	void RegisterProvider(const FName& InProviderName, const IDocumentationProvider* InProvider)
	{
		if (!InProvider)
		{
			return;
		}

		if (!DocumentationProviders.Find(InProviderName)) // Add
		{
			DocumentationProviders.Add(InProviderName, InProvider);
		}
		else // Override
		{
			DocumentationProviders[InProviderName] = InProvider;
		}
	}

private:
	TMap<FName, const IDocumentationProvider*> DocumentationProviders;
};

}
// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalArtifactProvider/URIResolvers/URIResolverBase.h"
#include "SyloURIResolver.generated.h"

/**
 * 
 */
UCLASS()
class FUTUREVERSEUBFCONTROLLER_API USyloURIResolver : public UURIResolverBase
{
	GENERATED_BODY()
public:
	virtual bool CanResolveURI(const FString& URI) override;
	virtual TFuture<UBF::FLoadDataArrayResult> ResolveURI(const FString& TypeId, const FString& URI) override;
};

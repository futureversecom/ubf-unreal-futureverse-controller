// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UBFItem.h"
#include "AssetRegisterUBFItem.generated.h"

/**
 * 
 */
UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UAssetRegisterUBFItem : public UUBFItem
{
	GENERATED_BODY()

public:
	virtual TFuture<bool> LoadContextTree() override;
	
	virtual TFuture<bool> LoadProfileURI() override;
};

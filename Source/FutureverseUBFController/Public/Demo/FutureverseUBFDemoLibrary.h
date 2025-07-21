// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectWrapper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FutureverseUBFDemoLibrary.generated.h"

/**
 * 
 */
UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UFutureverseUBFDemoLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static FString GetImagePropertyFromJsonString(const FJsonObjectWrapper& JsonObjectWrapper, const FString& ImagePropertyName);

};

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FutureverseUBFControllerSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Engine, defaultconfig, meta = (DisplayName = "Futureverse Controller Layer"))
class FUTUREVERSEUBFCONTROLLER_API UFutureverseUBFControllerSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UFutureverseUBFControllerSettings();
	
	FString GetDefaultAssetProfilePath() const { return DefaultAssetProfilePath.TrimStartAndEnd(); } 
private:
	UPROPERTY(EditAnywhere, Config)
	FString DefaultAssetProfilePath = "https://fv-ubf-assets-dev.s3.us-west-2.amazonaws.com/Genesis/Profiles/1.0/";
};

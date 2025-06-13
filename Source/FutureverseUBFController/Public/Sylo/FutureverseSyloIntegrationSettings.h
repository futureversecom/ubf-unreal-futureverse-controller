// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FutureverseSyloIntegrationSettings.generated.h"

/**
 * 
 */
UCLASS(DefaultConfig, Config=Engine, DisplayName="Futureverse Sylo Integration")
class FUTUREVERSEUBFCONTROLLER_API UFutureverseSyloIntegrationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UFutureverseSyloIntegrationSettings();

	UPROPERTY(EditAnywhere, Config)
	TArray<FString> TargetSyloResolverIDs = {TEXT("fv-sylo-resolver-staging")};
	
	UPROPERTY(EditAnywhere, Config)
	bool bAutomaticallyHandleSyloAccessSource = true;
};

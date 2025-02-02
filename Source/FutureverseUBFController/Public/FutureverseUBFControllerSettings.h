// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FutureverseUBFControllerSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, defaultconfig, meta = (DisplayName = "Futureverse UBF Controller Settings"))
class FUTUREVERSEUBFCONTROLLER_API UFutureverseUBFControllerSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	FString GetDefaultAssetProfilePath() const { return DefaultAssetProfilePath.TrimStartAndEnd(); } 
private:
	UPROPERTY(EditAnywhere, Config)
	FString DefaultAssetProfilePath = "https://fv-ubf-assets-dev.s3.us-west-2.amazonaws.com/Profiles";
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "Engine/DataAsset.h"
#include "CollectionAssetProfiles.generated.h"

USTRUCT(BlueprintType)
struct FUTUREVERSEUBFCONTROLLER_API FAssetProfileData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = FString("Invalid");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString GraphUri;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ResourceManifestUri;

	FAssetProfile CreateProfileFromData(const FString& BasePath) const;
};

/**
 * AssetProfile data to be used by UFutureverseUBFControllerSubsystem
 */
UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UCollectionAssetProfiles : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString BasePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString AssetProfilesJson;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAssetProfileData> AdditionalData;
};

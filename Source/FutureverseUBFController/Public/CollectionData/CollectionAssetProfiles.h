// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "Engine/DataAsset.h"
#include "CollectionAssetProfiles.generated.h"

enum class EEnvironment : uint8;
/*
 * Contains same data as FAssetProfile minus relative path,
 * exists to make data entry of asset profiles easier as you don't have to specify 'BasePath' multiple times
 */
USTRUCT(BlueprintType)
struct FUTUREVERSEUBFCONTROLLER_API FAssetProfileData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = FString("Invalid");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RenderBlueprintInstanceUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParsingBlueprintInstanceUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RenderCatalogUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParsingCatalogUri;
	
	FAssetProfile CreateProfileFromData(const FString& BasePath) const;
};

/**
 * AssetProfile data to be used by UFutureverseUBFControllerSubsystem to add custom AssetProfiles
 */
UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UCollectionAssetProfiles : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString BasePath;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<EEnvironment, FString> AssetProfilesJsonMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAssetProfileData> AdditionalData;
};

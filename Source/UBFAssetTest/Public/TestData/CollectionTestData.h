// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CollectionTestData.generated.h"


USTRUCT(BlueprintType)
struct UBFASSETTEST_API FTestAssetInputDefinition
{
	GENERATED_BODY()

	// Used for Graph Input and Context Tree Relationship
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TargetContractID;
};

USTRUCT(BlueprintType)
struct UBFASSETTEST_API FCollectionTestAssetDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ContractID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTestAssetInputDefinition> TestInputs;
};

/**
 * 
 */
UCLASS(BlueprintType)
class UBFASSETTEST_API UCollectionTestData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCollectionTestAssetDefinition> TestAssetDefinitions;
};

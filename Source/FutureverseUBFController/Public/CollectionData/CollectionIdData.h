// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CollectionIdData.generated.h"

enum class EEnvironment : uint8;

USTRUCT(BlueprintType)
struct FUTUREVERSEUBFCONTROLLER_API FCollectionIdDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEnvironment Environment;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> CollectionIds;
};

/**
 * 
 */
UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UCollectionIdData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCollectionIdDefinition> CollectionsIdDefinitions;

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetCollectionQueryIds(const EEnvironment Environment) const
	{
		for (auto CollectionIdDefinition : CollectionsIdDefinitions)
		{
			if(CollectionIdDefinition.Environment == Environment)
			{
				return CollectionIdDefinition.CollectionIds;
			}
		}

		return TArray<FString>();
	}
};
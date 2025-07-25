// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FutureverseUBFControllerSubsystem.h"
#include "Engine/DataAsset.h"
#include "CollectionIdData.generated.h"

enum class EEnvironment : uint8;

USTRUCT(BlueprintType)
struct FUTUREVERSEUBFCONTROLLER_API FCollectionIdDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEnvironment Environment = EEnvironment::Development;
	
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
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CollectionIdData.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UCollectionIdData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FString> CollectionsToQuery;

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetCollectionQueryIds() const {return CollectionsToQuery;}
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UBFInventoryComponent.generated.h"

class UUBFInventoryItem;

DECLARE_DYNAMIC_DELEGATE(FOnRequestCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedEvent);

UCLASS(Abstract, Blueprintable)
class FUTUREVERSEUBFCONTROLLER_API UUBFInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Request inventory items based off OwnerAddress
	UFUNCTION(BlueprintCallable)
	virtual void RequestFuturepassInventory(const FString& OwnerAddress, const FOnRequestCompleted& OnRequestCompleted) {};

	// Request inventory items based off OwnerAddress and CollectionsIds
	UFUNCTION(BlueprintCallable)
	virtual void RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress, const TArray<FString>& CollectionIds,
		const FOnRequestCompleted& OnRequestCompleted) {};

	// Get current received items
	UFUNCTION(BlueprintCallable)
	TArray<UUBFInventoryItem*>& GetInventory() { return Inventory; }

protected:
	UPROPERTY()
	TArray<UUBFInventoryItem*> Inventory;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryUpdatedEvent OnInventoryUpdated;
	
	FOnRequestCompleted OnInventoryRequestCompleted;
	FOnRequestCompleted OnFilteredInventoryRequestCompleted;
};

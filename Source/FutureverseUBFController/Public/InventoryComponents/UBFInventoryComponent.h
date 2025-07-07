// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UBFInventoryComponent.generated.h"

class UUBFItem;
class FItemRegistry;

DECLARE_DYNAMIC_DELEGATE(FOnRequestCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedEvent);

UCLASS(Abstract, Blueprintable)
class FUTUREVERSEUBFCONTROLLER_API UUBFInventoryComponent : public UActorComponent
{
public:
	virtual void InitializeComponent() override;

	// Request inventory items based off OwnerAddress
	UFUNCTION(BlueprintCallable)
	virtual void RequestFuturepassInventory(const FString& OwnerAddress, const FOnRequestCompleted& OnRequestCompleted) {};

	// Request inventory items based off OwnerAddress and CollectionsIds
	UFUNCTION(BlueprintCallable)
	virtual void RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress, const TArray<FString>& CollectionIds,
		const FOnRequestCompleted& OnRequestCompleted) {};

	// Get current received items
	UFUNCTION(BlueprintCallable)
	TArray<UUBFItem*>& GetInventory() { return Inventory; }

	UFUNCTION(BlueprintCallable)
	virtual UUBFItem* GetItem(const FString& ItemId);

	UFUNCTION(BlueprintCallable)
	virtual void RegisterItem(const FString& ItemId, UUBFItem* Item);

protected:
	UPROPERTY()
	TArray<UUBFItem*> Inventory;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryUpdatedEvent OnInventoryUpdated;

	TSharedPtr<FItemRegistry> ItemRegistry;
	
	FOnRequestCompleted OnInventoryRequestCompleted;
	FOnRequestCompleted OnFilteredInventoryRequestCompleted;
};

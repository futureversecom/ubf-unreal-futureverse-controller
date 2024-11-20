// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FuturePassInventoryItem.h"
#include "FutureverseInventoryComponent.generated.h"

class UGetFuturepassInventoryByCollectionAndOwner;
class UGetFuturepassInventory;

DECLARE_DYNAMIC_DELEGATE(FOnRequestCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedEvent);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FUTUREVERSEUBFCONTROLLER_API UFutureverseInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UFutureverseInventoryComponent();

	// Get current received items
	UFUNCTION(BlueprintCallable)
	void GetInventory(TArray<UFuturePassInventoryItem*>& OutInventory);

	// Request inventory items based off OwnerAddress
	UFUNCTION(BlueprintCallable)
	void RequestFuturepassInventory(const FString& OwnerAddress, const FOnRequestCompleted& OnRequestCompleted);

	// Request inventory items based off OwnerAddress and CollectionsIds
	UFUNCTION(BlueprintCallable)
	void RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress, const TArray<FString>& CollectionIds,
		const FOnRequestCompleted& OnRequestCompleted);

protected:
	void UpdateInventory(const TArray<FEmergenceInventoryItem>& Items);
	
private:
	UFUNCTION()
	void HandleGetFuturepassInventory(FEmergenceInventory Response, EErrorCode StatusCode);
	
	UFUNCTION()
	void HandleGetFuturepassInventoryByCollectionAndOwner(FEmergenceInventory Response, EErrorCode StatusCode);

	UFUNCTION()
	void HandleGetAssetTree(const TArray<FFutureverseAssetTreePath>& Tree, EErrorCode StatusCode);

	TMap<FString, FString> GetLinkedItemsForAssetTree(const TArray<FFutureverseAssetTreePath>& Tree);
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryUpdatedEvent OnInventoryUpdated;
	
	FOnRequestCompleted OnInventoryRequestCompleted;
	FOnRequestCompleted OnFilteredInventoryRequestCompleted;

	int32 PendingNumberOfAssetTreeRequests = 0;

	UPROPERTY()
	TArray<UFuturePassInventoryItem*> Inventory;
	
	UPROPERTY()
	UGetFuturepassInventory* PendingGetInventoryRequest;
	UPROPERTY()
	UGetFuturepassInventoryByCollectionAndOwner* PendingGetInventoryByCollectionAndOwnerRequest;
};

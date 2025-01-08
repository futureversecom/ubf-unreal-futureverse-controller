// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UBFInventoryComponent.h"
#include "InventoryService/EmergenceInventoryServiceStructs.h"
#include "EmergenceUBFInventoryComponent.generated.h"

struct FFutureverseAssetTreePath;

class UGetFuturepassInventoryByCollectionAndOwner;
class UGetFuturepassInventory;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FUTUREVERSEUBFCONTROLLER_API UEmergenceUBFInventoryComponent : public UUBFInventoryComponent
{
	GENERATED_BODY()
	
public:
	// Request inventory items based off OwnerAddress
	virtual void RequestFuturepassInventory(const FString& OwnerAddress, const FOnRequestCompleted& OnRequestCompleted) override;

	// Request inventory items based off OwnerAddress and CollectionsIds
	virtual void RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress, const TArray<FString>& CollectionIds,
		const FOnRequestCompleted& OnRequestCompleted) override;

protected:
	void UpdateInventory(const TArray<FEmergenceInventoryItem>& Items);
	
private:
	UFUNCTION()
	void HandleGetFuturepassInventory(FEmergenceInventory Response, EErrorCode StatusCode);
	
	UFUNCTION()
	void HandleGetFuturepassInventoryByCollectionAndOwner(FEmergenceInventory Response, EErrorCode StatusCode);

	UFUNCTION()
	void HandleGetAssetTree(const TArray<FFutureverseAssetTreePath>& Tree, EErrorCode StatusCode);
	
	UPROPERTY()
	UGetFuturepassInventory* PendingGetInventoryRequest;
	UPROPERTY()
	UGetFuturepassInventoryByCollectionAndOwner* PendingGetInventoryByCollectionAndOwnerRequest;
};

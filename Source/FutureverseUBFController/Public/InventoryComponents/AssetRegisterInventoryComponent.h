// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegisterQueryingLibrary.h"
#include "InventoryComponents/UBFInventoryComponent.h"
#include "Items/UBFItem.h"
#include "AssetRegisterInventoryComponent.generated.h"

struct FAssets;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FUTUREVERSEUBFCONTROLLER_API UAssetRegisterInventoryComponent : public UUBFInventoryComponent
{
	GENERATED_BODY()

public:
	virtual void RequestFuturepassInventory(const FString& OwnerAddress, const FOnRequestCompleted& OnRequestCompleted) override;
	
	virtual void RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress,
		const TArray<FString>& CollectionIds, const FOnRequestCompleted& OnRequestCompleted) override;
	
private:
	FUBFItemData CreateItemDataFromAsset(const FAsset& Asset);
	
	UFUNCTION()
	void HandleGetFuturepassInventory(bool bSuccess, const FAssets& Assets);
	
	UPROPERTY()
	FGetAssetsCompleted GetAssetsRequestCompleted;
};

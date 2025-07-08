// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControllerLayers/AssetProfile.h"
#include "Items/UBFItem.h"
#include "CollectionTestWidget.generated.h"

class UUBFItem;
class UCollectionTestInputBindingObject;
class UUBFInventoryItem;
class FMemoryCacheLoader;
class UCollectionTestData;
struct FUBFContextTreeData;

/**
 * 
 */
UCLASS()
class UBFASSETTEST_API UCollectionTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void LoadAllTestAssets(const UCollectionTestData* TestData, const FOnLoadCompleted& OnLoadCompleted);
	
	UFUNCTION(BlueprintCallable)
	TArray<UUBFItem*>& GetInventory() { return TestAssetInventory; }

	UFUNCTION(BlueprintCallable)
	UUBFItem* GetItemForAsset(const FString& AssetID);

	UFUNCTION(BlueprintCallable)
	TArray<FUBFContextTreeData> MakeContextTree(const FString& RootAssetID, const TArray<UCollectionTestInputBindingObject*>& Inputs);
private:
	UPROPERTY()
	TArray<UUBFItem*> TestAssetInventory;
	
	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader;
	
	TMap<FString, FAssetProfile> AssetProfiles;

	int32 NumberOfDownloadedProfiles;
};

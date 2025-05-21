// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControllerLayers/AssetProfile.h"
#include "CollectionTestWidget.generated.h"

class UCollectionTestInputBindingObject;
class UUBFInventoryItem;
class FMemoryCacheLoader;
class UCollectionTestData;
struct FUBFContextTreeData;
/**
 * 
 */

DECLARE_DYNAMIC_DELEGATE(FOnLoadCompleted);

UCLASS()
class UBFASSETTEST_API UCollectionTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void InitializeWidget();
	
	UFUNCTION(BlueprintCallable)
	void LoadAllTestAssets(const UCollectionTestData* TestData, const FOnLoadCompleted& OnLoadCompleted);
	
	UFUNCTION(BlueprintCallable)
	TArray<UUBFInventoryItem*>& GetInventory() { return TestAssetInventory; }

	UFUNCTION(BlueprintCallable)
	UUBFInventoryItem* GetItemForAsset(const FString& AssetID);

	UFUNCTION(BlueprintCallable)
	TArray<FUBFContextTreeData> MakeContextTree(const FString& RootAssetID, const TArray<UCollectionTestInputBindingObject*>& Inputs);
private:
	UPROPERTY()
	TArray<UUBFInventoryItem*> TestAssetInventory;
	
	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader;
	
	TMap<FString, FAssetProfile> AssetProfiles;

	int32 NumberOfDownloadedProfiles;
};

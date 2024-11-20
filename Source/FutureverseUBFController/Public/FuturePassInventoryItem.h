// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "Futurepass/GetAssetTree.h"
#include "InventoryService/EmergenceInventoryServiceStructs.h"
#include "FuturePassInventoryItem.generated.h"

USTRUCT(BlueprintType)
struct FUTUREVERSEUBFCONTROLLER_API FFutureverseAssetTreeData
{
	GENERATED_BODY()
	
	TArray<FFutureverseAssetTreePath> TreePaths;
	
	UPROPERTY()
	TMap<FString, FString> LinkedItems;
};

UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UFuturePassInventoryItem : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(const FEmergenceInventoryItem& EmergenceInventoryItem);
	void SetAssetTree(const FFutureverseAssetTreeData& NewAssetTree) { AssetTree = NewAssetTree; }

	const FAssetProfile& GetAssetProfileRef() const;
	const FFutureverseAssetTreeData& GetAssetTreeRef() const { return AssetTree; }

	// Get current cached asset profile, should be valid once Initialize() is called
	UFUNCTION(BlueprintCallable)
	FAssetProfile GetAssetProfile() const {return AssetProfile;}

	// Get associated AssetTreeData
	UFUNCTION(BlueprintCallable)
	FFutureverseAssetTreeData GetAssetTree() const { return AssetTree; }

	// Get collection id from json metadata
	UFUNCTION(BlueprintCallable)
	FString GetCollectionID() const;
	
	// Get collection id combined with token id
	UFUNCTION(BlueprintCallable)
	FString GetCombinedID() const;

	// Get AssetId used to find corresponding AssetProfile
	UFUNCTION(BlueprintCallable)
	FString GetAssetID() const;

	// Get associated FEmergenceInventoryItem
	UFUNCTION(BlueprintCallable)
	FEmergenceInventoryItem GetInventoryItem() const { return InventoryItem; }
	
private:
	FString MapCollectionIdToName(const FString& CollectionId) const;
	static void LogDataString(TSharedPtr<FJsonObject> JsonObject);
	
	FEmergenceInventoryItem InventoryItem;
	FFutureverseAssetTreeData AssetTree;
	mutable FAssetProfile AssetProfile;
};

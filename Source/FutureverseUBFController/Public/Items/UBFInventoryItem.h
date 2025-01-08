// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectWrapper.h"
#include "UObject/Object.h"
#include "UBFInventoryItem.generated.h"


struct FFutureverseAssetLoadData;

USTRUCT(Blueprintable)
struct FUBFItemData
{
	FUBFItemData(const FString& AssetID, const FString& AssetName, const FString& ContractID, const FString& TokenID,
		const FString& CollectionID, const FString& MetadataJson, const FJsonObjectWrapper& MetadataJsonObject)
		: AssetID(AssetID), AssetName(AssetName), ContractID(ContractID),
		TokenID(TokenID), CollectionID(CollectionID), MetadataJson(MetadataJson), MetadataJsonObject(MetadataJsonObject) {}

	GENERATED_BODY()

	FUBFItemData() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AssetID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AssetName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ContractID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TokenID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CollectionID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MetadataJson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FJsonObjectWrapper MetadataJsonObject;
	
	FString GetCombinedID() const
	{
		return FString::Printf(TEXT("%s:%s"), *CollectionID, *TokenID);
	}
	
	FString ToString() const
	{
		return FString::Printf(
			TEXT("AssetID: %s\nContractID: %s\nTokenID: %s\nCollectionID: %s\nMetadataJson: %s\n"),
			*AssetID,
			*ContractID,
			*TokenID,
			*CollectionID,
			*MetadataJson
		);
	}
};

USTRUCT(Blueprintable)
struct FUBFContextTreeData
{
	GENERATED_BODY()

	FUBFContextTreeData() {}
	
	FUBFContextTreeData(const FString& RootNodeID, const TMap<FString, FString>& Children)
	: RootNodeID(RootNodeID), Children(Children) {}

	UPROPERTY()
	FString RootNodeID;

	// <Relationship, AssetID>
	UPROPERTY()
	TMap<FString, FString> Children;
};

/**
 * 
 */
UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UUBFInventoryItem : public UObject
{
	GENERATED_BODY()

public:
	static FUBFItemData CreateItemDataFromMetadataJson(const FString& ContractID, const FString& TokenID,
		const FJsonObjectWrapper& MetadataJsonWrapper);
	
	UFUNCTION(BlueprintCallable)
	void SetItemData(const FUBFItemData& NewItemData) { ItemData = NewItemData; }
	
	UFUNCTION(BlueprintCallable)
	void SetContextTree(const TArray<FUBFContextTreeData>& NewContextTree) { ContextTree = NewContextTree; }

	UFUNCTION(BlueprintCallable)
	FUBFItemData GetItemData() { return ItemData; }

	UFUNCTION(BlueprintCallable)
	FUBFItemData& GetItemDataRef() { return ItemData; }
	
	UFUNCTION(BlueprintCallable)
	TArray<FUBFContextTreeData> GetContextTree() { return ContextTree; }

	UFUNCTION(BlueprintCallable)
	TArray<FUBFContextTreeData>& GetContextTreeRef() { return ContextTree; }

	UFUNCTION(BlueprintCallable)
	FString GetAssetID() const { return ItemData.AssetID; }

	UFUNCTION(BlueprintCallable)
	FString GetAssetName() const { return ItemData.AssetName; }

	UFUNCTION(BlueprintCallable)
	FString GetContractID() const { return ItemData.ContractID; }

	UFUNCTION(BlueprintCallable)
	FString GetCombinedID() const { return ItemData.GetCombinedID(); }

	UFUNCTION(BlueprintCallable)
	FString GetMetadataJson() const { return ItemData.MetadataJson; }

	TArray<FFutureverseAssetLoadData> GetLinkedAssetLoadData() const;
	
private:
	UPROPERTY()
	FUBFItemData ItemData;
	
	UPROPERTY()
	TArray<FUBFContextTreeData> ContextTree;
};

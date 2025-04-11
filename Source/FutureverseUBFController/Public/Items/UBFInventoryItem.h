// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectWrapper.h"
#include "UObject/Object.h"
#include "UBFInventoryItem.generated.h"

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

USTRUCT(Blueprintable, BlueprintType)
struct FUBFContextTreeRelationshipData
{
	GENERATED_BODY()

	FUBFContextTreeRelationshipData() {}

	FUBFContextTreeRelationshipData(const FString& RelationshipID, const FString& ChildAssetID)
	: RelationshipID(RelationshipID), ChildAssetID(ChildAssetID) {}

	UPROPERTY()
	FString RelationshipID;

	UPROPERTY()
	FString ChildAssetID;
};

USTRUCT(Blueprintable, BlueprintType)
struct FUBFContextTreeData
{
	GENERATED_BODY()

	FUBFContextTreeData() {}
	
	FUBFContextTreeData(const FString& RootNodeID, const TArray<FUBFContextTreeRelationshipData>& Relationships)
	: RootNodeID(RootNodeID), Relationships(Relationships) {}

	UPROPERTY()
	FString RootNodeID;

	UPROPERTY()
	TArray<FUBFContextTreeRelationshipData> Relationships;
};

// contains the core data that UBF Subsystem needs to render an item
USTRUCT(BlueprintType)
struct FUBFRenderData
{
	GENERATED_BODY()

	FUBFRenderData() {}

	FUBFRenderData(const FString& AssetID, const FString& ContractID,
			const FString& MetadataJson,const TArray<FUBFContextTreeData>& ContextTree)
		: AssetID(AssetID), ContractID(ContractID),
		  MetadataJson(MetadataJson), ContextTree(ContextTree) {}
	
	UPROPERTY(BlueprintReadOnly)
	FString AssetID = "Invalid";

	UPROPERTY(BlueprintReadOnly)
	FString ContractID;

	UPROPERTY(BlueprintReadOnly)
	FString MetadataJson;

	UPROPERTY(BlueprintReadOnly)
	TArray<FUBFContextTreeData> ContextTree;
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
	FUBFItemData GetItemData() const { return ItemData; }

	UFUNCTION(BlueprintCallable)
	FUBFItemData& GetItemDataRef() { return ItemData; }
	
	UFUNCTION(BlueprintCallable)
	TArray<FUBFContextTreeData> GetContextTree() const { return ContextTree; }

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
	FString GetTokenID() const { return ItemData.TokenID; }

	UFUNCTION(BlueprintCallable)
	FString GetCollectionID() const { return ItemData.CollectionID; }

	UFUNCTION(BlueprintCallable)
	FString GetMetadataJson() const { return ItemData.MetadataJson; }

	UFUNCTION(BlueprintCallable)
	FUBFRenderData GetRenderData();

	UFUNCTION(BlueprintCallable)
	void InitializeFromRenderData(const FUBFRenderData& RenderData);

protected:
	UPROPERTY()
	TArray<FUBFContextTreeData> ContextTree;
	
	UPROPERTY()
    FUBFItemData ItemData;
};

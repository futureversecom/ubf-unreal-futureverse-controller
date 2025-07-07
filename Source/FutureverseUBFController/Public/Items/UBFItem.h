// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectWrapper.h"
#include "InventoryComponents/ItemRegistry.h"
#include "UObject/Object.h"
#include "UBFItem.generated.h"

class FItemRegistry;

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

	FUBFContextTreeRelationshipData(const FString& RelationshipID, const FString& ChildAssetID,
		const FString& ProfileURI): RelationshipID(RelationshipID), ChildAssetID(ChildAssetID), ProfileURI(ProfileURI){}

	UPROPERTY()
	FString RelationshipID;

	UPROPERTY()
	FString ChildAssetID;

	UPROPERTY()
	FString ProfileURI;
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

	UPROPERTY(BlueprintReadOnly)
	FString ProfileURI;
};

// contains the core data that UBF Subsystem needs to render an item
USTRUCT(BlueprintType)
struct FUBFRenderData
{
	GENERATED_BODY()

	FUBFRenderData() {}

	FUBFRenderData(const FString& AssetID, const FString& MetadataJson, const TArray<FUBFContextTreeData>& ContextTree)
		: AssetID(AssetID), MetadataJson(MetadataJson), ContextTree(ContextTree) {}
	
	UPROPERTY(BlueprintReadOnly)
	FString AssetID = "Invalid";

	UPROPERTY(BlueprintReadOnly)
	FString MetadataJson;
	
	UPROPERTY(BlueprintReadOnly)
	FString ProfileURI;

	UPROPERTY(BlueprintReadOnly)
	TArray<FUBFContextTreeData> ContextTree;
};

DECLARE_DYNAMIC_DELEGATE(FOnLoadCompleted);

/**
 * 
 */
UCLASS(BlueprintType)
class FUTUREVERSEUBFCONTROLLER_API UUBFItem : public UObject
{
	GENERATED_BODY()
	
public:
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
	FString GetProfileURI() const { return ProfileURI; }
	
	UFUNCTION(BlueprintCallable)
	FUBFRenderData GetRenderData() { return FUBFRenderData(ItemData.AssetID, GetMetadataJson(), GetContextTreeRef());}
	
	UFUNCTION(BlueprintCallable)
	virtual void InitializeFromRenderData(const FUBFRenderData& RenderData);

	void SetItemRegistry(const TSharedPtr<FItemRegistry>& NewItemRegistry) { ItemRegistry = NewItemRegistry; }
	
	virtual TFuture<bool> EnsureContextTreeLoaded();
	virtual TFuture<bool> EnsureProfileURILoaded();
	
	virtual TFuture<bool> LoadContextTree();
	virtual TFuture<bool> LoadProfileURI();

protected:
	UPROPERTY()
	TArray<FUBFContextTreeData> ContextTree;

	UPROPERTY()
	FString ProfileURI;
	
	UPROPERTY()
	FUBFItemData ItemData;
	
	TSharedPtr<FItemRegistry> ItemRegistry;
};

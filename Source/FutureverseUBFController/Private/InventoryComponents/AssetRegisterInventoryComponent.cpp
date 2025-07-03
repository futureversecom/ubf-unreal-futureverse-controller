// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "InventoryComponents/AssetRegisterInventoryComponent.h"

#include "AssetRegisterQueryingLibrary.h"
#include "FutureverseUBFControllerLog.h"
#include "JsonObjectConverter.h"
#include "MetadataJsonUtils.h"
#include "Items/UBFInventoryItem.h"
#include "Schemas/AssetLink.h"
#include "Schemas/NFTAssetLink.h"


void UAssetRegisterInventoryComponent::RequestFuturepassInventory(const FString& OwnerAddress,
	const FOnRequestCompleted& OnRequestCompleted)
{
	GetAssetsRequestCompleted.BindDynamic(this, &ThisClass::HandleGetFuturepassInventory);
	
	FAssetConnection AssetConnectionInput;
	AssetConnectionInput.Addresses = {OwnerAddress};
	AssetConnectionInput.First = 1000;
	UAssetRegisterQueryingLibrary::GetAssets(AssetConnectionInput, GetAssetsRequestCompleted);
	
	OnInventoryRequestCompleted = OnRequestCompleted;
}

void UAssetRegisterInventoryComponent::RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress,
	const TArray<FString>& CollectionIds, const FOnRequestCompleted& OnRequestCompleted)
{
	GetAssetsRequestCompleted.BindDynamic(this, &ThisClass::HandleGetFuturepassInventory);
	
	FAssetConnection AssetConnectionInput;
	AssetConnectionInput.Addresses = {OwnerAddress};
	AssetConnectionInput.CollectionIds = CollectionIds;
	AssetConnectionInput.First = 1000;
	UAssetRegisterQueryingLibrary::GetAssets(AssetConnectionInput, GetAssetsRequestCompleted);
	
	OnFilteredInventoryRequestCompleted = OnRequestCompleted;
}

FUBFItemData UAssetRegisterInventoryComponent::CreateItemDataFromAsset(const FAsset& Asset)
{
	const FString AssetID = FString::Printf(TEXT("%s:%s"), *Asset.CollectionId, *Asset.TokenId);
	const FString AssetName = Asset.Metadata.Properties.Contains(TEXT("name")) ? Asset.Metadata.Properties[TEXT("name")] : TEXT("");
	const FString MetadataJson = MetadataJsonUtils::GetMetadataJson(Asset.OriginalJsonData.JsonObject);
	
	return FUBFItemData(AssetID, AssetName, Asset.CollectionId, Asset.TokenId, Asset.CollectionId, MetadataJson, Asset.OriginalJsonData);
}

void UAssetRegisterInventoryComponent::HandleGetFuturepassInventory(bool bSuccess, const FAssets& Assets)
{
	GetAssetsRequestCompleted.Clear();

	if (!bSuccess)
	{
		OnInventoryRequestCompleted.ExecuteIfBound();
		OnFilteredInventoryRequestCompleted.ExecuteIfBound();
		return;
	}
	
	Inventory.Empty();

	for (auto& AssetEdge : Assets.Edges)
	{
		const auto Asset = AssetEdge.Node;
		UUBFInventoryItem* UBFItem = NewObject<UUBFInventoryItem>(this);
		const auto ItemData = CreateItemDataFromAsset(Asset);
		UBFItem->SetItemData(ItemData);

		// // add context tree
		// TArray<FUBFContextTreeData> ContextTree;
		// TArray<FUBFContextTreeRelationshipData> Relationships;
		// if (auto NFTAssetLink = Cast<UNFTAssetLink>(Asset.LinkWrapper.Links))
		// {
		// 	for (const FLink& ChildLink : NFTAssetLink->ChildLinks)
		// 	{
		// 		const FString ChildAssetID = FString::Printf(TEXT("%s:%s"), *ChildLink.Asset.CollectionId, *ChildLink.Asset.TokenId);
		// 		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UAssetRegisterInventoryComponent::CreateItemDataFromAsset Adding Child Item: %s with Relationship: %s"), *ChildAssetID, *ChildLink.Path);
		// 		Relationships.Add(FUBFContextTreeRelationshipData(ChildLink.Path, ChildAssetID));
		// 	}
		// }
		//
		// ContextTree.Add(FUBFContextTreeData(ItemData.AssetID, Relationships));
		// UBFItem->SetContextTree(ContextTree);
		
		Inventory.Add(UBFItem);
	}

	OnInventoryRequestCompleted.ExecuteIfBound();
	OnFilteredInventoryRequestCompleted.ExecuteIfBound();
}

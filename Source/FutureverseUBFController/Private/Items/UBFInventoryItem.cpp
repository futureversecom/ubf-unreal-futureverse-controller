// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "Items/UBFInventoryItem.h"

#include "FutureverseAssetLoadData.h"
#include "MetadataJsonUtils.h"


FUBFItemData UUBFInventoryItem::CreateItemDataFromMetadataJson(const FString& ContractID, const FString& TokenID,
																const FJsonObjectWrapper& MetadataJsonWrapper)
{
	const FString AssetName = MetadataJsonUtils::GetAssetName(MetadataJsonWrapper.JsonObject); 
	const FString CollectionID = MetadataJsonUtils::GetCollectionID(MetadataJsonWrapper.JsonObject); 
	const FString MetadataJson = MetadataJsonUtils::GetMetadataJson(MetadataJsonWrapper.JsonObject);
	const FString AssetID = FString::Printf(TEXT("%s:%s"), *ContractID, *AssetName);
	
	return FUBFItemData(AssetID, AssetName, ContractID, TokenID, CollectionID, MetadataJson, MetadataJsonWrapper);
}

FUBFRenderData UUBFInventoryItem::GetRenderData()
{
	return FUBFRenderData(GetAssetID(), GetContractID(), GetMetadataJson(), GetContextTreeRef());
}

void UUBFInventoryItem::InitializeFromRenderData(const FUBFRenderData& RenderData)
{
	ItemData.AssetID = RenderData.AssetID;
	ItemData.ContractID = RenderData.ContractID;
	ItemData.MetadataJson = RenderData.MetadataJson;
	ContextTree = RenderData.ContextTree;
}

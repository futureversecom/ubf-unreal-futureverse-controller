// Fill out your copyright notice in the Description page of Project Settings.


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

TArray<FFutureverseAssetLoadData> UUBFInventoryItem::GetLinkedAssetLoadData() const
{
	TArray<FFutureverseAssetLoadData> OutContractIds;
		
	for (auto& ContextTreeData : ContextTree)
	{
		TArray<FString> Out;

		ContextTreeData.RootNodeID.ParseIntoArray(Out, TEXT(":"), true);
		if (!Out.IsEmpty())
		{
			OutContractIds.Add(FFutureverseAssetLoadData(ContextTreeData.RootNodeID, Out[0]));
		}
		
		for (auto& ChildItemTuple: ContextTreeData.Children)
		{
			ChildItemTuple.Value.ParseIntoArray(Out, TEXT(":"), true);

			if (Out.IsEmpty()) continue;
			
			OutContractIds.Add(FFutureverseAssetLoadData(ChildItemTuple.Value, Out[0]));
		}
	}

	if (OutContractIds.IsEmpty())
		OutContractIds.Add(FFutureverseAssetLoadData(GetAssetID(), GetContractID()));
		
	return OutContractIds;
}

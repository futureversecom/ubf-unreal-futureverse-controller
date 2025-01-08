// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponents/EmergenceUBFInventoryComponent.h"

#include "FutureverseUBFControllerLog.h"
#include "Futurepass/GetAssetTree.h"
#include "Futurepass/GetFuturepassInventory.h"
#include "Futurepass/GetFuturepassInventoryByCollectionAndOwner.h"
#include "Items/UBFInventoryItem.h"

void UEmergenceUBFInventoryComponent::RequestFuturepassInventory(const FString& OwnerAddress,
	const FOnRequestCompleted& OnRequestCompleted)
{
	if (PendingGetInventoryRequest)
	{
		PendingGetInventoryRequest->OnGetFuturepassInventoryCompleted.RemoveDynamic(this, &ThisClass::HandleGetFuturepassInventory);
		PendingGetInventoryRequest->Cancel();
	}
	
	const TArray Address = {OwnerAddress};
	PendingGetInventoryRequest = UGetFuturepassInventory::GetFuturepassInventory(this, Address);
	PendingGetInventoryRequest->OnGetFuturepassInventoryCompleted.AddDynamic(this, &ThisClass::HandleGetFuturepassInventory);
	PendingGetInventoryRequest->Activate();

	OnInventoryRequestCompleted = OnRequestCompleted;
}

void UEmergenceUBFInventoryComponent::RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress,
	const TArray<FString>& CollectionIds, const FOnRequestCompleted& OnRequestCompleted)
{
	if (PendingGetInventoryByCollectionAndOwnerRequest)
	{
		PendingGetInventoryByCollectionAndOwnerRequest->OnGetFuturepassInventoryCompleted.RemoveDynamic(this, &ThisClass::HandleGetFuturepassInventory);
		PendingGetInventoryByCollectionAndOwnerRequest->Cancel();
	}
	
	const TArray Address = {OwnerAddress};
	PendingGetInventoryByCollectionAndOwnerRequest =
		UGetFuturepassInventoryByCollectionAndOwner::GetFuturepassInventoryByCollectionAndOwner(this, Address, CollectionIds);
	PendingGetInventoryByCollectionAndOwnerRequest->OnGetFuturepassInventoryCompleted.AddDynamic(this, &ThisClass::HandleGetFuturepassInventoryByCollectionAndOwner);
	PendingGetInventoryByCollectionAndOwnerRequest->Activate();

	OnFilteredInventoryRequestCompleted = OnRequestCompleted;
}

void UEmergenceUBFInventoryComponent::UpdateInventory(const TArray<FEmergenceInventoryItem>& Items)
{
	Inventory.Empty();
	PendingNumberOfAssetTreeRequests = 0;
	
	for (auto& Item : Items)
	{
		UUBFInventoryItem* UBFItem = NewObject<UUBFInventoryItem>(this);
		const auto ItemData = UUBFInventoryItem::CreateItemDataFromMetadataJson(Item.contract, Item.tokenId, Item.OriginalData);
		UBFItem->SetItemData(ItemData);
		
		Inventory.Add(UBFItem);

		if (ItemData.CollectionID.IsEmpty()) continue;

		const auto AssetTreeRequest = UGetFutureverseAssetTree::GetFutureverseAssetTree(this, Item.tokenId, ItemData.CollectionID);
		AssetTreeRequest->OnGetAssetTreeCompleted.AddDynamic(this, &ThisClass::HandleGetAssetTree);
		AssetTreeRequest->Activate();
		
		PendingNumberOfAssetTreeRequests++;
	}

	if (PendingNumberOfAssetTreeRequests <= 0)
	{
		OnInventoryUpdated.Broadcast();
		OnInventoryRequestCompleted.ExecuteIfBound();
		OnFilteredInventoryRequestCompleted.ExecuteIfBound();
	}
}

void UEmergenceUBFInventoryComponent::HandleGetFuturepassInventory(FEmergenceInventory Response, EErrorCode StatusCode)
{
	PendingGetInventoryRequest->OnGetFuturepassInventoryCompleted.RemoveDynamic(this, &ThisClass::HandleGetFuturepassInventory);
	PendingGetInventoryRequest = nullptr;
	
	if (StatusCode != EErrorCode::EmergenceOk) return;

	UpdateInventory(Response.items);
}

void UEmergenceUBFInventoryComponent::HandleGetFuturepassInventoryByCollectionAndOwner(FEmergenceInventory Response,
	EErrorCode StatusCode)
{
	PendingGetInventoryByCollectionAndOwnerRequest->OnGetFuturepassInventoryCompleted.RemoveDynamic(this, &ThisClass::HandleGetFuturepassInventoryByCollectionAndOwner);
	PendingGetInventoryByCollectionAndOwnerRequest = nullptr;
	
	if (StatusCode != EErrorCode::EmergenceOk) return;

	UpdateInventory(Response.items);
}

void UEmergenceUBFInventoryComponent::HandleGetAssetTree(const TArray<FFutureverseAssetTreePath>& Tree,
	EErrorCode StatusCode)
{
	PendingNumberOfAssetTreeRequests--;
	
	if (StatusCode != EErrorCode::EmergenceOk) return;
	if (Tree.IsEmpty())
	{
		// not all assets return an asset tree
		if (PendingNumberOfAssetTreeRequests <= 0)
		{
			OnInventoryUpdated.Broadcast();
			OnInventoryRequestCompleted.ExecuteIfBound();
			OnFilteredInventoryRequestCompleted.ExecuteIfBound();
		}
		return;
	}

	for (const FFutureverseAssetTreePath& TreePath : Tree)
	{
		for (const auto Item : Inventory)
		{
			if (TreePath.Id.Contains(Item->GetCombinedID()))
			{
				TArray<FUBFContextTreeData> ContextTree;
				TMap<FString, FString> Children;
				
				for (const auto& AssetTreeObjectTuple : TreePath.Objects)
				{
					// assuming that equipped tree path always starts with "path:"
					if (!AssetTreeObjectTuple.Key.StartsWith(TEXT("path:"))) continue;

					// ensure related child item exists in the inventory and add it to context tree 
					for (const auto ChildItem : Inventory)
					{
						if (AssetTreeObjectTuple.Value.Id.Contains(ChildItem->GetCombinedID()))
						{
							UE_LOG(LogFutureverseUBFController, Verbose,
							TEXT("UFutureverseInventoryComponent::HandleGetAssetTree Adding Child Item: %s with Relationship: %s"), *Item->GetAssetID(), *AssetTreeObjectTuple.Key);
							Children.Add(AssetTreeObjectTuple.Key, ChildItem->GetAssetID());
						}
					}
				}
				
				// Add root item
				UE_LOG(LogFutureverseUBFController, Verbose,
					TEXT("UFutureverseInventoryComponent::HandleGetAssetTree Adding Root Item: %s Number of relationships: %d"), *Item->GetAssetID(), Children.Num());
				
				ContextTree.Add(FUBFContextTreeData(Item->GetAssetID(), Children));
				Item->SetContextTree(ContextTree);
			}
		}
	}
	
	if (PendingNumberOfAssetTreeRequests <= 0)
	{
		OnInventoryUpdated.Broadcast();
		OnInventoryRequestCompleted.ExecuteIfBound();
		OnFilteredInventoryRequestCompleted.ExecuteIfBound();
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "FutureverseInventoryComponent.h"

#include "FutureverseUBFControllerLog.h"
#include "FutureverseUBFControllerSubsystem.h"
#include "Futurepass/GetAssetTree.h"
#include "Futurepass/GetFuturepassInventory.h"
#include "Futurepass/GetFuturepassInventoryByCollectionAndOwner.h"


// Sets default values for this component's properties
UFutureverseInventoryComponent::UFutureverseInventoryComponent()
	: PendingGetInventoryRequest(nullptr), PendingGetInventoryByCollectionAndOwnerRequest(nullptr)
{}

void UFutureverseInventoryComponent::GetInventory(TArray<UFuturePassInventoryItem*>& OutInventory)
{
	OutInventory.Append(Inventory);
}

void UFutureverseInventoryComponent::RequestFuturepassInventory(const FString& OwnerAddress, const FOnRequestCompleted& OnRequestCompleted)
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

void UFutureverseInventoryComponent::RequestFuturepassInventoryByCollectionAndOwner(const FString& OwnerAddress,
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

void UFutureverseInventoryComponent::UpdateInventory(const TArray<FEmergenceInventoryItem>& Items)
{
	// todo: do we want to always clear or compare current?
	Inventory.Empty();
	PendingNumberOfAssetTreeRequests = 0;
	
	for (auto& Item : Items)
	{
		UFuturePassInventoryItem* FuturePassItem = NewObject<UFuturePassInventoryItem>(this);
		FuturePassItem->Initialize(Item);
		
		Inventory.Add(FuturePassItem);
		
		FString CollectionId = FuturePassItem->GetCollectionID();
		if (CollectionId.IsEmpty()) continue;

		const auto AssetTreeRequest = UGetFutureverseAssetTree::GetFutureverseAssetTree(this, Item.tokenId, CollectionId);
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

void UFutureverseInventoryComponent::HandleGetFuturepassInventory(FEmergenceInventory Response, EErrorCode StatusCode)
{
	PendingGetInventoryRequest->OnGetFuturepassInventoryCompleted.RemoveDynamic(this, &ThisClass::HandleGetFuturepassInventory);
	PendingGetInventoryRequest = nullptr;
	
	if (StatusCode != EErrorCode::EmergenceOk) return;

	UpdateInventory(Response.items);
}

void UFutureverseInventoryComponent::HandleGetFuturepassInventoryByCollectionAndOwner(FEmergenceInventory Response,
	EErrorCode StatusCode)
{
	PendingGetInventoryByCollectionAndOwnerRequest->OnGetFuturepassInventoryCompleted.RemoveDynamic(this, &ThisClass::HandleGetFuturepassInventoryByCollectionAndOwner);
	PendingGetInventoryByCollectionAndOwnerRequest = nullptr;
	
	if (StatusCode != EErrorCode::EmergenceOk) return;

	UpdateInventory(Response.items);
}

void UFutureverseInventoryComponent::HandleGetAssetTree(const TArray<FFutureverseAssetTreePath>& Tree,
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

	for (auto TreePath : Tree)
	{
		for (const auto Item : Inventory)
		{
			if(TreePath.Objects.IsEmpty()) continue;
			
			if (TreePath.Id.Contains(Item->GetCombinedID()))
			{
				FFutureverseAssetTreeData AssetTree;
				AssetTree.TreePaths = Tree;
				auto LinkedItems = GetLinkedItemsForAssetTree(Tree);
				
				// Add root item
				UE_LOG(LogFutureverseUBFController, Verbose,
					TEXT("UFutureverseInventoryComponent::HandleGetAssetTree Adding Root Item: %s to Linked Items"), *Item->GetAssetID());
				LinkedItems.Add(TreePath.Id, Item->GetAssetID());
				AssetTree.LinkedItems = LinkedItems;
			
				Item->SetAssetTree(AssetTree);
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

TMap<FString, FString> UFutureverseInventoryComponent::GetLinkedItemsForAssetTree(
	const TArray<FFutureverseAssetTreePath>& AssetTree)
{
	TMap<FString, FString> LinkedItems;
	
	for (int i = 0; i < AssetTree.Num(); ++i)
	{
		for (auto AssetTreeObject : AssetTree[i].Objects)
		{
			for (const auto Item : Inventory)
			{
				if (AssetTreeObject.Value.Id.Contains(Item->GetCombinedID()))
				{
					UE_LOG(LogFutureverseUBFController, Verbose,
					TEXT("UFutureverseInventoryComponent::HandleGetAssetTree Adding Item: %s to Linked Items"), *Item->GetAssetID());
					LinkedItems.Add(AssetTreeObject.Value.Id, Item->GetAssetID());
				}
			}
		}
	}

	return LinkedItems;
}




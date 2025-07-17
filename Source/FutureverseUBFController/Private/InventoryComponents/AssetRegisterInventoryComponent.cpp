// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "InventoryComponents/AssetRegisterInventoryComponent.h"

#include "AssetRegisterQueryingLibrary.h"
#include "FutureverseUBFControllerLog.h"
#include "FutureverseUBFControllerSettings.h"
#include "MetadataJsonUtils.h"
#include "Items/AssetRegisterUBFItem.h"
#include "Items/UBFItem.h"


void UAssetRegisterInventoryComponent::RequestFuturepassInventory(const FString& OwnerAddress,
	const FOnRequestCompleted& OnRequestCompleted)
{
	GetAssetsRequestCompleted.BindDynamic(this, &ThisClass::HandleGetFuturepassInventory);
	
	FAssetConnection AssetConnectionInput;
	AssetConnectionInput.Addresses = {OwnerAddress};
	AssetConnectionInput.First = NumberOfItemsToQuery;
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
	AssetConnectionInput.First = NumberOfItemsToQuery;
	UAssetRegisterQueryingLibrary::GetAssets(AssetConnectionInput, GetAssetsRequestCompleted);
	
	OnInventoryRequestCompleted = OnRequestCompleted;
}

void UAssetRegisterInventoryComponent::RequestFuturepassInventoryWithInput(const FAssetConnection& AssetConnectionInput,
	const FOnRequestCompleted& OnRequestCompleted)
{
	GetAssetsRequestCompleted.BindDynamic(this, &ThisClass::HandleGetFuturepassInventory);

	UAssetRegisterQueryingLibrary::GetAssets(AssetConnectionInput, GetAssetsRequestCompleted);
	
	OnInventoryRequestCompleted = OnRequestCompleted;
}

FUBFItemData UAssetRegisterInventoryComponent::CreateItemDataFromAsset(const FAsset& Asset)
{
	const FString AssetID = FString::Printf(TEXT("%s:%s"), *Asset.CollectionId, *Asset.TokenId);
	const FString MetadataJson = MetadataJsonUtils::GetMetadataJson(Asset.OriginalJsonData.JsonObject);
	FString AssetName = TEXT("");
	const TSharedPtr<FJsonValue> NameField = MetadataJsonUtils::FindFieldRecursively(Asset.Metadata.Properties.JsonObject, TEXT("name"));
	if (NameField)
	{
		NameField->TryGetString(AssetName);
	}
	return FUBFItemData(AssetID, AssetName, Asset.Collection.Location, Asset.TokenId, Asset.CollectionId, MetadataJson, Asset.OriginalJsonData);
}

void UAssetRegisterInventoryComponent::HandleGetFuturepassInventory(bool bSuccess, const FAssets& Assets)
{
	GetAssetsRequestCompleted.Clear();

	if (!bSuccess)
	{
		OnInventoryRequestCompleted.ExecuteIfBound();
		return;
	}
	
	Inventory.Empty();
	// force use legacy asset profile uri if UseAssetRegisterProfiles is false
	const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
	check(Settings);
	bool bUseARAssetProfile = false;
	if (Settings)
	{
		bUseARAssetProfile = Settings->GetUseAssetRegisterProfiles();
	}
	for (auto& AssetEdge : Assets.Edges)
	{
		const auto Asset = AssetEdge.Node;
		UUBFItem* UBFItem = NewObject<UAssetRegisterUBFItem>(this);
		const auto ItemData = CreateItemDataFromAsset(Asset);

		if (!bUseARAssetProfile)
		{
			FString LegacyProfileURI = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
			FString::Printf(TEXT("%s.json"), *ItemData.ContractID));
			LegacyProfileURI = LegacyProfileURI.Replace(TEXT(" "), TEXT(""));
			UBFItem->SetAssetProfileURI(LegacyProfileURI);

			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UAssetRegisterInventoryComponent::HandleGetFuturepassInventory using legacy assetprofile URI %s"), *LegacyProfileURI);
		}
		else
		{
			FString AssetProfilesKey = TEXT("asset-profile");
			
			if (Asset.Profiles.Contains(AssetProfilesKey))
				UBFItem->SetAssetProfileURI(Asset.Profiles[AssetProfilesKey]);
		}
		
		UBFItem->SetItemData(ItemData);
		UBFItem->SetItemRegistry(ItemRegistry);
		
		ItemRegistry->RegisterItem(UBFItem->GetAssetID(), UBFItem);
		
		Inventory.Add(UBFItem);
	}
	
	// FString HasPreviousPage = Assets.PageInfo.HasPreviousPage ? TEXT("true") : TEXT("false");
	// FString HasNextPage = Assets.PageInfo.HasNextPage ? TEXT("true") : TEXT("false");
	//
	// UE_LOG(LogFutureverseUBFController, Verbose,
	// 	TEXT("Inventory hasPreviousPage: %s hasNextPage: %s NextPage: %s"),
	// 	*HasPreviousPage, *HasNextPage, *Assets.PageInfo.NextPage);

	OnInventoryRequestCompleted.ExecuteIfBound();
}

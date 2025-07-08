// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Items/AssetRegisterUBFItem.h"

#include "AssetRegisterQueryingLibrary.h"
#include "FutureverseUBFControllerLog.h"
#include "FutureverseUBFControllerSettings.h"
#include "Schemas/NFTAssetLink.h"

TFuture<bool> UAssetRegisterUBFItem::LoadContextTree()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
 	TFuture<bool> Future = Promise->GetFuture();

	UAssetRegisterQueryingLibrary::GetAssetLinks(ItemData.TokenID, ItemData.CollectionID).Next([this, Promise]
		(const FLoadAssetResult& Result)
	{
		const auto Asset = Result.Value;
		TArray<FUBFContextTreeData> ContextTree;
		TArray<FUBFContextTreeRelationshipData> Relationships;
		
		if (const UNFTAssetLink* NFTAssetLink = Cast<UNFTAssetLink>(Asset.LinkWrapper.Links))
		{
			for (const FLink& ChildLink : NFTAssetLink->ChildLinks)
			{
				const FString ChildAssetID = FString::Printf(TEXT("%s:%s"), *ChildLink.Asset.CollectionId, *ChildLink.Asset.TokenId);
				UUBFItem* ChildItem = ItemRegistry->GetItem(ChildAssetID);
				
				if (!ChildItem)
				{
					UE_LOG(LogFutureverseUBFController, Warning, TEXT("UAssetRegisterUBFItem::HandleGetAssetLinks Failed to add Child Item: %s"), *ChildAssetID);
					continue;
				}

				// TODO fix this async logic
				ChildItem->EnsureProfileURILoaded().Next(
					[this, Promise, &Relationships, ChildLink, ChildAssetID, ChildItem]
					(const bool& Result)
				{
					UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UAssetRegisterUBFItem::HandleGetAssetLinks Adding Child Item: %s with Relationship: %s"), *ChildAssetID, *ChildLink.Path);
					Relationships.Add(FUBFContextTreeRelationshipData(ChildLink.Path, ChildAssetID, ChildItem->GetProfileURI()));
				});
			}
		}
		else
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UAssetRegisterUBFItem::HandleGetAssetLinks Failed to get NFTAssetLink for Asset: %s:%s"), *Asset.CollectionId, *Asset.TokenId);
		}
		
		ContextTree.Add(FUBFContextTreeData(ItemData.AssetID, Relationships));
		SetContextTree(ContextTree);
		
		Promise->SetValue(Result.bSuccess);
	});

	return Future;
}

 TFuture<bool> UAssetRegisterUBFItem::LoadProfileURI()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
	TFuture<bool> Future = Promise->GetFuture();
	
	UAssetRegisterQueryingLibrary::GetAssetProfile(ItemData.TokenID, ItemData.CollectionID).Next([this, Promise]
		(const FLoadJsonResult& Result)
	{
		// some items don't have profile uris uploaded to AssetRegister yet
		if (!Result.bSuccess)
		{
			const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
			check(Settings);
			if (Settings)
			{
				FString LegacyProfileURI = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
				FString::Printf(TEXT("%s.json"), *ItemData.ContractID));
				LegacyProfileURI = LegacyProfileURI.Replace(TEXT(" "), TEXT(""));
				ProfileURI = LegacyProfileURI;

				UE_LOG(LogFutureverseUBFController, Log, TEXT("UAssetRegisterUBFItem::LoadProfileURI using legacy assetprofile URI %s"), *ProfileURI);
			}
			else
			{
				UE_LOG(LogFutureverseUBFController, Error, TEXT("UAssetRegisterUBFItem::LoadProfileURI UFutureverseUBFControllerSettings was null. Could not fetch asset profile URI"));
				Promise->SetValue(false);
				return;
			}
		}
		else
		{
			UE_LOG(LogFutureverseUBFController, Log, TEXT("UAssetRegisterUBFItem::LoadProfileURI got assetprofile URI %s"), *Result.Value);
			ProfileURI = Result.Value;
		}
	
		Promise->SetValue(true);
	});

	return Future;
}

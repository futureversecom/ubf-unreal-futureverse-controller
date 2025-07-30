// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Items/AssetRegisterUBFItem.h"

#include "AssetRegisterQueryingLibrary.h"
#include "FutureverseUBFControllerLog.h"
#include "FutureverseUBFControllerSettings.h"
#include "LoadActions/LoadActionUtils.h"
#include "Schemas/Unions/NFTAssetLink.h"



TFuture<bool> UAssetRegisterUBFItem::LoadContextTree()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
 	TFuture<bool> Future = Promise->GetFuture();

	TWeakObjectPtr<UAssetRegisterUBFItem> WeakThis = this;

	UAssetRegisterQueryingLibrary::GetAssetLinks(ItemData.TokenID, ItemData.CollectionID).Next([WeakThis, Promise]
		(const FLoadAssetResult& Result)
	{
		if (!WeakThis.IsValid())
		{
			Promise->SetValue(false);
			return;
		}
		
		const auto Asset = Result.Value;
		TSharedPtr<TArray<FUBFContextTreeRelationshipData>> Relationships = MakeShared<TArray<FUBFContextTreeRelationshipData>>();

		TArray<TFuture<bool>> ProfileFutures;
		
		if (const UNFTAssetLinkObject* NFTAssetLink = Cast<UNFTAssetLinkObject>(Asset.LinkWrapper.Links))
		{
			for (const FLink& ChildLink : NFTAssetLink->Data.ChildLinks)
			{
				const FString ChildAssetID = FString::Printf(TEXT("%s:%s"), *ChildLink.Asset.CollectionId, *ChildLink.Asset.TokenId);
				UUBFItem* ChildItem = WeakThis->ItemRegistry->GetItem(ChildAssetID);

				if (!ChildItem)
				{
					UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to add Child Item: %s"), *ChildAssetID);
					continue;
				}

				TFuture<bool> ProfileFuture = ChildItem->EnsureProfileURILoaded().Next(
					[ChildLink, ChildAssetID, ChildItem, Relationships](const bool& Result)
					{
						if (Result)
						{
							Relationships->Add(FUBFContextTreeRelationshipData(ChildLink.Path, ChildAssetID, ChildItem->GetProfileURI()));
						}

						return Result;
					});
	
				ProfileFutures.Add(MoveTemp(ProfileFuture));
			}
		}
		else
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UAssetRegisterUBFItem::HandleGetAssetLinks Failed to get NFTAssetLink for Asset: %s:%s"), *Asset.CollectionId, *Asset.TokenId);
		}
		
		LoadActionUtils::WhenAll(ProfileFutures).Next([WeakThis, Promise, Relationships](const TArray<bool>& Results)
		{
			const bool bAllSuccess = !Results.Contains(false);

			if (!WeakThis.IsValid())
			{
				Promise->SetValue(false);
				return;
			}
			
			WeakThis->EnsureProfileURILoaded().Next([WeakThis, Promise, Relationships, bAllSuccess](bool bResult)
			{
				if (!WeakThis.IsValid())
				{
					Promise->SetValue(false);
					return;
				}
				
				TArray<FUBFContextTreeData> ContextTree;
				ContextTree.Add(FUBFContextTreeData(WeakThis->ItemData.AssetID, *Relationships.Get(), WeakThis->GetProfileURI()));
				WeakThis->SetContextTree(ContextTree);
				
				Promise->SetValue(bAllSuccess && bResult);
			});
		});
	});

	return Future;
}

TFuture<bool> UAssetRegisterUBFItem::LoadProfileURI()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
	TFuture<bool> Future = Promise->GetFuture();

	TWeakObjectPtr<UAssetRegisterUBFItem> WeakThis = this;
	
	UAssetRegisterQueryingLibrary::GetAssetProfile(ItemData.TokenID, ItemData.CollectionID).Next([WeakThis, Promise]
		(const FLoadJsonResult& Result)
	{
		if (!WeakThis.IsValid())
		{
			Promise->SetValue(false);
			return;
		}
		
		// some items don't have profile uris uploaded to AssetRegister yet
		if (!Result.bSuccess)
		{
			const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
			check(Settings);
			if (Settings)
			{
				FString LegacyProfileURI = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
				FString::Printf(TEXT("%s.json"), *WeakThis->ItemData.ContractID));
				LegacyProfileURI = LegacyProfileURI.Replace(TEXT(" "), TEXT(""));
				WeakThis->ProfileURI = LegacyProfileURI;

				UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UAssetRegisterUBFItem::LoadProfileURI using legacy assetprofile URI %s"), *WeakThis->ProfileURI);
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
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UAssetRegisterUBFItem::LoadProfileURI got assetprofile URI %s"), *Result.Value);
			WeakThis->ProfileURI = Result.Value;
		}
	
		Promise->SetValue(true);
	});

	return Future;
}

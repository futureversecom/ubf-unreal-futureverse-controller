// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadActions/LoadAssetCatalogAction.h"

#include "FutureverseUBFControllerLog.h"
#include "GlobalArtifactProvider/DownloadRequestManager.h"
#include "Util/CatalogUtils.h"

TFuture<bool> FLoadAssetCatalogAction::TryLoadAssetCatalog(const FAssetProfile& AssetProfile,
															const FFutureverseAssetLoadData& InLoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	bBlockCompletion = true;

	TSharedPtr<FLoadAssetCatalogAction> SharedThis = AsShared();
	AssetProfileLoaded = AssetProfile;
	LoadData = InLoadData;
	
	if(!AssetProfile.GetRenderBlueprintId(LoadData.VariantID).IsEmpty() && !AssetProfile.GetRenderCatalogUri(LoadData.VariantID).IsEmpty())
	{
		SharedThis->AddPendingLoad();
		
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Catalog"),
			AssetProfile.GetRenderCatalogUri(LoadData.VariantID),
			AssetProfile.GetRenderCatalogUri(LoadData.VariantID), MemoryCacheLoader)
			.Next([SharedThis, AssetProfile, this](const UBF::FLoadStringResult& LoadResult)
		{
			if (!LoadResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render catalog from %s"), *AssetProfile.GetRenderCatalogUri(LoadData.VariantID));
				SharedThis->CompletePendingLoad();
				return;
			}
				
			CatalogUtils::ParseCatalog(LoadResult.Result.Value, SharedThis->RenderCatalogMap);
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding rendering catalog from %s"), *AssetProfile.GetRenderCatalogUri(LoadData.VariantID));
			SharedThis->CompletePendingLoad();
		});
	}

	if (!AssetProfile.GetParsingBlueprintId(LoadData.VariantID).IsEmpty() && !AssetProfile.GetParsingCatalogUri(LoadData.VariantID).IsEmpty())
	{
		SharedThis->AddPendingLoad();
		
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Catalog"), AssetProfile.GetParsingCatalogUri(LoadData.VariantID), AssetProfile.GetParsingCatalogUri(LoadData.VariantID), MemoryCacheLoader)
			.Next([SharedThis, AssetProfile, this](const UBF::FLoadStringResult& LoadResult)
		{
			if (!LoadResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri(LoadData.VariantID));
				SharedThis->CompletePendingLoad();
				return;
			}
						
			CatalogUtils::ParseCatalog(LoadResult.Result.Value, SharedThis->ParsingCatalogMap);
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri(LoadData.VariantID));
			SharedThis->CompletePendingLoad();
		});
	}
	
	bBlockCompletion = false;
	CheckPendingLoadsComplete();

	return Future;
}

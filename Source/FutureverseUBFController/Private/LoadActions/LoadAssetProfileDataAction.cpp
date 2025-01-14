#include "LoadActions/LoadAssetProfileDataAction.h"

#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"

TFuture<bool> FLoadAssetProfileDataAction::TryLoadAssetProfileData(const FAssetProfile& AssetProfile, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	bBlockCompletion = true;

	TSharedPtr<FLoadAssetProfileDataAction> SharedThis = AsShared();
	AssetProfileLoaded = AssetProfile;
	if(!AssetProfile.RenderBlueprintInstanceUri.IsEmpty())
	{
		SharedThis->AddPendingLoad();
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Blueprint"),AssetProfile.GetRenderBlueprintInstanceUri(), AssetProfile.GetRenderBlueprintInstanceUri(), MemoryCacheLoader.Get())
			.Next([SharedThis, AssetProfile, MemoryCacheLoader](const UBF::FLoadStringResult& LoadResult)
		{
			if (!LoadResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render blueprint instance from %s"), *AssetProfile.GetParsingBlueprintInstanceUri());
				SharedThis->CompletePendingLoad();
				return;
			}

			FBlueprintInstance BlueprintInstance;
			AssetProfileUtils::ParseBlueprintInstanceJson(LoadResult.Result.Value, BlueprintInstance);
			SharedThis->AssetData.RenderGraphInstance = BlueprintInstance;

			if(!AssetProfile.RenderCatalogUri.IsEmpty())
			{
				SharedThis->AddPendingLoad();
				FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Catalog"),AssetProfile.GetRenderCatalogUri(), AssetProfile.GetRenderCatalogUri(), MemoryCacheLoader.Get())
					.Next([SharedThis, AssetProfile, BlueprintInstance](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render catalog from %s"), *AssetProfile.GetRenderCatalogUri());
						SharedThis->CompletePendingLoad();
						return;
					}
				
					TMap<FString, FCatalogElement> CatalogMap;
					AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, SharedThis->RenderCatalogMap);
					UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding rendering catalog from %s"), *AssetProfile.GetRenderCatalogUri());
					SharedThis->CompletePendingLoad();
				});
			}

			SharedThis->CompletePendingLoad();
		});
	}

	if(!AssetProfile.ParsingBlueprintInstanceUri.IsEmpty())
	{
		SharedThis->AddPendingLoad();
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Blueprint"),AssetProfile.GetParsingBlueprintInstanceUri(), AssetProfile.GetParsingBlueprintInstanceUri(), MemoryCacheLoader.Get())
			.Next([SharedThis, AssetProfile, MemoryCacheLoader](const UBF::FLoadStringResult& LoadResult)
		{
			if (!LoadResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing blueprint from %s"), *AssetProfile.GetParsingBlueprintInstanceUri());
				SharedThis->CompletePendingLoad();
				return;
			}
					
			FBlueprintInstance BlueprintInstance;
			AssetProfileUtils::ParseBlueprintInstanceJson(LoadResult.Result.Value, BlueprintInstance);
			SharedThis->AssetData.ParsingGraphInstance = BlueprintInstance;
				
			if(!AssetProfile.ParsingCatalogUri.IsEmpty())
			{
				SharedThis->AddPendingLoad();
				FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Catalog"), AssetProfile.GetParsingCatalogUri(), AssetProfile.GetParsingCatalogUri(), MemoryCacheLoader.Get())
					.Next([SharedThis, AssetProfile, BlueprintInstance](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri());
						SharedThis->CompletePendingLoad();
						return;
					}
						
					AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, SharedThis->ParsingCatalogMap);
					UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri());
					SharedThis->CompletePendingLoad();
				
				});
			}

			SharedThis->CompletePendingLoad();
		});
	}

	bBlockCompletion = false;
	CheckPendingLoadsComplete();

	return Future;
}

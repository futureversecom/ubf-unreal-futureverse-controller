#include "LoadAssetProfilesAction.h"

#include <FutureverseUBFControllerSettings.h>

#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/APIUtils.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"

TFuture<bool> FLoadAssetProfilesAction::TryLoadAssetProfile(const FString& ContractId, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	FString ProfileRemotePath;
	const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
	check(Settings);
	if (Settings)
	{
		ProfileRemotePath = FString::Printf(TEXT("%s/profiles_%s.json"), *Settings->GetDefaultAssetProfilePath(), *ContractId);
	}
	else
	{
		Promise->SetValue(false);
	}

	TSharedPtr<FLoadAssetProfilesAction> SharedThis = AsShared();
	// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath, "", MemoryCacheLoader.Get()).Next(
		[SharedThis, ProfileRemotePath, MemoryCacheLoader](const UBF::FLoadStringResult& AssetProfileResult)
	{
		if(!AssetProfileResult.Result.Key)
		{
			UE_LOG(LogFutureverseUBFController, Error, TEXT("UFutureverseUBFControllerSubsystem::LoadRemoteAssetProfile failed to load remote AssetProfile from %s"), *ProfileRemotePath);
			SharedThis->Promise->SetValue(false);
			return;
		}
			
		TArray<FAssetProfile> AssetProfileEntries;
		AssetProfileUtils::ParseAssetProfileJson(AssetProfileResult.Result.Value, AssetProfileEntries);
			
		for (FAssetProfile& AssetProfile : AssetProfileEntries)
		{
			// no need to provide base path here as the values are remote not local
			AssetProfile.RelativePath = "";
			SharedThis->AssetProfiles.Add(AssetProfile.Id, AssetProfile);
			
			FFutureverseAssetData AssetData;
			SharedThis->AssetDataMap.Add(AssetProfile.Id, AssetData);
			if(!AssetProfile.RenderBlueprintInstanceUri.IsEmpty())
			{
				SharedThis->AddPendingLoad();
				FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Blueprint"),AssetProfile.GetRenderBlueprintInstanceUri(), AssetProfile.GetRenderBlueprintInstanceUri(), MemoryCacheLoader.Get())
					.Next([SharedThis, AssetProfile, MemoryCacheLoader](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render blueprint instance from %s"), *AssetProfile.GetRenderBlueprintInstanceUri());
						SharedThis->CompletePendingLoad();
						return;
					}

					FBlueprintInstance BlueprintInstance;
					AssetProfileUtils::ParseBlueprintInstanceJson(LoadResult.Result.Value, BlueprintInstance);
					SharedThis->BlueprintInstances.Add(BlueprintInstance.GetId(), BlueprintInstance);
					SharedThis->AssetDataMap[AssetProfile.Id].RenderGraphInstance = BlueprintInstance;

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
							AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, CatalogMap);
							UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding rendering catalog from %s"), *AssetProfile.GetRenderCatalogUri());
							SharedThis->Catalogs.Add(BlueprintInstance.GetId(), CatalogMap);
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
					SharedThis->BlueprintInstances.Add(BlueprintInstance.GetId(), BlueprintInstance);
					SharedThis->AssetDataMap[AssetProfile.Id].ParsingGraphInstance = BlueprintInstance;
						
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
						
							TMap<FString, FCatalogElement> CatalogMap;
							AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, CatalogMap);
							UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri());
							SharedThis->Catalogs.Add(BlueprintInstance.GetId(), CatalogMap);
							SharedThis->CompletePendingLoad();
						
						});
					}

					SharedThis->CompletePendingLoad();
				});
			}
		};

		SharedThis->CheckPendingLoadsComplete();
	});
	
	return Future;
}

void FLoadAssetProfilesAction::CompletePendingLoad()
{
	{
		FScopeLock Lock(&CriticalSection);
		PendingLoads--;
	}

	CheckPendingLoadsComplete();
}

void FLoadAssetProfilesAction::CheckPendingLoadsComplete()
{
	bool bShouldSetValue = false;

	// Lock the critical section to safely read PendingLoads
	{
		FScopeLock Lock(&CriticalSection);
		if (PendingLoads <= 0)
		{
			bShouldSetValue = true;
		}
	}

	// Set the value outside the lock to avoid deadlocks
	if (bShouldSetValue)
	{
		Promise->SetValue(true);
	}
}

void FLoadAssetProfilesAction::AddPendingLoad()
{
	FScopeLock Lock(&CriticalSection);
	PendingLoads++;
}

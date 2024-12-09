#include "LoadAssetProfilesAction.h"

#include <FutureverseUBFControllerSettings.h>

#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/APIUtils.h"
#include "ControllerLayers/AssetProfileUtils.h"
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

	// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
	APIUtils::LoadStringFromURI(ProfileRemotePath, "", MemoryCacheLoader.Get()).Next(
		[this, ProfileRemotePath, TempCacheLoader](const UBF::FLoadStringResult& AssetProfileResult)
	{
		if(!AssetProfileResult.Result.Key)
		{
			UE_LOG(LogFutureverseUBFController, Error, TEXT("UFutureverseUBFControllerSubsystem::LoadRemoteAssetProfile failed to load remote AssetProfile from %s"), *ProfileRemotePath);
			Promise->SetValue(false);
			return;
		}
			
		TArray<FAssetProfile> AssetProfileEntries;
		AssetProfileUtils::ParseAssetProfileJson(AssetProfileResult.Result.Value, AssetProfileEntries);
			
		for (FAssetProfile& AssetProfile : AssetProfileEntries)
		{
			// no need to provide base path here as the values are remote not local
			AssetProfile.RelativePath = "";
			AssetProfiles.Add(AssetProfile.Id, AssetProfile);

			if(!AssetProfile.RenderBlueprintInstanceUri.IsEmpty())
			{
				AddPendingLoad();
				APIUtils::LoadStringFromURI(AssetProfile.GetRenderBlueprintInstanceUri(), AssetProfile.GetRenderBlueprintInstanceUri(), TempCacheLoader.Get())
					.Next([this, AssetProfile](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render blueprint instance from %s"), *AssetProfile.GetParsingBlueprintInstanceUri());
						CompletePendingLoad();
						return;
					}

					FBlueprintInstance BlueprintInstance;
					AssetProfileUtils::ParseBlueprintInstanceJson(LoadResult.Result.Value, BlueprintInstance);
					BlueprintInstances.Add(BlueprintInstance.GetId(), BlueprintInstance);
					CompletePendingLoad();
				});
			
				if(!AssetProfile.RenderCatalogUri.IsEmpty())
				{
					AddPendingLoad();
					APIUtils::LoadStringFromURI(AssetProfile.GetRenderCatalogUri(), AssetProfile.GetRenderCatalogUri(), TempCacheLoader.Get())
						.Next([this, AssetProfile](const UBF::FLoadStringResult& LoadResult)
					{
						if (!LoadResult.Result.Key)
						{
							UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render catalog from %s"), *AssetProfile.GetRenderCatalogUri());
							CompletePendingLoad();
							return;
						}
							
						TMap<FString, FCatalogElement> CatalogMap;
						AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, CatalogMap);
						Catalogs.Add(AssetProfile.RenderBlueprintInstanceUri, CatalogMap);
						CompletePendingLoad();
					});
				}
			}
		
			if(!AssetProfile.ParsingBlueprintInstanceUri.IsEmpty())
			{
				AddPendingLoad();
				APIUtils::LoadStringFromURI(AssetProfile.GetParsingBlueprintInstanceUri(), AssetProfile.GetParsingBlueprintInstanceUri(), TempCacheLoader.Get())
					.Next([this, AssetProfile](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing blueprint from %s"), *AssetProfile.GetParsingBlueprintInstanceUri());
						CompletePendingLoad();
						return;
					}
							
					FBlueprintInstance BlueprintInstance;
					AssetProfileUtils::ParseBlueprintInstanceJson(LoadResult.Result.Value, BlueprintInstance);
					BlueprintInstances.Add(BlueprintInstance.GetId(), BlueprintInstance);
					CompletePendingLoad();
				});
				
				if(!AssetProfile.ParsingCatalogUri.IsEmpty())
				{
					AddPendingLoad();
					APIUtils::LoadStringFromURI(AssetProfile.GetParsingCatalogUri(), AssetProfile.GetParsingCatalogUri(), TempCacheLoader.Get())
						.Next([this, AssetProfile](const UBF::FLoadStringResult& LoadResult)
					{
						if (!LoadResult.Result.Key)
						{
							UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri());
							CompletePendingLoad();
							return;
						}
							
						TMap<FString, FCatalogElement> CatalogMap;
						AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, CatalogMap);
						Catalogs.Add(AssetProfile.ParsingBlueprintInstanceUri, CatalogMap);
						CompletePendingLoad();
							
					});
				}
			}
		};

		CheckPendingLoadsComplete();
	});
	
	return Future;
}

void FLoadAssetProfilesAction::CompletePendingLoad()
{
	PendingLoads--;

	CheckPendingLoadsComplete();
}

void FLoadAssetProfilesAction::CheckPendingLoadsComplete()
{
	if (PendingLoads <= 0)
	{
		Promise->SetValue(true);
	}
}

void FLoadAssetProfilesAction::AddPendingLoad()
{
	PendingLoads++;
}

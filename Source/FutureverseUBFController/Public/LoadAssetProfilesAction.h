#pragma once
#include "FutureverseUBFControllerSubsystem.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"

class FLoadAssetProfilesAction
{
public:
	FLoadAssetProfilesAction() {}
	TFuture<bool> TryLoadAssetProfile(const FString& ContractId, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	TMap<FString, FAssetProfile> AssetProfiles;
	TMap<FString, FBlueprintInstance> BlueprintInstances;
	TMap<FString, TMap<FString, FCatalogElement>> Catalogs;
	TMap<FString, FFutureverseAssetData> AssetDataMap;

private:
	void CompletePendingLoad();
	void CheckPendingLoadsComplete();
	void AddPendingLoad();
	
	int PendingLoads = 0;

	TSharedPtr<TPromise<bool>> Promise;
};

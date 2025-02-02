#pragma once

#include "FutureverseAssetLoadData.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"
#include "LoadActions/LoadAction.h"

class FLoadAssetProfilesAction : public TLoadAction<FLoadAssetProfilesAction>
{
public:
	FLoadAssetProfilesAction() {}
	TFuture<bool> TryLoadAssetProfile(const FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	TMap<FString, FAssetProfile> AssetProfiles;
};

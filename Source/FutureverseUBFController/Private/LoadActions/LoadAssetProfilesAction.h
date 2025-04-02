// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

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
	TFuture<bool> TryLoadAssetProfile(const struct FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	TMap<FString, FAssetProfile> AssetProfiles;
};

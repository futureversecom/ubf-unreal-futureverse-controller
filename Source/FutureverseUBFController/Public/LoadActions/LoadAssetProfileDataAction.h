// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "FutureverseUBFControllerSubsystem.h"
#include "LoadActions/LoadAction.h"
#include "ControllerLayers/APIGraphProvider.h"

class FLoadAssetProfileDataAction : public TLoadAction<FLoadAssetProfileDataAction>
{
public:
	TFuture<bool> TryLoadAssetProfileData(const FAssetProfile& AssetProfile, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	FFutureverseAssetData AssetData;
	FAssetProfile AssetProfileLoaded;
	TMap<FString, FCatalogElement> RenderCatalogMap;
	TMap<FString, FCatalogElement> ParsingCatalogMap;
};

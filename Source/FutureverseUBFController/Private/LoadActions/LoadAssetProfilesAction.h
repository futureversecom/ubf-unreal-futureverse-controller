#pragma once

#include "FutureverseUBFControllerSubsystem.h"
#include "LoadAction.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"

class FLoadAssetProfilesAction : public TLoadAction<FLoadAssetProfilesAction>
{
public:
	FLoadAssetProfilesAction() {}
	TFuture<bool> TryLoadAssetProfile(const FString& ContractId, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	TMap<FString, FAssetProfile> AssetProfiles;
};

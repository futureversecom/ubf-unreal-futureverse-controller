#pragma once
#include "FutureverseUBFControllerSubsystem.h"
#include "LoadAction.h"
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

#pragma once
#include "LoadAction.h"
#include "ControllerLayers/APIGraphProvider.h"

class FTempCacheLoader;
class FMemoryCacheLoader;
class FLoadAssetProfileDataAction;

class FLoadAssetProfileDatasAction : TLoadAction<FLoadAssetProfileDatasAction>
{
public:
	FLoadAssetProfileDatasAction() {}
	TFuture<bool> TryLoadAssetProfileDatas(const TArray<FAssetProfile>& AssetProfiles, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	TArray<TSharedPtr<FLoadAssetProfileDataAction>> ProfileLoadActions;
};

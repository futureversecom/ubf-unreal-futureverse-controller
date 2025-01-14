#pragma once
#include "LoadAssetProfilesAction.h"
#include "LoadActions/LoadAction.h"

class FTempCacheLoader;
class FMemoryCacheLoader;

class FLoadMultipleAssetDatasAction : public TLoadAction<FLoadMultipleAssetDatasAction>
{
public:
	FLoadMultipleAssetDatasAction() {}
	TFuture<bool> TryLoadAssetProfiles(const TArray<FFutureverseAssetLoadData>& AssetLoadDatas, UFutureverseUBFControllerSubsystem*
	                                   FutureverseUbfControllerSubsystem);

	
};

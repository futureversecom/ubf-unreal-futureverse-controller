// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "LoadAction.h"
#include "ControllerLayers/AssetProfile.h"

class FTempCacheLoader;
class FMemoryCacheLoader;
class FLoadAssetProfileDataAction;

class FUTUREVERSEUBFCONTROLLER_API FLoadAssetProfileDatasAction : public TLoadAction<FLoadAssetProfileDatasAction>
{
public:
	FLoadAssetProfileDatasAction() {}
	TFuture<bool> TryLoadAssetProfileDatas(const TArray<FAssetProfile>& AssetProfiles, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader);

	TArray<TSharedPtr<FLoadAssetProfileDataAction>> ProfileLoadActions;
};

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "LoadActions/LoadAction.h"

class FTempCacheLoader;
class FMemoryCacheLoader;

class FLoadMultipleAssetDatasAction : public TLoadAction<FLoadMultipleAssetDatasAction>
{
public:
	FLoadMultipleAssetDatasAction() {}
	TFuture<bool> TryLoadAssetProfiles(const TArray<struct FFutureverseAssetLoadData>& AssetLoadDatas, class UFutureverseUBFControllerSubsystem*
	                                   FutureverseUbfControllerSubsystem);

	
};

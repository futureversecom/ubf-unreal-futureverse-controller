// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "FutureverseAssetLoadData.h"
#include "ControllerLayers/AssetProfile.h"
#include "GlobalArtifactProvider/CacheLoading/MemoryCacheLoader.h"
#include "LoadActions/LoadAction.h"

class FLoadAssetProfilesAction : public TLoadAction<FLoadAssetProfilesAction>
{
public:
	FLoadAssetProfilesAction() {}
	TFuture<bool> TryLoadAssetProfile(const struct FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader);

	// temp code, to be replaced with AssetRegister SDK
	static TFuture<FString> GetAssetProfileURLFromAssetRegister(const FString& CollectionId, const FString& TokenId);
	
	TMap<FString, FAssetProfile> AssetProfiles;
};

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "FutureverseUBFControllerSubsystem.h"
#include "ControllerLayers/AssetProfile.h"
#include "LoadActions/LoadAction.h"
#include "GlobalArtifactProvider/CatalogElement.h"

class FLoadAssetProfileDataAction : public TLoadAction<FLoadAssetProfileDataAction>
{
public:
	TFuture<bool> TryLoadAssetProfileData(const FAssetProfile& AssetProfile, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader);

	FFutureverseAssetData AssetData;
	FAssetProfile AssetProfileLoaded;
	TMap<FString, UBF::FCatalogElement> RenderCatalogMap;
	TMap<FString, UBF::FCatalogElement> ParsingCatalogMap;
};

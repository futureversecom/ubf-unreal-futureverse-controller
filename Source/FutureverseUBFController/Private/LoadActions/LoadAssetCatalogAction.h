// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "FutureverseAssetLoadData.h"

#include "FutureverseUBFControllerSubsystem.h"
#include "ControllerLayers/AssetProfile.h"
#include "GlobalArtifactProvider/CatalogElement.h"
#include "LoadActions/LoadAction.h"

class FLoadAssetCatalogAction : public TLoadAction<FLoadAssetCatalogAction>
{
public:
	TFuture<bool> TryLoadAssetCatalog(const FAssetProfile& AssetProfile, const FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader);
	
	FAssetProfile AssetProfileLoaded;
	FFutureverseAssetLoadData LoadData;
	TMap<FString, UBF::FCatalogElement> RenderCatalogMap;
	TMap<FString, UBF::FCatalogElement> ParsingCatalogMap;
};

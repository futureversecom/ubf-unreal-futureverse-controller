// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "ControllerLayers/AssetProfile.h"

FString FAssetProfile::GetRenderBlueprintInstanceUri() const
{
	return RelativePath + RenderBlueprintInstanceUri;
}

FString FAssetProfile::GetRenderCatalogUri() const
{
	return RelativePath + RenderCatalogUri;
}

FString FAssetProfile::GetParsingBlueprintInstanceUri() const
{
	return RelativePath + ParsingBlueprintInstanceUri;
}

FString FAssetProfile::GetParsingCatalogUri() const
{
	return RelativePath + ParsingCatalogUri;
}
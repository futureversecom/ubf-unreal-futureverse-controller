// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "CollectionData/CollectionAssetProfiles.h"

FAssetProfile FAssetProfileData::CreateProfileFromData(const FString& BasePath) const
{
	FAssetProfile AssetProfile;

	AssetProfile.Id = Id;
	AssetProfile.RenderBlueprintInstanceUri = RenderBlueprintInstanceUri;
	AssetProfile.RenderCatalogUri = RenderCatalogUri;
	AssetProfile.ParsingBlueprintInstanceUri = ParsingBlueprintInstanceUri;
	AssetProfile.ParsingCatalogUri = ParsingCatalogUri;
	AssetProfile.RelativePath = BasePath;
	
	return AssetProfile;
}

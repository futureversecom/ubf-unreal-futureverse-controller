// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "CollectionData/CollectionAssetProfiles.h"

FAssetProfile FAssetProfileData::CreateProfileFromData(const FString& BasePath) const
{
	FAssetProfileVariant Variant(TEXT("Default"), RenderBlueprintInstanceUri,
		ParsingBlueprintInstanceUri, RenderCatalogUri, ParsingCatalogUri);
	
	Variant.RelativePath = BasePath;
	FAssetProfile AssetProfile(Id, TArray{Variant});
	
	return AssetProfile;
}

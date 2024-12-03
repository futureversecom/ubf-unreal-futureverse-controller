// Fill out your copyright notice in the Description page of Project Settings.


#include "CollectionData/CollectionAssetProfiles.h"

FAssetProfile FAssetProfileData::CreateProfileFromData(const FString& BasePath) const
{
	FAssetProfile AssetProfile;

	AssetProfile.Id = Id;
	AssetProfile.RenderBlueprintInstanceUri = GraphUri;
	AssetProfile.RenderCatalogUri = ResourceManifestUri;
	AssetProfile.RelativePath = BasePath;
	
	return AssetProfile;
}

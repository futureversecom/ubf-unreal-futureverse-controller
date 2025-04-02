// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "CachedMesh.h"
#include "glTFRuntimeAsset.h"

FCachedMeshEntry::FCachedMeshEntry(const FglTFRuntimeConfig& ImportConfig, UglTFRuntimeAsset* Asset): ImportConfig(ImportConfig), Asset(Asset)
{
	
}

bool FCachedMesh::ContainsMesh(const FglTFRuntimeConfig& Config)
{
	int Index = GetIndexForConfig(Config);
	
	if (!LoadedAssets.IsValidIndex(Index))
		return false;
	
	return LoadedAssets[Index].Asset.IsValid();
}

void FCachedMesh::AddOrReplaceMesh(const FglTFRuntimeConfig& Config, UglTFRuntimeAsset* Asset)
{
	int Index = GetIndexForConfig(Config);

	if (!LoadedAssets.IsValidIndex(Index))
	{
		LoadedAssets.Add(FCachedMeshEntry(Config, Asset));
		return;
	}

	LoadedAssets[Index].Asset = Asset;
}

UglTFRuntimeAsset* FCachedMesh::GetMesh(const FglTFRuntimeConfig& Config)
{
	int Index = GetIndexForConfig(Config);

	if (!LoadedAssets.IsValidIndex(Index))
		return nullptr;

	return LoadedAssets[Index].Asset.Get();
}

int FCachedMesh::GetIndexForConfig(const FglTFRuntimeConfig& Config)
{
	for (int i = 0; i < LoadedAssets.Num(); i++)
	{
		if (FCachedMeshEntry::AreImportConfigsEqual(LoadedAssets[i].ImportConfig, Config))
			return i;
	}

	return -1;
}

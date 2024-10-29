// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GraphProvider.h"
#include "ICacheLoader.h"

class IHttpResponse;

struct FAssetProfileEntry
{
	FAssetProfileEntry(){}
	
	FString Id;

	FString GetGraphUri() const;
	FString GetResourceManifestUri() const;

	FString GraphUri;
	FString ResourceManifestUri;
	
	FString RelativePath;
};

struct FAssetResourceManifestElement
{
	FAssetResourceManifestElement(){}
	
	FString Id;
	FString Uri;
	FString Hash;
};

/**
 * 
 */
class UBFAPICONTROLLER_API FAPIGraphProvider final : public IGraphProvider
{
public:
	FAPIGraphProvider(): GraphCacheLoader(nullptr), ResourceCacheLoader(nullptr) {}

	explicit FAPIGraphProvider(const TSharedPtr<ICacheLoader>& NewGraphCacheLoader, const TSharedPtr<ICacheLoader>& NewResourceCacheLoader);
	
	virtual TFuture<UBF::FLoadGraphResult> GetGraph(const FString& BlueprintId) override;

	virtual TFuture<UBF::FLoadTextureResult> GetTextureResource(const FString& BlueprintId, const FString& ResourceId) override;

	virtual TFuture<UBF::FLoadMeshResult> GetMeshResource(const FString& BlueprintId, const FString& ResourceId) override;

	void RegisterAssetProfile(const FAssetProfileEntry& AssetProfile);
	void RegisterAssetProfiles(const TArray<FAssetProfileEntry>& AssetProfileEntries);
	
	virtual ~FAPIGraphProvider() override = default;
private:
	// These are records of id's and locations. 
	// They are used to download graphs and resources as needed
	TMap<FString, FAssetProfileEntry> AssetProfiles;
	TMap<FString, TMap<FString, FAssetResourceManifestElement>> ResourceManifests;

	TSharedPtr<ICacheLoader> GraphCacheLoader;
	TSharedPtr<ICacheLoader> ResourceCacheLoader;
};

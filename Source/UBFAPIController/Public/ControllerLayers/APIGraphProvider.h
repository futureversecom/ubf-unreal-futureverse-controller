// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GraphProvider.h"
#include "ICacheLoader.h"

#include "APIGraphProvider.generated.h"

class IHttpResponse;

USTRUCT(BlueprintType)
struct UBFAPICONTROLLER_API FAssetProfile
{
	GENERATED_BODY()
public:
	FAssetProfile(){}
	
	FString GetGraphUri() const;
	FString GetResourceManifestUri() const;
	bool IsValid() const {return Id != FString("Invalid");}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = FString("Invalid");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString GraphUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ResourceManifestUri;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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

	void RegisterAssetProfile(const FAssetProfile& AssetProfile);
	void RegisterAssetProfiles(const TArray<FAssetProfile>& AssetProfileEntries);
	
	virtual ~FAPIGraphProvider() override = default;
private:
	// These are records of id's and locations. 
	// They are used to download graphs and resources as needed
	TMap<FString, FAssetProfile> AssetProfiles;
	TMap<FString, TMap<FString, FAssetResourceManifestElement>> ResourceManifests;

	TSharedPtr<ICacheLoader> GraphCacheLoader;
	TSharedPtr<ICacheLoader> ResourceCacheLoader;
};

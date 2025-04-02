// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "BlueprintJson.h"
#include "CachedMesh.h"
#include "glTFRuntimeParser.h"
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
	
	FString GetRenderBlueprintInstanceUri() const;
	FString GetRenderCatalogUri() const;
	FString GetParsingBlueprintInstanceUri() const;
	FString GetParsingCatalogUri() const;
	bool IsValid() const {return Id != FString("Invalid");}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("Id: %s\nRenderBlueprintInstanceUri: %s\nParsingBlueprintInstanceUri: %s\nRenderCatalogUri: %s\nParsingCatalogUri: %s\nRelativePath: %s"),
			*Id,
			*RenderBlueprintInstanceUri,
			*ParsingBlueprintInstanceUri,
			*RenderCatalogUri,
			*ParsingCatalogUri,
			*RelativePath
		);
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = FString("Invalid");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RenderBlueprintInstanceUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParsingBlueprintInstanceUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RenderCatalogUri;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParsingCatalogUri;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RelativePath;
};

struct FCatalogElement
{
	FCatalogElement(){}

	// Equal operator
	bool operator==(const FCatalogElement& Other) const
	{
		return Id == Other.Id &&
			   Type == Other.Type &&
			   Uri == Other.Uri &&
			   Hash == Other.Hash;
	}

	bool EqualWithoutHash(const FCatalogElement& Other) const
	{
		return Id == Other.Id &&
			   Type == Other.Type &&
			   Uri == Other.Uri;
	}

	// ToString method
	FString ToString() const
	{
		return FString::Printf(TEXT("Id: %s, Type: %s, Uri: %s, Hash: %s"),
							   *Id, *Type, *Uri, *Hash);
	}
	
	FString Id;
	FString Type;
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
	
	virtual TFuture<UBF::FLoadGraphResult> GetGraph(const FString& ArtifactId) override;
	
	virtual TFuture<UBF::FLoadTextureResult> GetTextureResource(const FString& ArtifactId) override;

	virtual TFuture<UBF::FLoadMeshResult> GetMeshResource(const FString& ArtifactId, const FglTFRuntimeConfig& Config) override;
	virtual void PrintBlueprintDebug(const FString& ArtifactId, const FString& ContextString) override;

	void RegisterCatalog(const FCatalogElement& CatalogElement);
	void RegisterCatalogs(const TMap<FString, FCatalogElement>& CatalogMap);
	void RegisterBlueprintJson(const FBlueprintJson& BlueprintJson);
	
	virtual ~FAPIGraphProvider() override = default;

private:

	TMap<FString, TWeakObjectPtr<UTexture2D>> LoadedTexturesMap;
	TMap<FString, FCachedMesh> LoadedMeshesMap;
	
	TMap<FString, FCatalogElement> Catalog;
	TMap<FString, FBlueprintJson> BlueprintJsons;

	TSharedPtr<ICacheLoader> GraphCacheLoader;
	TSharedPtr<ICacheLoader> ResourceCacheLoader;
};

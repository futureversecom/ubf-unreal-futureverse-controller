// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintInstance.h"
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
	
	virtual TFuture<UBF::FLoadGraphResult> GetGraph(const FString& InstanceId) override;
	
	virtual TFuture<UBF::FLoadGraphInstanceResult> GetGraphInstance(const FString& InstanceId) override;

	virtual TFuture<UBF::FLoadTextureResult> GetTextureResource(const FString& BlueprintId, const FString& ResourceId) override;

	virtual TFuture<UBF::FLoadDataArrayResult> GetMeshResource(const FString& BlueprintId, const FString& ResourceId) override;

	void RegisterCatalog(const FString& InstanceId, const FCatalogElement& Catalog);
	void RegisterCatalogs(const FString& InstanceId, const TMap<FString, FCatalogElement>& CatalogMap);
	void RegisterBlueprintInstance(const FBlueprintInstance& BlueprintInstance);
	
	virtual ~FAPIGraphProvider() override = default;
private:
	TMap<FString, TMap<FString, FCatalogElement>> Catalogs;
	TMap<FString, FBlueprintInstance> BlueprintInstances;

	TSharedPtr<ICacheLoader> GraphCacheLoader;
	TSharedPtr<ICacheLoader> ResourceCacheLoader;
};

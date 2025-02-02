// Fill out your copyright notice in the Description page of Project Settings.


#include "ControllerLayers/APIGraphProvider.h"

#include "ControllerLayers/APIUtils.h"
#include "ImageUtils.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"

namespace APIGraphProvider
{
	bool bLogUBFJson = false;
	static TAutoConsoleVariable<bool> CVarLogUBFJson(
	TEXT("UBFAPIController.Logging.LogUBFJson"),
	bLogUBFJson,
	TEXT("Enable logging UBF json when downloaded in GetGraph()"));
}

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

FAPIGraphProvider::FAPIGraphProvider(const TSharedPtr<ICacheLoader>& NewGraphCacheLoader,
									const TSharedPtr<ICacheLoader>& NewResourceCacheLoader)
{
	GraphCacheLoader = NewGraphCacheLoader;
	ResourceCacheLoader = NewResourceCacheLoader;
}

TFuture<UBF::FLoadGraphResult> FAPIGraphProvider::GetGraph(const FString& ArtifactId)
{
	TSharedPtr<TPromise<UBF::FLoadGraphResult>> Promise = MakeShareable(new TPromise<UBF::FLoadGraphResult>());
	TFuture<UBF::FLoadGraphResult> Future = Promise->GetFuture();

	UBF::FLoadGraphResult LoadResult;
	LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
	
	// get graph from catalog using graph instance's blueprint id
	if (BlueprintJsons.Contains(ArtifactId))
	{
		UBF::FGraphHandle Graph;
		if (UBF::FGraphHandle::Load(UBF::FRegistryHandle::Default(), BlueprintJsons[ArtifactId].GetGraphJson(), Graph))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("Successfully loaded Graph %s"), *ArtifactId);
		
			LoadResult.Result = TPair<bool,UBF::FGraphHandle>(true, Graph);
			Promise->SetValue(LoadResult);	
		}
		else
		{
			UE_LOG(LogUBFAPIController, Error, TEXT("Unable to load Graph BlueprintId %s"), *ArtifactId);
			Promise->SetValue(LoadResult);
		}
		return Future;
	}
	
	if (!Catalog.Contains(ArtifactId))
	{
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetGraph Failed to get graph because no graph catalog element found for ArtifactId: %s"), *ArtifactId);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	const auto GraphResource = Catalog[ArtifactId];
	UE_LOG(LogUBFAPIController, Verbose, TEXT("Try Loading Graph %s from Uri %s"), *ArtifactId, *GraphResource.Uri);
	
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Graph"),GraphResource.Uri, GraphResource.Hash, GraphCacheLoader)
	.Next([this, ArtifactId, Promise](const UBF::FLoadStringResult& LoadGraphResult)
	{
		UBF::FLoadGraphResult PromiseResult;
		PromiseResult.Result = TPair<bool,UBF::FGraphHandle>(false, UBF::FGraphHandle());
		
		if (!LoadGraphResult.Result.Key)
		{
			Promise->SetValue(PromiseResult);	
			return;
		}

		FBlueprintJson BlueprintJson(ArtifactId, LoadGraphResult.Result.Value);

		if (APIGraphProvider::CVarLogUBFJson.GetValueOnAnyThread())
			UE_LOG(LogUBFAPIController, Log, TEXT("Graph downloaded with json: \n\n %s \n\n"), *LoadGraphResult.Result.Value);
		 			
		UBF::FGraphHandle Graph;
		if (UBF::FGraphHandle::Load(UBF::FRegistryHandle::Default(), LoadGraphResult.Result.Value, Graph))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("Successfully loaded Graph %s"), *ArtifactId);
		
			PromiseResult.Result = TPair<bool,UBF::FGraphHandle>(true, Graph);
			Promise->SetValue(PromiseResult);	
		}
		else
		{
			UE_LOG(LogUBFAPIController, Error, TEXT("Unable to load Graph BlueprintId %s"), *ArtifactId);
			Promise->SetValue(PromiseResult);
		}
	});
	
	return Future;
}

TFuture<UBF::FLoadTextureResult> FAPIGraphProvider::GetTextureResource(const FString& ArtifactId)
{
	TSharedPtr<TPromise<UBF::FLoadTextureResult>> Promise = MakeShareable(new TPromise<UBF::FLoadTextureResult>());
	TFuture<UBF::FLoadTextureResult> Future = Promise->GetFuture();
	
	if (!Catalog.Contains(ArtifactId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Verbose, TEXT("FAPIGraphProvider::GetTextureResource UBF doesn't have a ResourceId %s entry"), *ArtifactId);
		LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = Catalog[ArtifactId];
	FDownloadRequestManager::GetInstance()->LoadDataFromURI(TEXT("Texture"),ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader)
	.Next([this, Promise](const UBF::FLoadDataArrayResult& DataResult)
	{
		const TArray<uint8> Data = DataResult.Result.Value;
		UBF::FLoadTextureResult LoadResult;
		
		if (Data.Num() == 0 || Data.GetData() == nullptr)
		{
			UE_LOG(LogUBF, Error, TEXT("FAPIGraphProvider::GetTextureResource Data is empty"));
			LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
			Promise->SetValue(LoadResult);
			return;
		}
							
		UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(Data);
							
		Texture->AddToRoot();
		LoadResult.Result = TPair<bool, UTexture2D*>(true, Texture);
		Promise->SetValue(LoadResult);
	});

	return Future;
}

TFuture<UBF::FLoadDataArrayResult> FAPIGraphProvider::GetMeshResource(const FString& ArtifactId)
{
	TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShareable(new TPromise<UBF::FLoadDataArrayResult>());
	TFuture<UBF::FLoadDataArrayResult> Future = Promise->GetFuture();
	
	if (!Catalog.Contains(ArtifactId))
	{
		UBF::FLoadDataArrayResult LoadResult;
		TArray<uint8> Data;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF doesn't have a ResourceId %s entry."), *ArtifactId);
		LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = Catalog[ArtifactId];
	FDownloadRequestManager::GetInstance()->LoadDataFromURI(TEXT("Mesh"),ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader)
	.Next([this, Promise](const UBF::FLoadDataArrayResult& DataResult)
	{
		const TArray<uint8> Data = DataResult.Result.Value;
		UBF::FLoadDataArrayResult LoadResult;
			
		if (Data.Num() == 0 || Data.GetData() == nullptr)
		{
			UE_LOG(LogUBF, Error, TEXT("Data is empty"));
			LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
			Promise->SetValue(LoadResult);
			return;
		}
		
		LoadResult.Result = TPair<bool, TArray<uint8>>(true, Data);
		Promise->SetValue(LoadResult);
	});

	return Future;
}

void FAPIGraphProvider::RegisterCatalog(const FCatalogElement& CatalogElement)
{
	if (Catalog.Contains(CatalogElement.Id))
	{
		// If we are just registering the same catalog element again its ok
		// TODO maybe use new hash?
		if (Catalog[CatalogElement.Id].EqualWithoutHash(CatalogElement))
			return;

		UE_LOG(LogUBFAPIController, Warning, TEXT("[APIGraphProvider] Catalog Id Collision Detected. Existing entry: %s New Entry: %s"),
			*Catalog[CatalogElement.Id].ToString(), *CatalogElement.ToString());
		return;
	}

	Catalog.Add(CatalogElement.Id, CatalogElement);
}

void FAPIGraphProvider::RegisterCatalogs(const TMap<FString, FCatalogElement>& CatalogMap)
{
	for (const auto& CatalogElement : CatalogMap)
	{
		RegisterCatalog(CatalogElement.Value);
	}
}

void FAPIGraphProvider::RegisterBlueprintJson(const FBlueprintJson& BlueprintJson)
{
	if (BlueprintJsons.Contains(BlueprintJson.GetId()))
	{
		BlueprintJsons[BlueprintJson.GetId()] = BlueprintJson;
	}
	else
	{
		BlueprintJsons.Add(BlueprintJson.GetId(), BlueprintJson);
	}
}

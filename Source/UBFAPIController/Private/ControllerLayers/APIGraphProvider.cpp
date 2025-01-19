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

TFuture<UBF::FLoadGraphResult> FAPIGraphProvider::GetGraph(const FString& CatalogId, const FString& ArtifactId)
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

	if (!Catalogs.Contains(CatalogId))
	{
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetGraph Failed to get graph because no catalog found for CatalogId: %s ArtifactId: %s"), *CatalogId, *ArtifactId);
		Promise->SetValue(LoadResult);
		return Future;
	}

	auto Catalog = Catalogs[CatalogId];
	if (!Catalog.Contains(ArtifactId))
	{
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetGraph Failed to get graph because no graph catalog element found for ArtifactId: %s"), *ArtifactId);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	const auto GraphResource = Catalog[ArtifactId];
	UE_LOG(LogUBFAPIController, Verbose, TEXT("Try Loading Graph %s from Uri %s"), *ArtifactId, *GraphResource.Uri);
	
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Graph"),GraphResource.Uri, GraphResource.Hash, GraphCacheLoader.Get())
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

TFuture<UBF::FLoadGraphInstanceResult> FAPIGraphProvider::GetGraphInstance(const FString& InstanceId)
{
	TSharedPtr<TPromise<UBF::FLoadGraphInstanceResult>> Promise = MakeShareable(new TPromise<UBF::FLoadGraphInstanceResult>());
	TFuture<UBF::FLoadGraphInstanceResult> Future = Promise->GetFuture();
	
	UBF::FLoadGraphInstanceResult LoadResult;
	
	if (!BlueprintJsons.Contains(InstanceId))
	{
		UE_LOG(LogUBFAPIController, Warning, TEXT("FAPIGraphProvider::GetGraphInstance No BlueprintInstance found for InstanceId %s"), *InstanceId);
	}
	
	LoadResult.Result = BlueprintJsons.Contains(InstanceId)
		? TPair<bool, FBlueprintJson>(true, BlueprintJsons[InstanceId])
		: TPair<bool, FBlueprintJson>(false, FBlueprintJson());
	
	Promise->SetValue(LoadResult);
	return Future;
}

TFuture<UBF::FLoadTextureResult> FAPIGraphProvider::GetTextureResource(const FString& CatalogId,
	const FString& ArtifactId)
{
	TSharedPtr<TPromise<UBF::FLoadTextureResult>> Promise = MakeShareable(new TPromise<UBF::FLoadTextureResult>());
	TFuture<UBF::FLoadTextureResult> Future = Promise->GetFuture();
	
	//TODO handle download manifest if needed
	
	if (!Catalogs.Contains(CatalogId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetTextureResource UBF BlueprintID %s doesn't have a loaded manifest"), *CatalogId);
		LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto ResourceManifestElementMap = Catalogs[CatalogId];
	if (!ResourceManifestElementMap.Contains(ArtifactId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Verbose, TEXT("FAPIGraphProvider::GetTextureResource UBF BlueprintID %s doesn't have a ResourceId %s entry"), *CatalogId, *ArtifactId);
		LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = ResourceManifestElementMap[ArtifactId];
	FDownloadRequestManager::GetInstance()->LoadDataFromURI(TEXT("Texture"),ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader.Get())
	.Next([this, Promise](const UBF::FLoadDataArrayResult& DataResult)
	{
		const TArray<uint8> Data = DataResult.Result.Value;
		UBF::FLoadTextureResult LoadResult;
		
		if (Data.Num() == 0 || Data.GetData() == nullptr)
		{
			UE_LOG(LogUBF, Error, TEXT("Data is empty"));
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

TFuture<UBF::FLoadDataArrayResult> FAPIGraphProvider::GetMeshResource(const FString& CatalogId, const FString& ArtifactId)
{
	TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShareable(new TPromise<UBF::FLoadDataArrayResult>());
	TFuture<UBF::FLoadDataArrayResult> Future = Promise->GetFuture();
	
	//TODO handle download manifest if needed

	if (!Catalogs.Contains(CatalogId))
	{
		UBF::FLoadDataArrayResult LoadResult;
		TArray<uint8> Data;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF BlueprintID %s doesn't have a loaded manifest"), *CatalogId);
		LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto ResourceManifestElementMap = Catalogs[CatalogId];
	if (!ResourceManifestElementMap.Contains(ArtifactId))
	{
		UBF::FLoadDataArrayResult LoadResult;
		TArray<uint8> Data;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF BlueprintID %s doesn't have a ResourceId %s entry."), *CatalogId, *ArtifactId);
		LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = ResourceManifestElementMap[ArtifactId];
	FDownloadRequestManager::GetInstance()->LoadDataFromURI(TEXT("Mesh"),ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader.Get())
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

void FAPIGraphProvider::RegisterCatalog(const FString& InstanceId, const FCatalogElement& Catalog)
{
	if (Catalogs.Contains(InstanceId))
	{
		TMap<FString, FCatalogElement>& CatalogMap = Catalogs[InstanceId];
		if (CatalogMap.Contains(Catalog.Id))
		{
			CatalogMap[Catalog.Id] = Catalog;
		}
		else
		{
			CatalogMap.Add(Catalog.Id, Catalog);
		}
	}
	else
	{
		TMap<FString, FCatalogElement> NewCatalogMap;
		NewCatalogMap.Add(Catalog.Id, Catalog);
		Catalogs.Add(InstanceId, NewCatalogMap);
	}
}

void FAPIGraphProvider::RegisterCatalogs(const FString& InstanceId, const TMap<FString, FCatalogElement>& CatalogMap)
{
	for (const auto& Catalog : CatalogMap)
	{
		RegisterCatalog(InstanceId, Catalog.Value);
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

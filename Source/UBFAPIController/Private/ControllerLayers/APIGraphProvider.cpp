// Fill out your copyright notice in the Description page of Project Settings.


#include "ControllerLayers/APIGraphProvider.h"

#include "ControllerLayers/APIUtils.h"
#include "ImageUtils.h"
#include "ControllerLayers/AssetProfileUtils.h"

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

TFuture<UBF::FLoadGraphResult> FAPIGraphProvider::GetGraph(const FString& InstanceId)
{
	TSharedPtr<TPromise<UBF::FLoadGraphResult>> Promise = MakeShareable(new TPromise<UBF::FLoadGraphResult>());
	TFuture<UBF::FLoadGraphResult> Future = Promise->GetFuture();

	UBF::FLoadGraphResult LoadResult;
	LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
	
	// get graph from catalog using graph instance's blueprint id
	if (!BlueprintInstances.Contains(InstanceId))
	{
		UE_LOG(LogUBFAPIController, Warning, TEXT("FAPIGraphProvider::GetGraph No BlueprintInstance found for InstanceId %s"), *InstanceId);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto BlueprintId = BlueprintInstances[InstanceId].GetBlueprintId();
	if (BlueprintId.IsEmpty())
	{
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetGraph Failed to get graph because graph instance's Blueprint Id was invalid!"));
		Promise->SetValue(LoadResult);
		return Future;
	}

	if (!Catalogs.Contains(InstanceId))
	{
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetGraph Failed to get graph because no catalog found for Blueprint Id: %s"), *BlueprintId);
		Promise->SetValue(LoadResult);
		return Future;
	}

	auto Catalog = Catalogs[InstanceId];
	if (!Catalog.Contains(BlueprintId))
	{
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetGraph Failed to get graph because no graph catalog element found for Blueprint Id: %s"), *BlueprintId);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	const auto GraphResource = Catalog[BlueprintId];
	UE_LOG(LogUBFAPIController, Verbose, TEXT("Try Loading Graph %s from Uri %s"), *BlueprintId, *GraphResource.Uri);
	
	APIUtils::LoadStringFromURI(GraphResource.Uri, GraphResource.Hash, GraphCacheLoader.Get())
	.Next([this, BlueprintId, Promise](const UBF::FLoadStringResult& LoadGraphResult)
	{
		UBF::FLoadGraphResult PromiseResult;
		PromiseResult.Result = TPair<bool,UBF::FGraphHandle>(false, UBF::FGraphHandle());
		
		if (!LoadGraphResult.Result.Key)
		{
			Promise->SetValue(PromiseResult);	
			return;
		}
		
		UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("Graph downloaded with json: %s"), *LoadGraphResult.Result.Value);
		 			
		UBF::FGraphHandle Graph;
		if (UBF::FGraphHandle::Load(UBF::FRegistryHandle::Default(), LoadGraphResult.Result.Value, Graph))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("Successfully loaded Graph %s"), *BlueprintId);
		
			PromiseResult.Result = TPair<bool,UBF::FGraphHandle>(true, Graph);
			Promise->SetValue(PromiseResult);	
		}
		else
		{
			UE_LOG(LogUBFAPIController, Error, TEXT("Unable to load Graph BlueprintId %s"), *BlueprintId);
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
	
	if (!BlueprintInstances.Contains(InstanceId))
	{
		UE_LOG(LogUBFAPIController, Warning, TEXT("FAPIGraphProvider::GetGraphInstance No BlueprintInstance found for InstanceId %s"), *InstanceId);
	}
	
	LoadResult.Result = BlueprintInstances.Contains(InstanceId)
		? TPair<bool, FBlueprintInstance>(true, BlueprintInstances[InstanceId])
		: TPair<bool, FBlueprintInstance>(false, FBlueprintInstance());
	
	Promise->SetValue(LoadResult);
	return Future;
}

TFuture<UBF::FLoadTextureResult> FAPIGraphProvider::GetTextureResource(const FString& BlueprintId,
	const FString& ResourceId)
{
	TSharedPtr<TPromise<UBF::FLoadTextureResult>> Promise = MakeShareable(new TPromise<UBF::FLoadTextureResult>());
	TFuture<UBF::FLoadTextureResult> Future = Promise->GetFuture();
	
	//TODO handle download manifest if needed
	
	if (!Catalogs.Contains(BlueprintId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetTextureResource UBF BlueprintID %s doesn't have a loaded manifest"), *BlueprintId);
		LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto ResourceManifestElementMap = Catalogs[BlueprintId];
	if (!ResourceManifestElementMap.Contains(ResourceId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Verbose, TEXT("FAPIGraphProvider::GetTextureResource UBF BlueprintID %s doesn't have a ResourceId %s entry"), *BlueprintId, *ResourceId);
		LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = ResourceManifestElementMap[ResourceId];
	APIUtils::LoadDataFromURI(ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader.Get())
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

TFuture<UBF::FLoadDataArrayResult> FAPIGraphProvider::GetMeshResource(const FString& BlueprintId, const FString& ResourceId)
{
	TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShareable(new TPromise<UBF::FLoadDataArrayResult>());
	TFuture<UBF::FLoadDataArrayResult> Future = Promise->GetFuture();
	
	//TODO handle download manifest if needed

	if (!Catalogs.Contains(BlueprintId))
	{
		UBF::FLoadDataArrayResult LoadResult;
		TArray<uint8> Data;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF BlueprintID %s doesn't have a loaded manifest"), *BlueprintId);
		LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto ResourceManifestElementMap = Catalogs[BlueprintId];
	if (!ResourceManifestElementMap.Contains(ResourceId))
	{
		UBF::FLoadDataArrayResult LoadResult;
		TArray<uint8> Data;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF BlueprintID %s doesn't have a ResourceId %s entry."), *BlueprintId, *ResourceId);
		LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = ResourceManifestElementMap[ResourceId];
	APIUtils::LoadDataFromURI(ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader.Get())
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
		auto CatalogMap = Catalogs[InstanceId];
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

void FAPIGraphProvider::RegisterBlueprintInstance(const FString& InstanceId,
	const FBlueprintInstance& BlueprintInstance)
{
	if (BlueprintInstances.Contains(InstanceId))
	{
		BlueprintInstances[InstanceId] = BlueprintInstance;
	}
	else
	{
		BlueprintInstances.Add(InstanceId, BlueprintInstance);
	}
}

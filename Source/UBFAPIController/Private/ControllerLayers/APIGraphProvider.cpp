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

TMap<FString, UBF::FDynamicHandle>& FBlueprintInstance::GetVariables()
{
	TMap<FString, UBF::FDynamicHandle> Variables;
	// todo
	return Variables;
}

FAPIGraphProvider::FAPIGraphProvider(const TSharedPtr<ICacheLoader>& NewGraphCacheLoader,
									const TSharedPtr<ICacheLoader>& NewResourceCacheLoader)
{
	GraphCacheLoader = NewGraphCacheLoader;
	ResourceCacheLoader = NewResourceCacheLoader;
}

TFuture<UBF::FLoadGraphResult> FAPIGraphProvider::GetGraph(const FString& BlueprintId)
{
	TSharedPtr<TPromise<UBF::FLoadGraphResult>> Promise = MakeShareable(new TPromise<UBF::FLoadGraphResult>());
	TFuture<UBF::FLoadGraphResult> Future = Promise->GetFuture();

	if (AssetProfiles.Contains(BlueprintId))
	{
		const FString RenderBlueprintInstanceUri = AssetProfiles[BlueprintId].GetRenderBlueprintInstanceUri();
		const FString RenderCatalogUri = AssetProfiles[BlueprintId].GetRenderCatalogUri();
		
		// don't block Graph download while downloading manifest
		UE_LOG(LogUBFAPIController, Verbose, TEXT("Loading BlueprintId %s with RenderBlueprintInstanceUri %s and RenderCatalogUri %s"), *BlueprintId, *RenderBlueprintInstanceUri, *RenderCatalogUri); 
		 APIUtils::LoadStringFromURI(RenderCatalogUri, "", GraphCacheLoader.Get())
		 .Next([this, BlueprintId, RenderBlueprintInstanceUri, Promise](const UBF::FLoadStringResult& ResourceManifestResult)
		 {
		 	if (!ResourceManifestResult.Result.Key)
		 	{
		 		UBF::FLoadGraphResult LoadResult;
		 		LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
		 		Promise->SetValue(LoadResult);	
		 		return;
		 	}
		
		 	TMap<FString, FCatalogElement> CatalogMap;
		 	AssetProfileUtils::ParseCatalog(ResourceManifestResult.Result.Value, CatalogMap);
		 	if (Catalogs.Contains(BlueprintId))
		 	{
		 		Catalogs[BlueprintId] = CatalogMap;
		 	}
		 	else
		 	{
		 		Catalogs.Add(BlueprintId, CatalogMap);
		 	}
		 	
		 	UE_LOG(LogUBFAPIController, Verbose, TEXT("Try Loading Graph %s from cachewith RenderBlueprintInstanceUri %s"), *BlueprintId, *RenderBlueprintInstanceUri);
		
		 	APIUtils::LoadStringFromURI(RenderBlueprintInstanceUri, "", GraphCacheLoader.Get())
		 	.Next([this, BlueprintId, Promise](const UBF::FLoadStringResult& GraphResult)
		 	{
		 		UBF::FLoadGraphResult LoadResult;
		
		 		if (!GraphResult.Result.Key)
		 		{
		 			LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
		 			Promise->SetValue(LoadResult);
		 		}
		 		else
		 		{
		 			UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("Graph downloaded with json: %s"), *GraphResult.Result.Value);
		 			
		 			UBF::FGraphHandle Graph;
		 			if (UBF::FGraphHandle::Load(UBF::FRegistryHandle::Default(), GraphResult.Result.Value, Graph))
		 			{
		 				UE_LOG(LogUBFAPIController, Verbose, TEXT("Successfully loaded Graph %s"), *BlueprintId);
		 				LoadResult.Result = TPair<bool,UBF::FGraphHandle>(true, Graph);
		 				Promise->SetValue(LoadResult);
		 			}
		 			else
		 			{
		 				UE_LOG(LogUBFAPIController, Error, TEXT("Unable to load Graph BlueprintId %s"), *BlueprintId);
		 				LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
		 				Promise->SetValue(LoadResult);
		 			}
		 		}
		 	});
		});
	}
	else
	{
		UBF::FLoadGraphResult LoadResult;
		UE_LOG(LogUBFAPIController, Warning, TEXT("No AssetProfile found for BlueprintId %s"), *BlueprintId);
		LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	return Future;
}

TFuture<UBF::FLoadGraphInstanceResult> FAPIGraphProvider::GetGraphInstance(const FString& InstanceId)
{
	// TODO: Implement this
	TSharedPtr<TPromise<UBF::FLoadGraphInstanceResult>> Promise = MakeShareable(new TPromise<UBF::FLoadGraphInstanceResult>());
	TFuture<UBF::FLoadGraphInstanceResult> Future = Promise->GetFuture();
	UBF::FLoadGraphInstanceResult LoadResult;

	LoadResult.Result = TPair<bool, IBlueprintInstance*>(false, nullptr);
	
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

void FAPIGraphProvider::RegisterAssetProfile(const FAssetProfile& AssetProfile)
{
	if (AssetProfiles.Contains(AssetProfile.Id))
	{
		AssetProfiles[AssetProfile.Id] = AssetProfile;
	}
	else
	{
		AssetProfiles.Add(AssetProfile.Id, AssetProfile);
	}
}

void FAPIGraphProvider::RegisterAssetProfiles(const TArray<FAssetProfile>& AssetProfileEntries)
{
	for (const FAssetProfile& Entry : AssetProfileEntries)
	{
		RegisterAssetProfile(Entry);
	}
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

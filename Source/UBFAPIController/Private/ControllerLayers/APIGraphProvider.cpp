// Fill out your copyright notice in the Description page of Project Settings.


#include "ControllerLayers/APIGraphProvider.h"

#include "ControllerLayers/APIUtils.h"
#include "glTFRuntimeAsset.h"
#include "glTFRuntimeParser.h"
#include "ImageUtils.h"
#include "ControllerLayers/AssetProfileUtils.h"

FString FAssetProfile::GetGraphUri() const
{
	return RelativePath + GraphUri;
}

FString FAssetProfile::GetResourceManifestUri() const
{
	return RelativePath + ResourceManifestUri;
}

FString FAssetProfile::GetParsingBlueprintUri() const
{
	return RelativePath + ParsingBlueprintUri;
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
		const FString GraphUri = AssetProfiles[BlueprintId].GetGraphUri();
		const FString ResourceManifestUri = AssetProfiles[BlueprintId].GetResourceManifestUri();

		UE_LOG(LogUBFAPIController, Verbose, TEXT("Loading BlueprintId %s with GraphURI %s and ResourceManifestUri %s"), *BlueprintId, *GraphUri, *ResourceManifestUri);
		
		// TODO don't block Graph download while downloading manifest
		 APIUtils::LoadStringFromURI(ResourceManifestUri, "", GraphCacheLoader.Get())
		 .Next([this, BlueprintId, GraphUri, Promise](const UBF::FLoadStringResult& ResourceManifestResult)
		 {
		 	if (!ResourceManifestResult.Result.Key)
		 	{
		 		UBF::FLoadGraphResult LoadResult;
		 		LoadResult.Result = TPair<bool, UBF::FGraphHandle>(false, UBF::FGraphHandle());
		 		Promise->SetValue(LoadResult);	
		 		return;
		 	}
		
		 	TMap<FString, FAssetResourceManifestElement> ResourceManifestElementMap;
		 	AssetProfileUtils::ParseResourceManifest(ResourceManifestResult.Result.Value, ResourceManifestElementMap);
		 	if (ResourceManifests.Contains(BlueprintId))
		 	{
		 		ResourceManifests[BlueprintId] = ResourceManifestElementMap;
		 	}
		 	else
		 	{
		 		ResourceManifests.Add(BlueprintId, ResourceManifestElementMap);
		 	}
		 	
		 	UE_LOG(LogUBFAPIController, Verbose, TEXT("Try Loading Graph  %s from cachewith GraphURI %s"), *BlueprintId, *GraphUri);
		
		 	APIUtils::LoadStringFromURI(GraphUri, "", GraphCacheLoader.Get())
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

TFuture<UBF::FLoadTextureResult> FAPIGraphProvider::GetTextureResource(const FString& BlueprintId,
	const FString& ResourceId)
{
	TSharedPtr<TPromise<UBF::FLoadTextureResult>> Promise = MakeShareable(new TPromise<UBF::FLoadTextureResult>());
	TFuture<UBF::FLoadTextureResult> Future = Promise->GetFuture();
	
	//TODO handle download manifest if needed
	
	if (!ResourceManifests.Contains(BlueprintId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetTextureResource UBF BlueprintID %s doesn't have a loaded manifest"), *BlueprintId);
		LoadResult.Result = TPair<bool, UTexture2D*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto ResourceManifestElementMap = ResourceManifests[BlueprintId];
	if (!ResourceManifestElementMap.Contains(ResourceId))
	{
		UBF::FLoadTextureResult LoadResult;
		UE_LOG(LogUBFAPIController, Warning, TEXT("FAPIGraphProvider::GetTextureResource UBF BlueprintID %s doesn't have a ResourceId %s entry"), *BlueprintId, *ResourceId);
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

TFuture<UBF::FLoadMeshResult> FAPIGraphProvider::GetMeshResource(const FString& BlueprintId, const FString& ResourceId)
{
	TSharedPtr<TPromise<UBF::FLoadMeshResult>> Promise = MakeShareable(new TPromise<UBF::FLoadMeshResult>());
	TFuture<UBF::FLoadMeshResult> Future = Promise->GetFuture();
	
	//TODO handle download manifest if needed

	if (!ResourceManifests.Contains(BlueprintId))
	{
		UBF::FLoadMeshResult LoadResult;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF BlueprintID %s doesn't have a loaded manifest"), *BlueprintId);
		LoadResult.Result = TPair<bool, UglTFRuntimeAsset*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}
	
	auto ResourceManifestElementMap = ResourceManifests[BlueprintId];
	if (!ResourceManifestElementMap.Contains(ResourceId))
	{
		UBF::FLoadMeshResult LoadResult;
		UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource UBF BlueprintID %s doesn't have a ResourceId %s entry."), *BlueprintId, *ResourceId);
		LoadResult.Result = TPair<bool, UglTFRuntimeAsset*>(false, nullptr);
		Promise->SetValue(LoadResult);
		return Future;
	}

	const auto ResourceManifestElement = ResourceManifestElementMap[ResourceId];
	APIUtils::LoadDataFromURI(ResourceManifestElement.Uri, ResourceManifestElement.Hash, ResourceCacheLoader.Get())
	.Next([this, Promise](const UBF::FLoadDataArrayResult& DataResult)
	{
		const TArray<uint8> Data = DataResult.Result.Value;
		UBF::FLoadMeshResult LoadResult;
		UglTFRuntimeAsset* Asset = NewObject<UglTFRuntimeAsset>();
				
		if (Data.Num() == 0 || Data.GetData() == nullptr)
		{
			UE_LOG(LogUBF, Error, TEXT("Data is empty"));
			LoadResult.Result = TPair<bool, UglTFRuntimeAsset*>(false, Asset);
			Promise->SetValue(LoadResult);
			return;
		}
				
		FglTFRuntimeConfig LoaderConfig;
		LoaderConfig.TransformBaseType = EglTFRuntimeTransformBaseType::YForward;
		Asset->RuntimeContextObject = LoaderConfig.RuntimeContextObject;
		Asset->RuntimeContextString = LoaderConfig.RuntimeContextString;
	
		if (!Asset->LoadFromData(Data.GetData(), Data.Num(), LoaderConfig))
		{
			UE_LOG(LogUBFAPIController, Error, TEXT("FAPIGraphProvider::GetMeshResource Failed to Load Data"));
			LoadResult.Result = TPair<bool, UglTFRuntimeAsset*>(false, Asset);
			Promise->SetValue(LoadResult);
		}
		else
		{
			LoadResult.Result = TPair<bool, UglTFRuntimeAsset*>(true, Asset);
			Promise->SetValue(LoadResult);
		}
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
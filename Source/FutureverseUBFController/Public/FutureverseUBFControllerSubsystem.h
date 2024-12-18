// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FuturePassInventoryItem.h"
#include "UBFRuntimeController.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/APISubGraphResolver.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FutureverseUBFControllerSubsystem.generated.h"

class FLoadMultipleAssetDatasAction;
class FLoadAssetProfilesAction;
class UCollectionRemappings;
class UCollectionAssetProfiles;

struct FFutureverseAssetData
{
	FFutureverseAssetData(){}
	
	FBlueprintInstance RenderGraphInstance;
	FBlueprintInstance ParsingGraphInstance;
};

/**
 * 
 */
UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UFutureverseUBFControllerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFutureverseUBFControllerSubsystem();
	
	static UFutureverseUBFControllerSubsystem* Get(const UObject* WorldContext);
	
	// Used for rendering an item by itself without asset tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItem(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	// Used for rendering an item and other linked items using context tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItemTree(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);

	// Asset profiles contain the path for Blueprints, Parsing Blueprints and ResourceManifests associated with an UFuturePassInventoryItem
	// Currently this data needs to provided by the experience using the below functions

	// Register AssetProfile from data asset
	UFUNCTION(BlueprintCallable)
	void RegisterAssetProfilesFromData(UCollectionAssetProfiles* CollectionAssetProfiles);
	// Register AssetProfile from json
	UFUNCTION(BlueprintCallable)
	void RegisterAssetProfilesFromJson(const FString& Json, const FString& BasePath);
	// Register AssetProfile directly
	UFUNCTION(BlueprintCallable)
	void RegisterAssetProfile(const FAssetProfile& AssetProfile);
	// Clears all registered AssetProfiles
	UFUNCTION(BlueprintCallable)
	void ClearAssetProfiles();

	// Asset data contains the actual blueprint instances that were fetched using the urls from asset profiles
	void RegisterAssetData(const FString& AssetId, const FFutureverseAssetData& AssetData);

private:
	void ExecuteGraph(const FString& GraphId, UUBFRuntimeController* Controller,
		IGraphProvider* GraphProvider, ISubGraphResolver* SubGraphResolver,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	void BuildContextTreeFromAssetTree(const TSharedPtr<FContextTree>& ContextTree, const FFutureverseAssetTreeData& AssetTree,
		const FString& TraitTargetId, const TMap<FString, UBF::FDynamicHandle>& Traits) const;

	void ParseInputs(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete,
		TSharedPtr<FContextTree> ContextTree, const bool bShouldBuildContextTree);

	TFuture<bool> TryLoadAssetDatas(const TArray<struct FFutureverseAssetLoadData>& LoadDatas);
	
	TFuture<bool> TryLoadAssetData(const FFutureverseAssetLoadData& LoadData);
	TFuture<bool> TryLoadAssetProfileData(const FString& AssetID);
	TFuture<bool> TryLoadAssetProfile(const FString& ContractId);
	
	TFuture<TMap<FString, UUBFBindingObject*>> GetTraitsForItem(const FString& ParsingGraphId,
		UUBFRuntimeController* Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const;
	
	TSharedPtr<FAPIGraphProvider> APIGraphProvider;
	TSharedPtr<FAPISubGraphResolver> APISubGraphProvider;
	
	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader;
	TSharedPtr<FTempCacheLoader> TempCacheLoader;

	TMap<FString, FAssetProfile> AssetProfiles;
	TMap<FString, FFutureverseAssetData> AssetDataMap;

	TSet<TSharedPtr<FLoadAssetProfilesAction>> PendingActions;
	TSet<TSharedPtr<class FLoadAssetProfileDataAction>> PendingDataActions;
	TSet<TSharedPtr<FLoadMultipleAssetDatasAction>> PendingMultiLoadActions;
	
	mutable UBF::FExecutionContextHandle LastParsingGraphExecutionContextHandle;
	mutable UBF::FGraphHandle LastParsedGraph;
	
	friend class UFuturePassInventoryItem;
	friend class FLoadMultipleAssetDatasAction;
};

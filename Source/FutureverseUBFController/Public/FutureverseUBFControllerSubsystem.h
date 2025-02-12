// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetIdMap.h"
#include "UBFRuntimeController.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/APISubGraphResolver.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"
#include "Items/UBFInventoryItem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FutureverseUBFControllerSubsystem.generated.h"

class FLoadMultipleAssetDatasAction;
class FLoadAssetProfilesAction;
class UCollectionRemappings;
class UCollectionAssetProfiles;

struct FFutureverseAssetData
{
	FFutureverseAssetData(){}
	
	FBlueprintJson RenderGraphInstance;
	FBlueprintJson ParsingGraphInstance;
};



UENUM(BlueprintType)
enum class EEnvironment : uint8
{
	Development,
	Staging,
	Production,
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
	void RenderItem(UUBFInventoryItem* Item, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	// Used for rendering an item and other linked items using context tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItemTree(UUBFInventoryItem* Item, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);

	// Asset profiles contain the path for Blueprints, Parsing Blueprints and ResourceManifests associated with an UFuturePassInventoryItem
	// Currently this data needs to provided by the experience using the below functions

	// Register AssetProfile from data asset
	UFUNCTION(BlueprintCallable)
	void RegisterAssetProfilesFromData(const EEnvironment& Environment, UCollectionAssetProfiles* CollectionAssetProfiles);
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

	TFuture<bool> TryLoadAssetDatas(const TArray<struct FFutureverseAssetLoadData>& LoadDatas);
	
private:
	void ExecuteItemGraph(UUBFInventoryItem* Item,
	                  UUBFRuntimeController* Controller, const bool bShouldBuildContextTree,
	                  const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	void CreateBlueprintInstancesFromContextTree(const TArray<FUBFContextTreeData>& UBFContextTree,
	                                        const FString& RootAssetId, const TMap<FString, UBF::FDynamicHandle>& RootTraits, TArray<UBF::FBlueprintInstance>& OutBlueprintInstances) const;

	void ParseInputsThenExecute(UUBFInventoryItem* Item, UUBFRuntimeController* Controller,
	                            const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete,
	                            const bool bShouldBuildContextTree);

	void ExecuteGraph(UUBFInventoryItem* Item, UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, bool
	                  bShouldBuildContextTree, const FOnComplete& OnComplete);
	
	
	TFuture<bool> TryLoadAssetData(const FFutureverseAssetLoadData& LoadData);
	TFuture<bool> TryLoadAssetProfileData(const FString& AssetID);
	TFuture<bool> TryLoadAssetProfile(const FFutureverseAssetLoadData& LoadData);
	
	
	TFuture<TMap<FString, UUBFBindingObject*>> GetTraitsForItem(const FString& ParsingGraphId,
		UUBFRuntimeController* Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const;
	
	TSharedPtr<FAPIGraphProvider> APIGraphProvider;
	
	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader;
	TSharedPtr<FTempCacheLoader> TempCacheLoader;

	TAssetIdMap<FAssetProfile> AssetProfiles;
	TAssetIdMap<FFutureverseAssetData> AssetDataMap;

	mutable TMap<FString, UBF::FExecutionContextHandle> PendingParsingGraphContexts;

	TSet<TSharedPtr<FLoadAssetProfilesAction>> PendingActions;
	TSet<TSharedPtr<class FLoadAssetProfileDataAction>> PendingDataActions;
	TSet<TSharedPtr<FLoadMultipleAssetDatasAction>> PendingMultiLoadActions;
	
	friend class UUBFInventoryItem;
	friend class FLoadMultipleAssetDatasAction;
};

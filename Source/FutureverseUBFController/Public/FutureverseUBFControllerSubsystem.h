// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetIdMap.h"
#include "UBFRuntimeController.h"
#include "ControllerLayers/AssetProfile.h"
#include "GlobalArtifactProvider/CacheLoading/MemoryCacheLoader.h"
#include "Items/UBFInventoryItem.h"
#include "Items/UBFRenderDataContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FutureverseUBFControllerSubsystem.generated.h"

class FLoadMultipleAssetDatasAction;
class FLoadAssetProfilesAction;
class UCollectionRemappings;
class UCollectionAssetProfiles;

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
	void RenderItem(UUBFInventoryItem* Item, const FString& VariantID, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	// Used for rendering an item and other linked items using context tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItemTree(UUBFInventoryItem* Item, const FString& VariantID, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	// Used for rendering an item by itself without asset tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItemFromRenderData(const FUBFRenderData& RenderData, const FString& VariantID, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);

	// Used for rendering an item by itself without asset tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItemTreeFromRenderData(const FUBFRenderData& RenderData, const FString& VariantID, UUBFRuntimeController* Controller,
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
	
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	TFuture<bool> TryLoadAssetDatas(const TArray<struct FFutureverseAssetLoadData>& LoadDatas);

private:
	void RenderItemInternal(FUBFRenderDataPtr RenderData, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	void RenderItemTreeInternal(FUBFRenderDataPtr RenderData, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	void ExecuteItemGraph(
		FUBFRenderDataPtr RenderData, UUBFRuntimeController* Controller,
		const bool bShouldBuildContextTree, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete) const;
	
	void CreateBlueprintInstancesFromContextTree(const FUBFRenderDataPtr& RenderData, const TArray<FUBFContextTreeData>& UBFContextTree,
	                                        const FString& RootAssetId, TArray<UBF::FExecutionInstanceData>& OutBlueprintInstances) const;

	void ParseInputsThenExecute(FUBFRenderDataPtr RenderData, UUBFRuntimeController* Controller,
	                            const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete,
	                            const bool bShouldBuildContextTree) const;

	void ExecuteGraph(const FUBFRenderDataPtr& RenderData, UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, bool
	                  bShouldBuildContextTree, const FOnComplete& OnComplete) const;

	TFuture<bool> EnsureAssetDatasLoaded(const TArray<struct FFutureverseAssetLoadData>& LoadDatas);
	TFuture<bool> EnsureAssetDataLoaded(const FFutureverseAssetLoadData& LoadData);
	
	TFuture<bool> EnsureAssetProfilesLoaded(const FFutureverseAssetLoadData& LoadData);
	TFuture<bool> EnsureCatalogsLoaded(const FFutureverseAssetLoadData& LoadData);

	bool IsAssetProfileLoaded(const FFutureverseAssetLoadData& LoadData) const;
	bool IsCatalogLoaded(const FFutureverseAssetLoadData& LoadData) const;
	
	TFuture<TMap<FString, UUBFBindingObject*>> GetTraitsForItem(const FString& ParsingGraphId,
		UUBFRuntimeController* Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const;

	bool IsSubsystemValid() const;

	TAssetIdMap<FAssetProfile> AssetProfiles;
	TSet<FString> LoadedVariantCatalogs;

	bool bIsInitialized = false;

	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader = MakeShared<FMemoryCacheLoader>();
	
	friend class UUBFInventoryItem;
	friend class FLoadMultipleAssetDatasAction;
};

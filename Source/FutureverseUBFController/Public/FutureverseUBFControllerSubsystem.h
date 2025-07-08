// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetIdMap.h"
#include "UBFRuntimeController.h"
#include "ControllerLayers/AssetProfile.h"
#include "AssetProfile/AssetProfileRegistrySubsystem.h"
#include "GlobalArtifactProvider/CacheLoading/MemoryCacheLoader.h"
#include "Items/UBFItem.h"
#include "Items/UBFRenderDataContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FutureverseUBFControllerSubsystem.generated.h"

struct FLoadAssetProfileResult;
struct FLoadLinkedAssetProfilesResult;
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
	void RenderItem(UUBFItem* Item, const FString& VariantID, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	// Used for rendering an item and other linked items using context tree
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnComplete"))
	void RenderItemTree(UUBFItem* Item, const FString& VariantID, UUBFRuntimeController* Controller,
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
	
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	class FRenderItemInfo
	{
	public:
		FUBFRenderDataPtr RenderData;
		TWeakObjectPtr<UUBFRuntimeController> Controller;
		TMap<FString, UUBFBindingObject*> InputMap;
		TAssetIdMap<FAssetProfile> AssetProfiles;
		FOnComplete OnComplete;
	};
	
	void RenderItemInternal(TSharedPtr<FRenderItemInfo> RenderItemInfo);
	
	void RenderItemTreeInternal(TSharedPtr<FRenderItemInfo> RenderItemInfo);
	
	void ExecuteItemGraph(TSharedPtr<FRenderItemInfo> RenderItemInfo, const bool bShouldBuildContextTree) const;
	
	void CreateBlueprintInstancesFromContextTree(TSharedPtr<FRenderItemInfo> RenderItemInfo, const TArray<FUBFContextTreeData>& UBFContextTree,
	                                        const FString& RootAssetId, TArray<UBF::FExecutionInstanceData>& OutBlueprintInstances) const;

	void ParseInputsThenExecute(TSharedPtr<FRenderItemInfo> RenderItemInfo,
	                            const bool bShouldBuildContextTree) const;

	void ExecuteGraph(TSharedPtr<FRenderItemInfo> RenderItemInfo, const bool bShouldBuildContextTree) const;

	TFuture<FLoadLinkedAssetProfilesResult> EnsureAssetDatasLoaded(const TArray<struct FFutureverseAssetLoadData>& LoadDatas);
	TFuture<FLoadAssetProfileResult> EnsureAssetDataLoaded(const FFutureverseAssetLoadData& LoadData);
	
	TFuture<FLoadAssetProfileResult> EnsureAssetProfilesLoaded(const FFutureverseAssetLoadData& LoadData) const;
	TFuture<bool> EnsureCatalogsLoaded(const FFutureverseAssetLoadData& LoadData, const FAssetProfile& AssetProfile);
	
	bool IsCatalogLoaded(const FFutureverseAssetLoadData& LoadData) const;
	
	TFuture<TMap<FString, UUBFBindingObject*>> GetTraitsForItem(const FString& ParsingGraphId,
		const TWeakObjectPtr<UUBFRuntimeController>& Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const;

	bool IsSubsystemValid() const;
	
	TSet<FString> LoadedVariantCatalogs;

	bool bIsInitialized = false;

	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader = MakeShared<FMemoryCacheLoader>();
	
	friend class UUBFInventoryItem;
	friend class FLoadMultipleAssetDatasAction;
	friend class UCollectionTestWidget;


};

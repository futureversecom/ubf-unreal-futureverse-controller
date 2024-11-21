// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FuturePassInventoryItem.h"
#include "UBFRuntimeController.h"
#include "ControllerLayers/APISubGraphResolver.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FutureverseUBFControllerSubsystem.generated.h"

class UCollectionRemappings;
class UCollectionAssetProfiles;
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
	UFUNCTION(BlueprintCallable)
	void RenderItem(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	// Used for rendering an item and other linked items using context tree
	UFUNCTION(BlueprintCallable)
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

private:
	FAssetProfile GetAssetProfile(const FString& AssetID) const;

	void ExecuteGraph(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
		IGraphProvider* GraphProvider, ISubGraphResolver* SubGraphResolver,
		const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete);
	
	void BuildContextTreeFromAssetTree(const TSharedPtr<FContextTree>& ContextTree, const FFutureverseAssetTreeData& AssetTree,
		const TMap<FString, UBF::FDynamicHandle>& RootTraits) const;
	
	TSharedPtr<FAPIGraphProvider> APIGraphProvider;
	TSharedPtr<FAPISubGraphResolver> APISubGraphProvider;
	
	TSharedPtr<FMemoryCacheLoader> MemoryCacheLoader;
	TSharedPtr<FTempCacheLoader> TempCacheLoader;

	TMap<FString, FAssetProfile> AssetProfiles;
	TMap<FString, UBF::FGraphHandle> ParsingGraphs;
	
	UBF::FExecutionContextHandle LastParsingGraphExecutionContextHandle;
	UBF::FGraphHandle LastParsedGraph;
	
	friend class UFuturePassInventoryItem;
};

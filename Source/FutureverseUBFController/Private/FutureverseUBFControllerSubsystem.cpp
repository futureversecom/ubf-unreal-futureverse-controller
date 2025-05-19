// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "FutureverseUBFControllerSubsystem.h"

#include "BlueprintUBFLibrary.h"
#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "UBFLogData.h"
#include "UBFUtils.h"
#include "CollectionData/CollectionAssetProfiles.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ExecutionSets/ExecutionSetData.h"
#include "ExecutionSets/ExecutionSetResult.h"
#include "GlobalArtifactProvider/GlobalArtifactProviderSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "LoadActions/LoadAssetCatalogAction.h"
#include "LoadActions/LoadAssetProfilesAction.h"
#include "LoadActions/LoadMultipleAssetDatasAction.h"

UFutureverseUBFControllerSubsystem::UFutureverseUBFControllerSubsystem()
{
	MemoryCacheLoader = MakeShared<FMemoryCacheLoader>();
}

void UFutureverseUBFControllerSubsystem::RenderItem(UUBFInventoryItem* Item, const FString& VariantID, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem was provided invalid Item. Cannot Render."));
		return;
	}
	
	if (!IsValid(Controller))
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem was provided invalid Controller. Cannot Render."));
		return;
	}
	
	RenderItemInternal(FUBFRenderDataContainer::GetFromData(Item->GetRenderData(), VariantID), Controller, InputMap, OnComplete);
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UUBFInventoryItem* Item, const FString& VariantID, 
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree was provided invalid Item. Cannot Render."));
		return;
	}
	
	if (!IsValid(Controller))
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree was provided invalid Controller. Cannot Render."));
		return;
	}
	
	RenderItemTreeInternal(FUBFRenderDataContainer::GetFromData(Item->GetRenderData(), VariantID), Controller, InputMap, OnComplete);
}

void UFutureverseUBFControllerSubsystem::RenderItemFromRenderData(const FUBFRenderData& RenderData, const FString& VariantID, 
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	RenderItemInternal(FUBFRenderDataContainer::GetFromData(RenderData, VariantID), Controller, InputMap, OnComplete);
}

void UFutureverseUBFControllerSubsystem::RenderItemTreeFromRenderData(const FUBFRenderData& RenderData, const FString& VariantID, 
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	RenderItemTreeInternal(FUBFRenderDataContainer::GetFromData(RenderData, VariantID), Controller, InputMap, OnComplete);
}

TFuture<TMap<FString, UUBFBindingObject*>> UFutureverseUBFControllerSubsystem::GetTraitsForItem(
	const FString& ParsingGraphId, UUBFRuntimeController* Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const
{
	TSharedPtr<TPromise<TMap<FString, UUBFBindingObject*>>> Promise = MakeShareable(new TPromise<TMap<FString, UUBFBindingObject*>>());
	TFuture<TMap<FString, UUBFBindingObject*>> Future = Promise->GetFuture();
	
	if (!IsSubsystemValid())
	{
		TMap<FString, UBF::FDynamicHandle> Traits;
		Promise->SetValue(UBFUtils::AsBindingObjectMap(Traits));
		return Future;
	}
	
	const auto OnParsingGraphComplete = [this, Promise](bool Success, const TSharedPtr<UBF::FExecutionSetResult>& Result)
	{
		// inject outputs of the parsing graph as the inputs of the graph to execute
		Promise->SetValue(UBFUtils::AsBindingObjectMap(Result->GetAllOutputs()));
	};

	UBF::FExecutionInstanceData ParsingBlueprintData(ParsingGraphId);
	ParsingBlueprintData.AddInputs(ParsingInputs);
	TSharedPtr<UBF::FExecutionSetData> ExecutionSetData = MakeShared<UBF::FExecutionSetData>(Controller->RootComponent, TArray{ParsingBlueprintData}, OnParsingGraphComplete);

	UBF::Execute(ParsingBlueprintData.GetInstanceId(), ExecutionSetData);

	return Future;
}

bool UFutureverseUBFControllerSubsystem::IsSubsystemValid() const
{
	return IsValid(this) && bIsInitialized;
}

void UFutureverseUBFControllerSubsystem::ParseInputsThenExecute(FUBFRenderDataPtr RenderData, UUBFRuntimeController* Controller,
                                                                const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete,
                                                                const bool bShouldBuildContextTree) const
{
	// get metadata json string from original json
	UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs Parsing Metadata: %s"), *RenderData->GetMetadataJson());
	TMap<FString, UBF::FDynamicHandle> ParsingInputs =
	{
		{TEXT("metadata"), UBF::FDynamicHandle::String(RenderData->GetMetadataJson()) }
	};
				
	const auto ParsingGraphId = AssetProfiles.Get(RenderData->GetAssetID()).GetParsingBlueprintId(RenderData->GetVariantID());
	GetTraitsForItem(ParsingGraphId, Controller, ParsingInputs).Next(
		[this, RenderData, bShouldBuildContextTree, InputMap, Controller, OnComplete]
		(const TMap<FString, UUBFBindingObject*>& Traits)
	{
		if (!IsSubsystemValid()) return;
			
		TMap<FString, UUBFBindingObject*> Inputs = InputMap;
		Inputs.Append(Traits);
		ExecuteGraph(RenderData, Controller, Inputs, bShouldBuildContextTree, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::ExecuteGraph(const FUBFRenderDataPtr& RenderData, UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, bool bShouldBuildContextTree, const FOnComplete& OnComplete) const
{
	FBlueprintExecutionData ExecutionData;

	FString RenderBlueprintId = AssetProfiles.Get(RenderData->GetAssetID()).GetRenderBlueprintId(RenderData->GetVariantID());

	if (bShouldBuildContextTree)
	{
		CreateBlueprintInstancesFromContextTree(RenderData, RenderData->GetContextTreeRef(), RenderData->GetAssetID(), ExecutionData.BlueprintInstances);
	}
	else
	{
		ExecutionData.BlueprintInstances.Add(UBF::FExecutionInstanceData(RenderBlueprintId));
	}
	
	ExecutionData.InputMap = InputMap;

	FString InstanceID;

	// input priorities in order (inputs, traits, blueprint variables)
	// Find traits from blueprint instance for root as it contains inputs for parent -> child graph relationships
	for (const UBF::FExecutionInstanceData& BlueprintInstance : ExecutionData.BlueprintInstances)
	{
		if (BlueprintInstance.GetBlueprintId() == RenderBlueprintId)
		{
			InstanceID = BlueprintInstance.GetInstanceId();
			break;
		}
	}

	if (InstanceID.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph no InstanceID found, cannot Execute"));
		return;
	}
					
	for (auto Input : ExecutionData.InputMap)
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph ResolvedInput %s"), *Input.Value->ToString());
	}
	
	if (!IsValid(Controller))
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller provided. Cannot render."));
		return;
	}
	
	Controller->ExecuteBlueprint(InstanceID, ExecutionData, OnComplete);
}

TFuture<bool> UFutureverseUBFControllerSubsystem::EnsureAssetDatasLoaded(
	const TArray<FFutureverseAssetLoadData>& LoadDatas)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetDatasAction> LoadMultipleAssetDatasAction = MakeShared<FLoadMultipleAssetDatasAction>();

	LoadMultipleAssetDatasAction->TryLoadMultipleAssetDatasAction(LoadDatas, this)
	.Next([this, Promise, LoadMultipleAssetDatasAction](bool bSuccess)
	{
		if (!IsSubsystemValid())
		{
			Promise->SetValue(false);
			return;
		}
		
		Promise->SetValue(bSuccess);
	});

	return Future;
}

TFuture<bool> UFutureverseUBFControllerSubsystem::EnsureAssetDataLoaded(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();
	
	EnsureAssetProfilesLoaded(LoadData).Next([LoadData, this, Promise](bool bResult)
	{
		if (!IsSubsystemValid() || !bResult)
		{
			Promise->SetValue(false);
			return;
		}
		
		EnsureCatalogsLoaded(LoadData).Next([Promise, this](bool bResult)
		{
			if (!IsSubsystemValid() || !bResult)
			{
				Promise->SetValue(false);
				return;
			}

			Promise->SetValue(true);
		});
	});

	return Future;
}

TFuture<bool> UFutureverseUBFControllerSubsystem::EnsureAssetProfilesLoaded(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	if (IsAssetProfileLoaded(LoadData))
	{
		Promise->SetValue(true);
		return Future;
	}

	TSharedPtr<FLoadAssetProfilesAction> LoadAssetProfilesAction = MakeShared<FLoadAssetProfilesAction>();

	LoadAssetProfilesAction->TryLoadAssetProfile(LoadData, MemoryCacheLoader)
	.Next([this, Promise, LoadAssetProfilesAction](bool bSuccess)
	{
		if (!IsSubsystemValid())
		{
			Promise->SetValue(false);
			return;
		}
		
		if (bSuccess)
		{
			for (const auto& Element : LoadAssetProfilesAction->AssetProfiles)
			{
				UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetProfile AssetId %s AssetProfile %s loaded."), *Element.Key, *Element.Value.ToString());
				RegisterAssetProfile(Element.Value);
			}
			
			Promise->SetValue(true);
		}
		else
		{
			Promise->SetValue(false);
		}
	});

	return Future;
}

TFuture<bool> UFutureverseUBFControllerSubsystem::EnsureCatalogsLoaded(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	if (IsCatalogLoaded(LoadData))
	{
		Promise->SetValue(true);
		return Future;
	}

	TSharedPtr<FLoadAssetCatalogAction> LoadAssetCatalogAction = MakeShared<FLoadAssetCatalogAction>();

	LoadAssetCatalogAction->TryLoadAssetCatalog(AssetProfiles.Get(LoadData.AssetID), LoadData, MemoryCacheLoader)
	.Next([this, Promise, LoadAssetCatalogAction](bool bSuccess)
	{
		if (!IsSubsystemValid())
		{
			Promise->SetValue(false);
			return;
		}
		
		if (bSuccess)
		{
			UGlobalArtifactProviderSubsystem::Get(this)->RegisterCatalogs(LoadAssetCatalogAction->RenderCatalogMap);
			UGlobalArtifactProviderSubsystem::Get(this)->RegisterCatalogs(LoadAssetCatalogAction->ParsingCatalogMap);
			LoadedVariantCatalogs.Add(LoadAssetCatalogAction->LoadData.GetCombinedVariantID());
			
			Promise->SetValue(true);
		}
		else
		{
			Promise->SetValue(false);
		}
	});
	
	return Future;
}

bool UFutureverseUBFControllerSubsystem::IsAssetProfileLoaded(const FFutureverseAssetLoadData& LoadData) const
{
	return AssetProfiles.Contains(LoadData.AssetID);
}

bool UFutureverseUBFControllerSubsystem::IsCatalogLoaded(const FFutureverseAssetLoadData& LoadData) const
{
	return LoadedVariantCatalogs.Contains(LoadData.GetCombinedVariantID());
}

void UFutureverseUBFControllerSubsystem::RegisterAssetProfilesFromData(
	const EEnvironment& Environment, UCollectionAssetProfiles* CollectionAssetProfiles)
{
	if (!CollectionAssetProfiles->AssetProfilesJsonMap.Contains(Environment)) return;
	
	const auto AssetProfilesJson = CollectionAssetProfiles->AssetProfilesJsonMap[Environment];
	RegisterAssetProfilesFromJson(AssetProfilesJson, CollectionAssetProfiles->BasePath);
	
	for (const FAssetProfileData& Data : CollectionAssetProfiles->AdditionalData)
	{
		RegisterAssetProfile(Data.CreateProfileFromData(CollectionAssetProfiles->BasePath));
	}
}

void UFutureverseUBFControllerSubsystem::ExecuteItemGraph(FUBFRenderDataPtr RenderData,
	UUBFRuntimeController* Controller, const bool bShouldBuildContextTree,
    const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete) const
{
	const auto& AssetProfile = AssetProfiles.Get(RenderData->GetAssetID());
		
	if (!AssetProfile.GetParsingBlueprintId(RenderData->GetVariantID()).IsEmpty())
	{
		ParseInputsThenExecute(RenderData, Controller, InputMap, OnComplete, bShouldBuildContextTree);
		return;
	}

	if (AssetProfile.GetRenderBlueprintId(RenderData->GetVariantID()).IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph Item %s provided invalid Rendering Graph Instance. Cannot render."), *RenderData->GetAssetID());
		OnComplete.ExecuteIfBound(false, FUBFExecutionReport::Failure());
		return;
	}
	
	ExecuteGraph(RenderData, Controller, InputMap, bShouldBuildContextTree, OnComplete);
}

void UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree(const FUBFRenderDataPtr& RenderData,
	const TArray<FUBFContextTreeData>& UBFContextTree, const FString& RootAssetId, TArray<UBF::FExecutionInstanceData>& OutBlueprintInstances) const
{
	if (UBFContextTree.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Can't build context tree beacuse UBFContextTree is empty."));
		
		auto ParentNodeRenderBlueprint = AssetProfiles.Get(RootAssetId).GetRenderBlueprintId(RenderData->GetVariantID());
		UBF::FExecutionInstanceData BlueprintInstance(ParentNodeRenderBlueprint);
		OutBlueprintInstances.Reset();
		OutBlueprintInstances.Add(BlueprintInstance);
		return;
	}

	TMap<FString, UBF::FExecutionInstanceData> AssetIdToInstanceMap;

	for (const auto& ContextTreeData : UBFContextTree)
	{
		if (!AssetProfiles.Contains(ContextTreeData.RootNodeID))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ContextTreeData.RootNodeID);
			continue;
		}
		
		auto ParentNodeRenderBlueprint = AssetProfiles.Get(ContextTreeData.RootNodeID).GetRenderBlueprintId(RenderData->GetVariantID());
		UBF::FExecutionInstanceData BlueprintInstance(ParentNodeRenderBlueprint);

		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Adding BlueprintInstance to mapping with Key: %s Value: %s.")
			, *ContextTreeData.RootNodeID, *BlueprintInstance.ToString());
		AssetIdToInstanceMap.Add(ContextTreeData.RootNodeID, BlueprintInstance);
	}

	for (const auto& ContextTreeData : UBFContextTree)
	{
		for (const auto& Relationship : ContextTreeData.Relationships)
		{
			if (!AssetProfiles.Contains(Relationship.ChildAssetID))
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *Relationship.ChildAssetID);
				continue;
			}

			if (!AssetIdToInstanceMap.Contains(Relationship.ChildAssetID))
			{
				if (!AssetProfiles.Contains(ContextTreeData.RootNodeID))
				{
					UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ContextTreeData.RootNodeID);
					continue;
				}

				auto ChildNodeRenderBlueprint = AssetProfiles.Get(Relationship.ChildAssetID).GetRenderBlueprintId(RenderData->GetVariantID());
				UBF::FExecutionInstanceData NewBlueprintInstance(ChildNodeRenderBlueprint);
				AssetIdToInstanceMap.Add(Relationship.ChildAssetID, NewBlueprintInstance);
			}

			FString RelationshipKey = Relationship.RelationshipID;

			//TODO remove temp solution because pb graphs are wrong
			RelationshipKey = RelationshipKey.Replace(TEXT("path:"), TEXT("")).Replace(TEXT("_accessory"), TEXT(""));
			
			AssetIdToInstanceMap[ContextTreeData.RootNodeID].AddInput(RelationshipKey, UBF::FDynamicHandle::String(AssetIdToInstanceMap[Relationship.ChildAssetID].GetInstanceId()));
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Added Child Node %s with Relationship: %s."), *Relationship.ChildAssetID, *RelationshipKey);
		}
	}

	AssetIdToInstanceMap.GenerateValueArray(OutBlueprintInstances);
}

void UFutureverseUBFControllerSubsystem::RegisterAssetProfilesFromJson(const FString& Json, const FString& BasePath)
{
	TArray<FAssetProfile> AssetProfileEntries;
	AssetProfileUtils::ParseAssetProfileJson(Json, AssetProfileEntries);

	for (FAssetProfile& AssetProfile : AssetProfileEntries)
	{
		AssetProfile.OverrideRelativePaths(BasePath);
		RegisterAssetProfile(AssetProfile);
	}
}

void UFutureverseUBFControllerSubsystem::RegisterAssetProfile(const FAssetProfile& AssetProfile)
{
	AssetProfiles.Add(AssetProfile.GetId(), AssetProfile);
}

void UFutureverseUBFControllerSubsystem::ClearAssetProfiles()
{
	AssetProfiles.Clear();
}

void UFutureverseUBFControllerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	bIsInitialized = false;
}

void UFutureverseUBFControllerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bIsInitialized = true;
}

void UFutureverseUBFControllerSubsystem::RenderItemInternal(FUBFRenderDataPtr RenderData,
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	FFutureverseAssetLoadData LoadData = FFutureverseAssetLoadData(RenderData->GetAssetID(), RenderData->GetContractID());
	LoadData.VariantID = RenderData->GetVariantID();
	
	EnsureAssetDataLoaded(LoadData).Next([this, RenderData, Controller, InputMap, OnComplete](const bool bIsAssetProfileLoaded)
	{
		if (!IsSubsystemValid()) return;
			
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid AssetProfile. Cannot render."), *RenderData->GetAssetID());
			OnComplete.ExecuteIfBound(false, FUBFExecutionReport::Failure());
			return;
		}
			
		if (!AssetProfiles.Contains(RenderData->GetAssetID()))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem AssetProfilesMap does not contian Item %s. Cannot render."), *RenderData->GetAssetID());
			OnComplete.ExecuteIfBound(false, FUBFExecutionReport::Failure());
			return;
		}
			
		ExecuteItemGraph(RenderData, Controller, false, InputMap, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTreeInternal(FUBFRenderDataPtr RenderData,
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TArray<FFutureverseAssetLoadData> AssetLoadDatas = RenderData->GetLinkedAssetLoadData();

	for (FFutureverseAssetLoadData& AssetLoadData : AssetLoadDatas)
	{
		AssetLoadData.VariantID = RenderData->GetVariantID();
	}

	if (AssetLoadDatas.IsEmpty())
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree AssetLoadDatas empty for Item %s."), *RenderData->GetAssetID());
	
	EnsureAssetDatasLoaded(AssetLoadDatas).Next([this, RenderData, Controller, InputMap, OnComplete](const bool bIsAssetProfileLoaded)
	{
		if (!IsSubsystemValid()) return;
		
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s asset tree failed to load one or many AssetDatas. This will cause asset tree to not render fully"), *RenderData->GetAssetID());
		}
		
		if (!AssetProfiles.Contains(RenderData->GetAssetID()))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree AssetDataMap does not contian Item %s. Cannot render."), *RenderData->GetAssetID());
			OnComplete.ExecuteIfBound(false, FUBFExecutionReport::Failure());
			return;
		}
		
		ExecuteItemGraph(RenderData, Controller, true, InputMap, OnComplete);
	});
}

UFutureverseUBFControllerSubsystem* UFutureverseUBFControllerSubsystem::Get(const UObject* WorldContext)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GameInstance->GetSubsystem<UFutureverseUBFControllerSubsystem>();
	}

	return nullptr;
}
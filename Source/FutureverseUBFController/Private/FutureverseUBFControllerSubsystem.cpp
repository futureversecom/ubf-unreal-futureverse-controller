// Fill out your copyright notice in the Description page of Project Settings.


#include "FutureverseUBFControllerSubsystem.h"

#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "CollectionData/CollectionAssetProfiles.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"
#include "Kismet/GameplayStatics.h"
#include "LoadActions/LoadAssetProfileDataAction.h"
#include "LoadActions/LoadAssetProfilesAction.h"
#include "LoadActions/LoadMultipleAssetDatasAction.h"

UFutureverseUBFControllerSubsystem::UFutureverseUBFControllerSubsystem()
{
	MemoryCacheLoader = MakeShared<FMemoryCacheLoader>();
	TempCacheLoader = MakeShared<FTempCacheLoader>();
	APIGraphProvider = MakeShared<FAPIGraphProvider>(MemoryCacheLoader, TempCacheLoader);
}

void UFutureverseUBFControllerSubsystem::RenderItem(UUBFInventoryItem* Item, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TryLoadAssetData(FFutureverseAssetLoadData(Item->GetAssetID(), Item->GetContractID())).Next([this, Item, Controller, InputMap, OnComplete](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid AssetProfile. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		if (!AssetDataMap.Contains(Item->GetAssetID()))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem AssetDataMap does not contian Item %s. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		ExecuteItemGraph(Item, Controller, false, InputMap, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UUBFInventoryItem* Item,
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TArray<FFutureverseAssetLoadData> AssetLoadDatas = Item->GetLinkedAssetLoadData();

	if (AssetLoadDatas.IsEmpty())
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree AssetLoadDatas empty for Item %s."), *Item->GetAssetID());
	
	TryLoadAssetDatas(AssetLoadDatas).Next([this, Item, Controller, InputMap, OnComplete](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s asset tree failed to load one or many AssetDatas. This will cause asset tree to not render fully"), *Item->GetAssetID());
		}
		
		if (!AssetDataMap.Contains(Item->GetAssetID()))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree AssetDataMap does not contian Item %s. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		ExecuteItemGraph(Item, Controller, true, InputMap, OnComplete);
	});
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetProfile(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadAssetProfilesAction> LoadAssetProfilesAction = MakeShared<FLoadAssetProfilesAction>();
	PendingActions.Add(LoadAssetProfilesAction);

	LoadAssetProfilesAction->TryLoadAssetProfile(LoadData, MemoryCacheLoader, TempCacheLoader)
	.Next([this, Promise, LoadAssetProfilesAction](bool bSuccess)
	{
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

		PendingActions.Remove(LoadAssetProfilesAction);
	});

	return Future;
}


TFuture<TMap<FString, UUBFBindingObject*>> UFutureverseUBFControllerSubsystem::GetTraitsForItem(
	const FString& ParsingGraphId, UUBFRuntimeController* Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const
{
	TSharedPtr<TPromise<TMap<FString, UUBFBindingObject*>>> Promise = MakeShareable(new TPromise<TMap<FString, UUBFBindingObject*>>());
	TFuture<TMap<FString, UUBFBindingObject*>> Future = Promise->GetFuture();
	
	APIGraphProvider->GetGraph(ParsingGraphId)
		.Next([this, ParsingInputs, ParsingGraphId, Controller, Promise]
		(const UBF::FLoadGraphResult& Result)
	{
		if (!Result.Result.Key)
		{
			UE_LOG(LogUBF, Error, TEXT("Aborting execution: graph '%s' is invalid"), *ParsingGraphId);
			TMap<FString, UBF::FDynamicHandle> Traits;
			Promise->SetValue(UBFUtils::AsBindingObjectMap(Traits));
			return;
		}

		UBF::FGraphHandle ParsedGraph = Result.Result.Value;
		FString Id = FGuid::NewGuid().ToString();
		PendingParsingGraphContexts.Add(Id, UBF::FExecutionContextHandle());
			
		const auto OnParsingGraphComplete = [this, Promise, ParsedGraph, Id]
		{
			// inject outputs of the parsing graph as the inputs of the graph to execute
			TArray<UBF::FBindingInfo> Outputs;
			ParsedGraph.GetOutputs(Outputs);
				
			TMap<FString, UBF::FDynamicHandle> Traits;
			for (auto Output : Outputs)
			{
				UBF::FDynamicHandle DynamicOutput;
				if (PendingParsingGraphContexts[Id].TryReadOutput(Output.Id, DynamicOutput))
				{
					Traits.Add(Output.Id, DynamicOutput);
				}
			}
			PendingParsingGraphContexts.Remove(Id);
			Promise->SetValue(UBFUtils::AsBindingObjectMap(Traits));
		};
			
		ParsedGraph.Execute(ParsingGraphId, Controller->RootComponent, APIGraphProvider, TMap<FString, UBF::FBlueprintInstance>(),
			ParsingInputs, OnParsingGraphComplete, PendingParsingGraphContexts[Id]);
	});

	return Future;
}

void UFutureverseUBFControllerSubsystem::ParseInputsThenExecute(UUBFInventoryItem* Item, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete,
	const bool bShouldBuildContextTree)
{
	const auto AssetData = AssetDataMap.Get(Item->GetAssetID());
	
	// get metadata json string from original json
	UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs Parsing Metadata: %s"), *Item->GetMetadataJson());
	TMap<FString, UBF::FDynamicHandle> ParsingInputs =
	{
		{TEXT("metadata"), UBF::FDynamicHandle::String(Item->GetMetadataJson()) }
	};
				
	const auto ParsingGraphId = AssetDataMap.Get(Item->GetAssetID()).ParsingGraphInstance.GetId();
	GetTraitsForItem(ParsingGraphId, Controller, ParsingInputs).Next(
		[this, Item, bShouldBuildContextTree, InputMap, Controller, OnComplete]
		(const TMap<FString, UUBFBindingObject*>& Traits)
	{
			TMap<FString, UUBFBindingObject*> Inputs = InputMap;
			Inputs.Append(Traits);
			ExecuteGraph(Item, Controller, Inputs, bShouldBuildContextTree, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::ExecuteGraph(UUBFInventoryItem* Item, UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, bool bShouldBuildContextTree, const FOnComplete& OnComplete)
{
	FBlueprintExecutionData ExecutionData;

	if (bShouldBuildContextTree)
	{
		CreateBlueprintInstancesFromContextTree(Item->GetContextTreeRef(), Item->GetAssetID(), UBFUtils::AsDynamicMap(InputMap), ExecutionData.BlueprintInstances);
	}
	else
	{
		ExecutionData.InputMap = InputMap;
	}

	// input priorities in order (inputs, traits, blueprint variables)
	// Find traits from blueprint instance for root as it contains inputs for parent -> child graph relationships
	for (const UBF::FBlueprintInstance& BlueprintInstance : ExecutionData.BlueprintInstances)
	{
		if (BlueprintInstance.GetBlueprintId() == AssetDataMap.Get(Item->GetAssetID()).RenderGraphInstance.GetId())
		{
			for (const auto& BindingObjectMapTuple : UBFUtils::AsBindingObjectMap(BlueprintInstance.GetInputs()))
			{
				ExecutionData.InputMap.Add(BindingObjectMapTuple.Key, BindingObjectMapTuple.Value);
			}
				
			break;
		}
	}
					
	for (auto Input : ExecutionData.InputMap)
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs ResolvedInput %s"), *Input.Value->ToString());
	}
	
	if (!Controller)
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller provided. Cannot render."));
		return;
	}
			
	Controller->SetGraphProviders(APIGraphProvider);
	Controller->ExecuteBlueprint(AssetDataMap.Get(Item->GetAssetID()).RenderGraphInstance.GetId(), ExecutionData, OnComplete);
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetDatas(const TArray<FFutureverseAssetLoadData>& LoadDatas)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetDatasAction> LoadAssetProfilesAction = MakeShared<FLoadMultipleAssetDatasAction>();
	PendingMultiLoadActions.Add(LoadAssetProfilesAction);

	LoadAssetProfilesAction->TryLoadAssetProfiles(LoadDatas, this)
	.Next([this, Promise, LoadAssetProfilesAction](bool bSuccess)
	{
		Promise->SetValue(bSuccess);

		PendingMultiLoadActions.Remove(LoadAssetProfilesAction);
	});

	return Future;
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetData(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	// Check AssetData already exists
	if (AssetDataMap.Contains(LoadData.AssetID))
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetData Asset Data already exists for AssetId %s"), *LoadData.AssetID);
		Promise->SetValue(true);
		return Future;
	}
	
	// Check AssetProfile already exists
	if (AssetProfiles.Contains(LoadData.AssetID))
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetData AssetProfile already exists for AssetId %s"), *LoadData.AssetID);
		TryLoadAssetProfileData(LoadData.AssetID).Next([this, Promise, LoadData](bool bResult)
		{
			if (!bResult|| !AssetDataMap.Contains(LoadData.AssetID))
			{
				FAssetProfile AssetProfile = AssetProfiles.Get(LoadData.AssetID);
 				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetData Failed to load asset data for AssetId %s from asset profile %s"), *LoadData.AssetID, *AssetProfile.ToString());
				Promise->SetValue(false);
				return;
			}
	
			TryLoadAssetData(LoadData).Next([Promise](bool bResult)
			{
				Promise->SetValue(bResult);
			});
		});
		return Future;
	}
	
	UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetData No AssetProfile exists for AssetId %s Attempting to load..."), *LoadData.AssetID);
	TryLoadAssetProfile(LoadData).Next([this, Promise, LoadData](bool bResult)
	{
		if (!bResult || !AssetProfiles.Contains(LoadData.AssetID))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetData Failed to load asset profile for AssetId %s. Result %d ProfileExists %d"), *LoadData.AssetID, bResult, AssetProfiles.Contains(LoadData.AssetID));
			Promise->SetValue(false);
			return;
		}
		
		TryLoadAssetData(LoadData).Next([Promise](bool bResult)
		{
			Promise->SetValue(bResult);
		});
	});

	return Future;
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetProfileData(const FString& AssetID)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	if (!AssetProfiles.Contains(AssetID))
	{
		Promise->SetValue(false);
		return Future;
	}

	FAssetProfile AssetProfile = AssetProfiles.Get(AssetID);
	
	TSharedPtr<FLoadAssetProfileDataAction> AssetProfileDataAction = MakeShared<FLoadAssetProfileDataAction>();
	PendingDataActions.Add(AssetProfileDataAction);

	AssetProfileDataAction->TryLoadAssetProfileData(AssetProfile, MemoryCacheLoader, TempCacheLoader).Next([this, AssetProfileDataAction, Promise](bool bSuccess)
	{
		if (bSuccess)
		{
			APIGraphProvider->RegisterBlueprintJson(AssetProfileDataAction->AssetData.RenderGraphInstance);

			if (AssetProfileDataAction->AssetData.ParsingGraphInstance.IsValid())
				APIGraphProvider->RegisterBlueprintJson(AssetProfileDataAction->AssetData.ParsingGraphInstance);

			APIGraphProvider->RegisterCatalogs(AssetProfileDataAction->RenderCatalogMap);
			
			if (AssetProfileDataAction->AssetData.ParsingGraphInstance.IsValid())
				APIGraphProvider->RegisterCatalogs(AssetProfileDataAction->ParsingCatalogMap);
			
			RegisterAssetData(AssetProfileDataAction->AssetProfileLoaded.Id, AssetProfileDataAction->AssetData);
			
			Promise->SetValue(true);
		}
		else
		{
			Promise->SetValue(false);
		}

		PendingDataActions.Remove(AssetProfileDataAction);
	});

	return Future;
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

void UFutureverseUBFControllerSubsystem::ExecuteItemGraph(UUBFInventoryItem* Item,
	UUBFRuntimeController* Controller, const bool bShouldBuildContextTree,
    const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	const auto& AssetData = AssetDataMap.Get(Item->GetAssetID());
		
	if (AssetData.ParsingGraphInstance.IsValid())
	{
		ParseInputsThenExecute(Item, Controller, InputMap, OnComplete, bShouldBuildContextTree);
		return;
	}

	if (!AssetData.RenderGraphInstance.IsValid())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph Item %s provided invalid Rendering Graph Instance. Cannot render."), *Item->GetAssetID());
		return;
	}
	
	ExecuteGraph(Item, Controller, InputMap, bShouldBuildContextTree, OnComplete);
}

void UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree(
	const TArray<FUBFContextTreeData>& UBFContextTree, const FString& RootAssetId,
	const TMap<FString, UBF::FDynamicHandle>& RootTraits, TArray<UBF::FBlueprintInstance>& OutBlueprintInstances) const
{
	if (UBFContextTree.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Can't build context tree beacuse UBFContextTree is empty."));
		
		auto ParentNodeRenderGraphInstance = AssetDataMap.Get(RootAssetId).RenderGraphInstance;
		UBF::FBlueprintInstance BlueprintInstance(ParentNodeRenderGraphInstance.GetId());
		BlueprintInstance.AddInputs(RootTraits);
		OutBlueprintInstances.Reset();
		OutBlueprintInstances.Add(BlueprintInstance);
		return;
	}

	TMap<FString, UBF::FBlueprintInstance> AssetIdToInstanceMap;

	for (const auto& ContextTreeData : UBFContextTree)
	{
		if (!AssetDataMap.Contains(ContextTreeData.RootNodeID))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ContextTreeData.RootNodeID);
			continue;
		}
		
		auto ParentNodeRenderGraphInstance = AssetDataMap.Get(ContextTreeData.RootNodeID).RenderGraphInstance;
		UBF::FBlueprintInstance BlueprintInstance(ParentNodeRenderGraphInstance.GetId());
		
		if (ContextTreeData.RootNodeID == RootAssetId)
		{
			BlueprintInstance.AddInputs(RootTraits);
		}

		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Adding BlueprintInstance to mapping with Key: %s Value: %s.")
			, *ContextTreeData.RootNodeID, *BlueprintInstance.ToString());
		AssetIdToInstanceMap.Add(ContextTreeData.RootNodeID, BlueprintInstance);
	}

	for (const auto& ContextTreeData : UBFContextTree)
	{
		for (const auto& ChildItem : ContextTreeData.Children)
		{
			if (!AssetDataMap.Contains(ChildItem.Value))
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ChildItem.Value);
				continue;
			}

			if (!AssetIdToInstanceMap.Contains(ChildItem.Value))
			{
				if (!AssetDataMap.Contains(ContextTreeData.RootNodeID))
				{
					UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ContextTreeData.RootNodeID);
					continue;
				}

				auto ChildNodeRenderGraph = AssetDataMap.Get(ChildItem.Value).RenderGraphInstance;
				UBF::FBlueprintInstance NewBlueprintInstance(ChildNodeRenderGraph.GetId());
				AssetIdToInstanceMap.Add(ChildItem.Value, NewBlueprintInstance);
			}

			FString RelationshipKey = ChildItem.Key;

			//TODO remove temp solution because pb graphs are wrong
			RelationshipKey = RelationshipKey.Replace(TEXT("path:"), TEXT("")).Replace(TEXT("_accessory"), TEXT(""));
			
			AssetIdToInstanceMap[ContextTreeData.RootNodeID].AddInput(RelationshipKey, UBF::FDynamicHandle::String(AssetIdToInstanceMap[ChildItem.Value].GetInstanceId()));
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Added Child Node %s with Relationship: %s."), *ChildItem.Value, *RelationshipKey);
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
		AssetProfile.RelativePath = BasePath;
		RegisterAssetProfile(AssetProfile);
	}
}

void UFutureverseUBFControllerSubsystem::RegisterAssetProfile(const FAssetProfile& AssetProfile)
{
	AssetProfiles.Add(AssetProfile.Id, AssetProfile);
}

void UFutureverseUBFControllerSubsystem::ClearAssetProfiles()
{
	AssetProfiles.Clear();
}

void UFutureverseUBFControllerSubsystem::RegisterAssetData(const FString& AssetId, const FFutureverseAssetData& AssetData)
{
	if (AssetDataMap.Contains(AssetId))
	{
		AssetDataMap.Remove(AssetId);
	}
	
	AssetDataMap.Add(AssetId, AssetData);

	UE_LOG(LogFutureverseUBFController, VeryVerbose,
		TEXT("UFutureverseUBFControllerSubsystem::RegisterAssetData Registered AssetData for Item %s RenderingGraphBlueprintId: %s ParsingGraphBlueprintId: %s"),
		*AssetId, *AssetData.RenderGraphInstance.GetId(), *AssetData.ParsingGraphInstance.GetId());
}

UFutureverseUBFControllerSubsystem* UFutureverseUBFControllerSubsystem::Get(const UObject* WorldContext)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GameInstance->GetSubsystem<UFutureverseUBFControllerSubsystem>();
	}

	return nullptr;
}

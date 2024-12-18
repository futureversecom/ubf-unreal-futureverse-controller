// Fill out your copyright notice in the Description page of Project Settings.


#include "FutureverseUBFControllerSubsystem.h"

#include "EmergenceSingleton.h"
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

void UFutureverseUBFControllerSubsystem::RenderItem(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FContextTree> ContextTree = MakeShared<FContextTree>();
	APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
	
	TryLoadAssetData(FFutureverseAssetLoadData(Item->GetAssetID(), Item->GetInventoryItem().contract)).Next([this, Item, Controller, InputMap, OnComplete, ContextTree](const bool bIsAssetProfileLoaded)
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
		
		ExecuteGraph(Item, ContextTree, Controller, false, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UFuturePassInventoryItem* Item,
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FContextTree> ContextTree = MakeShared<FContextTree>();
	APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
	
	TryLoadAssetDatas(Item->GetAssetTreeRef().GetLinkedAssetLoadData()).Next([this, Item, Controller, InputMap, OnComplete, ContextTree](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s asset tree failed to load one or many AssetDatas. This will cause asset tree to not render fully"), *Item->GetAssetID());
		}
		
		if (!AssetDataMap.Contains(Item->GetAssetID()))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem AssetDataMap does not contian Item %s. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		ExecuteGraph(Item, ContextTree, Controller, true, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	});
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetProfile(const FString& ContractId)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadAssetProfilesAction> LoadAssetProfilesAction = MakeShared<FLoadAssetProfilesAction>();
	PendingActions.Add(LoadAssetProfilesAction);

	LoadAssetProfilesAction->TryLoadAssetProfile(ContractId, MemoryCacheLoader, TempCacheLoader)
	.Next([this, Promise, LoadAssetProfilesAction](bool bSuccess)
	{
		if (bSuccess)
		{
			for (const auto& Element : LoadAssetProfilesAction->AssetProfiles)
			{
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
	
	const auto OnParsingGraphComplete = [this, Promise]
	{
		// inject outputs of the parsing graph as the inputs of the graph to execute
		TArray<UBF::FBindingInfo> Outputs;
		LastParsedGraph.GetOutputs(Outputs);
		
		TMap<FString, UBF::FDynamicHandle> Traits;
		for (auto Output : Outputs)
		{
			UBF::FDynamicHandle DynamicOutput;
			if (LastParsingGraphExecutionContextHandle.TryReadOutput(Output.Id, DynamicOutput))
			{
				Traits.Add(Output.Id, DynamicOutput);
			}
		}
		Promise->SetValue(UBFUtils::AsBindingObjectMap(Traits));
	};
	
	APIGraphProvider->GetGraph(ParsingGraphId)
		.Next([this, ParsingInputs, ParsingGraphId, OnParsingGraphComplete, Controller, Promise]
		(const UBF::FLoadGraphResult& Result)
	{
		if (!Result.Result.Key)
		{
			UE_LOG(LogUBF, Error, TEXT("Aborting execution: graph '%s' is invalid"), *ParsingGraphId);
			TMap<FString, UBF::FDynamicHandle> Traits;
			Promise->SetValue(UBFUtils::AsBindingObjectMap(Traits));
			return;
		}

		LastParsedGraph = Result.Result.Value;
		LastParsedGraph.Execute(ParsingGraphId, Controller->RootComponent, APIGraphProvider.Get(), APISubGraphProvider.Get(),
			ParsingInputs, OnParsingGraphComplete, LastParsingGraphExecutionContextHandle);
	});

	return Future;
}

void UFutureverseUBFControllerSubsystem::ParseInputsThenExecute(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete, TSharedPtr<FContextTree> ContextTree,
	const bool bShouldBuildContextTree)
{
	const auto AssetData = AssetDataMap[Item->GetAssetID()];
	
	// get metadata json string from original json
	FString MetadataJson;
	TSharedPtr<FJsonObject> MetadataObjectField = Item->GetInventoryItem().OriginalData.JsonObject
		->GetObjectField(TEXT("node"))->GetObjectField(TEXT("metadata"));
				
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MetadataJson);
	FJsonSerializer::Serialize(MetadataObjectField.ToSharedRef(), Writer);
			
	UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs Parsing Metadata: %s"), *MetadataJson);
	TMap<FString, UBF::FDynamicHandle> ParsingInputs =
	{
		{TEXT("metadata"), UBF::FDynamicHandle::String(MetadataJson) }
	};
				
	const auto ParsingGraphId = AssetDataMap[Item->GetAssetID()].ParsingGraphInstance.GetId();
	GetTraitsForItem(ParsingGraphId, Controller, ParsingInputs).Next(
		[this, Item, ContextTree, bShouldBuildContextTree, InputMap, Controller, AssetData, OnComplete]
		(const TMap<FString, UUBFBindingObject*>& Traits)
	{
		if (bShouldBuildContextTree)
		{
			auto RootRenderGraphInstanceId = AssetData.RenderGraphInstance.GetId();
			const auto RootNode = ContextTree->AddItem(RootRenderGraphInstanceId);
			RootNode->AddTraits(AssetData.RenderGraphInstance.GetVariables());
			ContextTree->SetRoot(RootNode);
			RootNode->AddTraits(UBFUtils::AsDynamicMap(Traits));
			BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), RootNode);
		}

		// input priorities in order (inputs, traits, blueprint variables)
		TMap<FString, UUBFBindingObject*> ResolvedInputs = UBFUtils::AsBindingObjectMap(AssetData.RenderGraphInstance.GetVariables());;
		ResolvedInputs.Append(Traits);
		ResolvedInputs.Append(InputMap);
					
		for (auto Input : ResolvedInputs)
		{
			UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs ResolvedInput %s"), *Input.Value->ToString());
		}
	
		if (!Controller)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller provided. Cannot render."));
			return;
		}
			
		Controller->SetGraphProviders(APIGraphProvider.Get(), APISubGraphProvider.Get());
		Controller->ExecuteGraph(AssetData.RenderGraphInstance.GetId(), ResolvedInputs, OnComplete);
	});
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
		Promise->SetValue(true);
		return Future;
	}
	
	// Check AssetProfile already exists
	if (AssetProfiles.Contains(LoadData.AssetID))
	{
		TryLoadAssetProfileData(LoadData.AssetID).Next([this, Promise, LoadData](bool bResult)
		{
			if (!bResult|| !AssetDataMap.Contains(LoadData.AssetID))
			{
				FAssetProfile AssetProfile = AssetProfiles.Contains(LoadData.AssetID) ? AssetProfiles[LoadData.AssetID] : FAssetProfile();
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
	
	TryLoadAssetProfile(LoadData.ContractID).Next([this, Promise, LoadData](bool bResult)
	{
		if (!bResult || !AssetProfiles.Contains(LoadData.AssetID))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::TryLoadAssetData Failed to load asset profile for AssetId %s"), *LoadData.AssetID);
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

	FAssetProfile AssetProfile = AssetProfiles[AssetID];
	
	TSharedPtr<FLoadAssetProfileDataAction> AssetProfileDataAction = MakeShared<FLoadAssetProfileDataAction>();
	PendingDataActions.Add(AssetProfileDataAction);

	AssetProfileDataAction->TryLoadAssetProfileData(AssetProfile, MemoryCacheLoader, TempCacheLoader).Next([this, AssetProfileDataAction, Promise](bool bSuccess)
	{
		if (bSuccess)
		{
			APIGraphProvider->RegisterBlueprintInstance(AssetProfileDataAction->AssetData.RenderGraphInstance);

			if (AssetProfileDataAction->AssetData.ParsingGraphInstance.IsValid())
				APIGraphProvider->RegisterBlueprintInstance(AssetProfileDataAction->AssetData.ParsingGraphInstance);

			APIGraphProvider->RegisterCatalogs(AssetProfileDataAction->AssetData.RenderGraphInstance.GetId(), AssetProfileDataAction->RenderCatalogMap);
			
			if (AssetProfileDataAction->AssetData.ParsingGraphInstance.IsValid())
				APIGraphProvider->RegisterCatalogs(AssetProfileDataAction->AssetData.ParsingGraphInstance.GetId(), AssetProfileDataAction->ParsingCatalogMap);
			
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
	UCollectionAssetProfiles* CollectionAssetProfiles)
{
	if (!UEmergenceSingleton::GetEmergenceManager(this)) return;
	
	const auto Environment = UEmergenceSingleton::GetEmergenceManager(this)->GetFutureverseEnvironment();
	
	if (!CollectionAssetProfiles->AssetProfilesJsonMap.Contains(Environment)) return;
	
	const auto AssetProfilesJson = CollectionAssetProfiles->AssetProfilesJsonMap[Environment];
	RegisterAssetProfilesFromJson(AssetProfilesJson, CollectionAssetProfiles->BasePath);
	
	for (const FAssetProfileData& Data : CollectionAssetProfiles->AdditionalData)
	{
		RegisterAssetProfile(Data.CreateProfileFromData(CollectionAssetProfiles->BasePath));
	}
}

void UFutureverseUBFControllerSubsystem::ExecuteGraph(UFuturePassInventoryItem* Item, const TSharedPtr<FContextTree>& ContextTree,
	UUBFRuntimeController* Controller, const bool bShouldBuildContextTree, IGraphProvider* GraphProvider, ISubGraphResolver* SubGraphResolver,
    const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	const auto AssetData = AssetDataMap[Item->GetAssetID()];
		
	if (AssetData.ParsingGraphInstance.IsValid())
	{
		ParseInputsThenExecute(Item, Controller, InputMap, OnComplete, ContextTree, bShouldBuildContextTree);
		return;
	}

	if (!AssetData.RenderGraphInstance.IsValid())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid Rendering Graph Instance. Cannot render."), *Item->GetAssetID());
		return;
	}
	
	if (!AssetData.ParsingGraphInstance.IsValid() && bShouldBuildContextTree)
	{
		// no traits from parsing graph but still build the tree
		auto RootRenderGraphInstanceId = AssetData.RenderGraphInstance.GetId();
		const auto RootNode = ContextTree->AddItem(RootRenderGraphInstanceId);
		RootNode->AddTraits(AssetData.RenderGraphInstance.GetVariables());
		ContextTree->SetRoot(RootNode);
		
		BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), RootNode);
	}
	
	// input priorities in order (inputs, traits, blueprint variables)
	auto ResolvedInputs = UBFUtils::AsBindingObjectMap(AssetData.RenderGraphInstance.GetVariables());
	ResolvedInputs.Append(InputMap);
	
	if (!Controller)
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller provided. Cannot render."));
		return;
	}
	
	Controller->SetGraphProviders(GraphProvider, SubGraphResolver);
	Controller->ExecuteGraph(AssetData.RenderGraphInstance.GetId(), ResolvedInputs, OnComplete);
}

void UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree(const TSharedPtr<FContextTree>& ContextTree,
	const FFutureverseAssetTreeData& AssetTree, const TSharedPtr<FContextData>& RootNode) const
{
	const auto ItemAssetTree = AssetTree.TreePaths;
	if (ItemAssetTree.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree Can't build context tree beacuse ItemAssetTree is empty."));
		return;
	}
	
	for (int i = 0; i < ItemAssetTree.Num(); ++i)
	{
		if (ItemAssetTree[i].Objects.IsEmpty()) continue;

		if (!AssetTree.LinkedItems.Contains(ItemAssetTree[i].Id))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree AssetTree.LinkedItems does not contain %s."), *ItemAssetTree[i].Id);
			continue;
		}
		if (!AssetDataMap.Contains(AssetTree.LinkedItems[ItemAssetTree[i].Id]))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree AssetDataMap does not contain %s."), *AssetTree.LinkedItems[ItemAssetTree[i].Id]);
			continue;
		}
		
		for (const auto AssetTreeObject : ItemAssetTree[i].Objects)
		{
			if (!AssetTreeObject.Key.Contains(TEXT("path:"))) continue;
			
			if (!AssetTree.LinkedItems.Contains(AssetTreeObject.Value.Id))
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree AssetTree.LinkedItems does not contain %s."), *ItemAssetTree[i].Id);
				continue;
			}

			if (!AssetDataMap.Contains(AssetTree.LinkedItems[AssetTreeObject.Value.Id]))
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree AssetDataMap does not contain %s."), *AssetTree.LinkedItems[AssetTreeObject.Value.Id]);
				continue;
			}
			auto NodeRenderGraphInstanceId = AssetDataMap[AssetTree.LinkedItems[AssetTreeObject.Value.Id]].RenderGraphInstance.GetId();
			const auto ChildNode = ContextTree->AddItem(NodeRenderGraphInstanceId);
			ChildNode->AddTraits( AssetDataMap[AssetTree.LinkedItems[AssetTreeObject.Value.Id]].RenderGraphInstance.GetVariables());
			
			FString Relationship = AssetTreeObject.Key;
			RootNode->AddChild(ChildNode, Relationship);
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree Added Child Node %s with Relationship: %s."), *AssetTree.LinkedItems[AssetTreeObject.Value.Id], *Relationship);
		}
	}
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
	AssetProfiles.Reset();
}

void UFutureverseUBFControllerSubsystem::RegisterAssetData(const FString& AssetId, const FFutureverseAssetData& AssetData)
{
	if (AssetDataMap.Contains(AssetId))
	{
		AssetDataMap[AssetId] = AssetData;
	}
	else
	{
		AssetDataMap.Add(AssetId, AssetData);
	}

	UE_LOG(LogFutureverseUBFController, VeryVerbose,
		TEXT("UFutureverseUBFControllerSubsystem::RegisterAssetData Registered AssetData for Item %s RenderingGraphBlueprintId: %s ParsingGraphBlueprintId: %s"),
		*AssetId, *AssetData.RenderGraphInstance.GetBlueprintId(), *AssetData.ParsingGraphInstance.GetBlueprintId());
}

UFutureverseUBFControllerSubsystem* UFutureverseUBFControllerSubsystem::Get(const UObject* WorldContext)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GameInstance->GetSubsystem<UFutureverseUBFControllerSubsystem>();
	}

	return nullptr;
}

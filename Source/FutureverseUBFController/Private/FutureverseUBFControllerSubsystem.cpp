// Fill out your copyright notice in the Description page of Project Settings.


#include "FutureverseUBFControllerSubsystem.h"

#include "EmergenceSingleton.h"
#include "FutureverseUBFControllerLog.h"
#include "LoadAssetProfilesAction.h"
#include "CollectionData/CollectionAssetProfiles.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"
#include "Kismet/GameplayStatics.h"

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

	if (AssetDataMap.Contains(Item->GetAssetID()))
	{
		const auto AssetData = AssetDataMap[Item->GetAssetID()];
		
		if (AssetData.ParsingGraphInstance.IsValid())
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, false);
			return;
		}

		if (!AssetData.RenderGraphInstance.IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid Rendering Graph Instance. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item->GetAssetID(), Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
		return;
	}

	TryLoadAssetProfile(Item->GetAssetID(), Item->GetInventoryItem().contract).Next([this, Item, Controller, InputMap, OnComplete, ContextTree](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid AssetProfile. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		const auto AssetData = AssetDataMap[Item->GetAssetID()];
		if (AssetData.ParsingGraphInstance.IsValid())
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, false);
			return;
		}

		if (!AssetData.RenderGraphInstance.IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid Rendering Graph Instance. Cannot render."), *Item->GetAssetID());
			return;
		}
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item->GetAssetID(), Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UFuturePassInventoryItem* Item,
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FContextTree> ContextTree = MakeShared<FContextTree>();
	if (AssetDataMap.Contains(Item->GetAssetID()))
	{
		const auto AssetData = AssetDataMap[Item->GetAssetID()];
		
		if (AssetData.ParsingGraphInstance.IsValid())
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, true);
			return;
		}

		// no traits because this item doesn't have a parsing graph but still build the tree
		const TMap<FString, UBF::FDynamicHandle> Traits;
		BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), "", Traits);
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item->GetAssetID(), Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
		return;
	}
	
	TryLoadAssetProfile(Item->GetAssetID(), Item->GetInventoryItem().contract).Next([this, Item, Controller, InputMap, OnComplete, ContextTree](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s provided invalid AssetProfile. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		const auto AssetData = AssetDataMap[Item->GetAssetID()];
		
		if (AssetData.ParsingGraphInstance.IsValid())
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, true);
			return;
		}

		if (!AssetData.RenderGraphInstance.IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s provided invalid Rendering Graph Instance. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		// no traits because this item doesn't have a parsing graph but still build the tree
		const TMap<FString, UBF::FDynamicHandle> Traits;
		BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), "", Traits);
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(AssetData.RenderGraphInstance.GetId(), Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	});
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetProfile(const FString& AssetId, const FString& ContractId)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadAssetProfilesAction> LoadAssetProfilesAction = MakeShared<FLoadAssetProfilesAction>();

	LoadAssetProfilesAction->TryLoadAssetProfile(ContractId, MemoryCacheLoader, TempCacheLoader)
	.Next([this, AssetId, Promise, LoadAssetProfilesAction](bool bSuccess)
	{
		if (bSuccess)
		{
			for (const auto& Element : LoadAssetProfilesAction->AssetProfiles)
			{
				RegisterAssetProfile(Element.Value);
			}
			for (const auto& Element : LoadAssetProfilesAction->BlueprintInstances)
			{
				APIGraphProvider->RegisterBlueprintInstance(Element.Key, Element.Value);
			}
			for (const auto& Element : LoadAssetProfilesAction->Catalogs)
			{
				APIGraphProvider->RegisterCatalogs(Element.Key, Element.Value);
			}
			for (const auto& Element : LoadAssetProfilesAction->AssetDataMap)
			{
				RegisterAssetData(Element.Key, Element.Value);
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

TFuture<TMap<FString, UUBFBindingObject*>> UFutureverseUBFControllerSubsystem::GetTraitsForItem(
	const FString& ParsingGraphId, UUBFRuntimeController* Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const
{
	TSharedPtr<TPromise<TMap<FString, UUBFBindingObject*>>> Promise = MakeShareable(new TPromise<TMap<FString, UUBFBindingObject*>>());
	TFuture<TMap<FString, UUBFBindingObject*>> Future = Promise->GetFuture();
	
	TMap<FString, UBF::FDynamicHandle> Traits;
	const auto OnParsingGraphComplete = [this, Promise, &Traits]
	{
		// inject outputs of the parsing graph as the inputs of the graph to execute
		TArray<UBF::FBindingInfo> Outputs;
		LastParsedGraph.GetOutputs(Outputs);
			
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
		.Next([this, ParsingInputs, ParsingGraphId, OnParsingGraphComplete, Controller, Promise, &Traits]
		(const UBF::FLoadGraphResult& Result)
	{
		if (!Result.Result.Key)
		{
			UE_LOG(LogUBF, Error, TEXT("Aborting execution: graph '%s' is invalid"), *ParsingGraphId);
			Promise->SetValue(UBFUtils::AsBindingObjectMap(Traits));
			return;
		}

		LastParsedGraph = Result.Result.Value;
		LastParsedGraph.Execute(ParsingGraphId, Controller->RootComponent, APIGraphProvider.Get(), APISubGraphProvider.Get(),
			ParsingInputs, OnParsingGraphComplete, LastParsingGraphExecutionContextHandle);
	});

	return Future;
}

void UFutureverseUBFControllerSubsystem::ParseInputs(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete, TSharedPtr<FContextTree> ContextTree,
	const bool bShouldBuildContextTree)
{
	const auto AssetData = AssetDataMap[Item->GetAssetID()];

	// get variables from rendering graph instance
	APIGraphProvider->GetGraphInstance(AssetData.RenderGraphInstance.GetId()).Next(
		[this, Item, Controller, InputMap, OnComplete, ContextTree, bShouldBuildContextTree]
		(const UBF::FLoadGraphInstanceResult& LoadResult)
	{
		if(!LoadResult.Result.Key)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s failed to get parsing graph instance. Cannot render."), *Item->GetAssetID());
			return;
		}
			
		TMap<FString, UBF::FDynamicHandle> Variables;
		LoadResult.Result.Value.GetVariables(Variables);
			
		TMap<FString, UUBFBindingObject*> VariableMap = UBFUtils::AsBindingObjectMap(Variables);
					
		// get metadata json string from original json
		FString MetadataJson;
		TSharedPtr<FJsonObject> MetadataObjectField = Item->GetInventoryItem().OriginalData.JsonObject
			->GetObjectField(TEXT("node"))->GetObjectField(TEXT("metadata"));
				
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MetadataJson);
		FJsonSerializer::Serialize(MetadataObjectField.ToSharedRef(), Writer);
		
		TMap<FString, UBF::FDynamicHandle> ParsingInputs =
		{
			{TEXT("metadata"), UBF::FDynamicHandle::String(MetadataJson) }
		};
				
		const auto ParsingGraphId = AssetDataMap[Item->GetAssetID()].ParsingGraphInstance.GetId();
		GetTraitsForItem(ParsingGraphId, Controller, ParsingInputs).Next(
			[this, Item, ContextTree, bShouldBuildContextTree, &VariableMap, InputMap, Controller, OnComplete]
			(const TMap<FString, UUBFBindingObject*>& Traits)
		{
			if (bShouldBuildContextTree)
			{
				BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), Item->GetAssetID(), UBFUtils::AsDynamicMap(Traits));
			}

			// input priorities in order (inputs, traits, blueprint variables)
			VariableMap.Append(Traits);
			VariableMap.Append(InputMap);
					
			for (auto Input : VariableMap)
			{
				UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs ResolvedInput Key: %s Value: %s"), *Input.Key, *Input.Value->ToString());
			}
					
			APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
			ExecuteGraph(Item->GetAssetID(), Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), VariableMap, OnComplete);
		});
	});
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

void UFutureverseUBFControllerSubsystem::ExecuteGraph(const FString& GraphId, UUBFRuntimeController* Controller,
    IGraphProvider* GraphProvider, ISubGraphResolver* SubGraphResolver,
    const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	if (!Controller)
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller provided. Cannot render."));
		return;
	}

	Controller->SetGraphProviders(GraphProvider, SubGraphResolver);
	Controller->ExecuteGraph(GraphId, InputMap, OnComplete);
}

void UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree(const TSharedPtr<FContextTree>& ContextTree,
	const FFutureverseAssetTreeData& AssetTree, const FString& TraitTargetId, const TMap<FString, UBF::FDynamicHandle>& Traits) const
{
	const auto ItemAssetTree = AssetTree.TreePaths;
	if (ItemAssetTree.IsEmpty()) return;
	
	for (int i = 0; i < ItemAssetTree.Num(); ++i)
	{
		if (ItemAssetTree[i].Objects.IsEmpty()) continue;

		if (!AssetTree.LinkedItems.Contains(ItemAssetTree[i].Id))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree AssetTree.LinkedItems does not contain %s."), *ItemAssetTree[i].Id);
			continue;
		}
		const auto RootNode = ContextTree->AddItem(AssetTree.LinkedItems[ItemAssetTree[i].Id]);
		ContextTree->SetRoot(RootNode);
		
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree Adding Root Node %s."), *AssetTree.LinkedItems[ItemAssetTree[i].Id]);
		if (!TraitTargetId.IsEmpty() && AssetTree.LinkedItems[ItemAssetTree[i].Id] == TraitTargetId)
		{
			RootNode->AddTraits(Traits);
		}
		
		for (const auto AssetTreeObject : ItemAssetTree[i].Objects)
		{
			if (!AssetTreeObject.Key.Contains(TEXT("path:"))) continue;
			if (!AssetTree.LinkedItems.Contains(AssetTreeObject.Value.Id))
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::BuildContextTreeFromAssetTree AssetTree.LinkedItems does not contain %s."), *ItemAssetTree[i].Id);
				continue;
			}
			const auto ChildNode = ContextTree->AddItem(AssetTree.LinkedItems[AssetTreeObject.Value.Id]);
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

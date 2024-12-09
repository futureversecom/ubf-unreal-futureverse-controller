// Fill out your copyright notice in the Description page of Project Settings.


#include "FutureverseUBFControllerSubsystem.h"

#include "EmergenceSingleton.h"
#include "FutureverseUBFControllerLog.h"
#include "FutureverseUBFControllerSettings.h"
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

	if (Item->GetAssetProfile().IsValid())
	{
		// todo: get bindings from blueprint instance and smosh variables with parsed inputs
		if (ParsingGraphs.Contains(Item->GetAssetProfileRef().Id))
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, false);
			return;
		}
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
		return;
	}

	TryLoadAssetProfile(Item->GetInventoryItem().contract).Next([this, Item, Controller, InputMap, OnComplete, ContextTree](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid AssetProfile. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		Item->SetAssetProfile(GetAssetProfile(Item->GetAssetID()));
		
		if (ParsingGraphs.Contains(Item->GetAssetProfileRef().Id))
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, false);
			return;
		}
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UFuturePassInventoryItem* Item,
														UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FContextTree> ContextTree = MakeShared<FContextTree>();

	if (Item->GetAssetProfile().IsValid())
	{
		if (!ParsingGraphs.Contains(Item->GetAssetProfileRef().Id))
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, true);
			return;
		}

		// no traits because this item doesn't have a parsing graph but still build the tree
		const TMap<FString, UBF::FDynamicHandle> Traits;
		BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), "", Traits);
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
		return;
	}
	
	TryLoadAssetProfile(Item->GetInventoryItem().contract).Next([this, Item, Controller, InputMap, OnComplete, ContextTree](const bool bIsAssetProfileLoaded)
	{
		if (!bIsAssetProfileLoaded)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s provided invalid AssetProfile. Cannot render."), *Item->GetAssetID());
			return;
		}
		
		Item->SetAssetProfile(GetAssetProfile(Item->GetAssetID()));
		
		if (!ParsingGraphs.Contains(Item->GetAssetProfileRef().Id))
		{
			ParseInputs(Item, Controller, InputMap, OnComplete, ContextTree, true);
			return;
		}

		// no traits because this item doesn't have a parsing graph but still build the tree
		const TMap<FString, UBF::FDynamicHandle> Traits;
		BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), "", Traits);
	
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	});
}

TFuture<bool> UFutureverseUBFControllerSubsystem::TryLoadAssetProfile(const FString& ContractId)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadAssetProfilesAction> LoadAssetProfilesAction = MakeShared<FLoadAssetProfilesAction>();

	LoadAssetProfilesAction->TryLoadAssetProfile(ContractId, MemoryCacheLoader, TempCacheLoader).Next([this, Promise, LoadAssetProfilesAction](bool bSuccess)
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
			Promise->SetValue(true);
		}
		else
		{
			Promise->SetValue(false);
		}
	});

	return Future;
}

void UFutureverseUBFControllerSubsystem::ParseInputs(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
	const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete, TSharedPtr<FContextTree> ContextTree,
	const bool bShouldBuildContextTree)
{
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
			
	LastParsedGraph = ParsingGraphs[Item->GetAssetProfileRef().Id];
			
	const auto OnParsingGraphComplete = [this, ContextTree, Item, Controller, InputMap, OnComplete, bShouldBuildContextTree]
	{
		// inject outputs of the parsing graph as the inputs of the graph to execute
		TMap<FString, UBF::FDynamicHandle> Traits;
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

		if (bShouldBuildContextTree)
		{
			BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), Item->GetAssetProfileRef().Id, Traits);
		}
		
		TMap<FString, UUBFBindingObject*> ResolvedInputMap = InputMap;
		for (auto& Trait : Traits)
		{
			UUBFBindingObject* BindingObject = NewObject<UUBFBindingObject>();
			const UBF::FBindingInfo BindingInfo(Trait.Key, Trait.Value.GetTypeString(), Trait.Value);
			BindingObject->Initialize(BindingInfo);

			// inputs have higher priority over the traits
			if (!ResolvedInputMap.Contains(Trait.Key))
			{
				ResolvedInputMap.Add(Trait.Key, BindingObject);
			}
		}

		for (auto Input : ResolvedInputMap)
		{
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs ResolvedInput Key: %s Value: %s"), *Input.Key, *Input.Value->ToString());
		}
		
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), ResolvedInputMap, OnComplete);
	};
			
	LastParsedGraph.Execute("parsing", Controller->RootComponent, nullptr,  nullptr, ParsingInputs, OnParsingGraphComplete, LastParsingGraphExecutionContextHandle);
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

void UFutureverseUBFControllerSubsystem::ExecuteGraph(UFuturePassInventoryItem* Item, UUBFRuntimeController* Controller,
                                                      IGraphProvider* GraphProvider, ISubGraphResolver* SubGraphResolver,
                                                      const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	if (!Item)
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Item provided. Cannot render."));
		return;
	}
	if (!Controller)
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller provided. Cannot render."));
		return;
	}
	
	FAssetProfile AssetProfile = Item->GetAssetProfileRef();

	if (!AssetProfile.IsValid())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid AssetProfile. Cannot render."), *Item->GetAssetID());
		return;
	}

	Controller->SetGraphProviders(GraphProvider, SubGraphResolver);
	Controller->ExecuteGraph(AssetProfile.Id, InputMap, OnComplete);
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
	APIGraphProvider->RegisterAssetProfile(AssetProfile);
}

void UFutureverseUBFControllerSubsystem::ClearAssetProfiles()
{
	AssetProfiles.Reset();
}

FAssetProfile UFutureverseUBFControllerSubsystem::GetAssetProfile(const FString& AssetID) const
{
	if (AssetProfiles.Contains(AssetID))
	{
		return AssetProfiles[AssetID];
	}

	return FAssetProfile();
}

UFutureverseUBFControllerSubsystem* UFutureverseUBFControllerSubsystem::Get(const UObject* WorldContext)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GameInstance->GetSubsystem<UFutureverseUBFControllerSubsystem>();
	}

	return nullptr;
}
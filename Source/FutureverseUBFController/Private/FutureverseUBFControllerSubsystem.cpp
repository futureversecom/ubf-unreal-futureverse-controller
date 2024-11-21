// Fill out your copyright notice in the Description page of Project Settings.


#include "FutureverseUBFControllerSubsystem.h"

#include "EmergenceSingleton.h"
#include "FutureverseUBFControllerLog.h"
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
	APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
	
	ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UFuturePassInventoryItem* Item,
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FContextTree> ContextTree = MakeShared<FContextTree>();

	if (Item->GetAssetTreeRef().TreePaths.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree %s does not have an asset tree."), *Item->GetAssetID());
		
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	}
	else
	{
		TMap<FString, UBF::FDynamicHandle> Traits;
		if (ParsingGraphs.Contains(Item->GetAssetProfileRef().Id))
		{
			UE_LOG(LogFutureverseUBFController, Log, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Found Parsing Graph for %s."), *Item->GetAssetProfileRef().Id);
			
			FString MetadataJson;
			Item->GetInventoryItem().OriginalData.JsonObjectToString(MetadataJson);

			TMap<FString, UBF::FDynamicHandle> ParsingInputs =
			{
				{
					TEXT("metadata"), UBF::FDynamicHandle::String(MetadataJson) 
				}
			};
			
			LastParsedGraph = ParsingGraphs[Item->GetAssetProfileRef().Id];
			
			const auto OnParsingGraphComplete = [this, ContextTree, Item, Controller, InputMap, OnComplete, &Traits]
			{
				UE_LOG(LogFutureverseUBFController, Log, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Finished executing parsing graph %s"), *Item->GetAssetProfileRef().Id);
				
				TArray<UBF::FBindingInfo> Outputs;
				LastParsedGraph.GetOutputs(Outputs);
			
				for (auto Output : Outputs)
				{
					UBF::FDynamicHandle DynamicOutput;
					UE_LOG(LogFutureverseUBFController, Log, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree TryReadOutput %s"), *Output.Id);
					if (LastParsingGraphExecutionContextHandle.TryReadOutput(Output.Id, DynamicOutput))
					{
						Traits.Add(Output.Id, Output.DynamicPtr);
					}
				}
				
				BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), Traits);
				
				APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
				UE_LOG(LogFutureverseUBFController, Log, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Executing Item Graph %s."), *Item->GetAssetID());
				ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
			};

			UE_LOG(LogFutureverseUBFController, Log, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Executing Parsing Graph %s."), *Item->GetAssetProfileRef().Id);
			LastParsedGraph.Execute("parsing", Controller->RootComponent, nullptr, nullptr, ParsingInputs, nullptr, OnParsingGraphComplete, LastParsingGraphExecutionContextHandle);
			return;
		}
		
		UE_LOG(LogFutureverseUBFController, Log, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree No Parsing Graph Found for %s."), *Item->GetAssetProfileRef().Id);
			
		BuildContextTreeFromAssetTree(ContextTree, Item->GetAssetTreeRef(), Traits);
			
		APISubGraphProvider = MakeShared<FAPISubGraphResolver>(ContextTree);
		ExecuteGraph(Item, Controller, APIGraphProvider.Get(), APISubGraphProvider.Get(), InputMap, OnComplete);
	}
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
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem null Item provided. Cannot render."));
		return;
	}
	if (!Controller)
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem null Controller provided. Cannot render."));
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
	const FFutureverseAssetTreeData& AssetTree, const TMap<FString, UBF::FDynamicHandle>& RootTraits) const
{
	const auto ItemAssetTree = AssetTree.TreePaths;
	if (ItemAssetTree.IsEmpty()) return;

	const auto RootNode = ContextTree->AddItem(AssetTree.LinkedItems[ItemAssetTree[0].Id], RootTraits);
	ContextTree->SetRoot(RootNode);
		
	for (int i = 0; i < ItemAssetTree.Num(); ++i)
	{
		for (const auto AssetTreeObject : ItemAssetTree[i].Objects)
		{
			const auto Node = ContextTree->AddItem(AssetTree.LinkedItems[AssetTreeObject.Value.Id]);

			FString Relationship = AssetTreeObject.Key;
			
			RootNode->AddChild(Node, Relationship);
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
	
	if(!AssetProfile.ParsingBlueprintUri.IsEmpty())
	{
		APIUtils::LoadStringFromURI(AssetProfile.GetParsingBlueprintUri(), "ParsingGraph", TempCacheLoader.Get())
			.Next([this, AssetProfile](const UBF::FLoadStringResult& ParsingBlueprintResult)
		{
			if (!ParsingBlueprintResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing blueprint from %s"), *AssetProfile.GetParsingBlueprintUri());
				return;
			}
					
			UBF::FGraphHandle Graph;
			if (UBF::FGraphHandle::Load(UBF::FRegistryHandle::Default(), ParsingBlueprintResult.Result.Value, Graph))
			{
				UE_LOG(LogUBFAPIController, Log, TEXT("Successfully loaded ParsingGraph for %s"), *AssetProfile.Id);
				ParsingGraphs.Add(AssetProfile.Id, Graph);
			}
			else
			{
				UE_LOG(LogUBFAPIController, Warning, TEXT("Unable to load ParsingGraph for %s"), *AssetProfile.Id);
			}
		});
	}
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
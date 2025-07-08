// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "FutureverseUBFControllerSubsystem.h"

#include "BlueprintUBFLibrary.h"
#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "UBFLogData.h"
#include "AssetProfile/AssetProfileRegistrySubsystem.h"
#include "Util/UBFUtils.h"
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

void UFutureverseUBFControllerSubsystem::RenderItem(UUBFItem* Item, const FString& VariantID, UUBFRuntimeController* Controller,
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
	
	TSharedPtr<FRenderItemInfo> RenderItemInfo = MakeShared<FRenderItemInfo>();
	RenderItemInfo->Controller = Controller;
	RenderItemInfo->InputMap = InputMap;
	RenderItemInfo->OnComplete = OnComplete;
	
	// TODO what if item becomes invalid while we load this?
	// TODO what if subsystem becomes invalid while we load this?
	
	Item->EnsureProfileURILoaded().Next([this, Item, RenderItemInfo, VariantID](bool bResult)
	{
		Item->EnsureContextTreeLoaded().Next([this, Item, RenderItemInfo, VariantID](bool bResult)
		{
			if (!IsSubsystemValid()) return;
			
			RenderItemInfo->RenderData = FUBFRenderDataContainer::GetFromData(Item->GetRenderData(), VariantID);
				
			RenderItemInternal(RenderItemInfo);
		});
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTree(UUBFItem* Item, const FString& VariantID, 
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

	TSharedPtr<FRenderItemInfo> RenderItemInfo = MakeShared<FRenderItemInfo>();
	RenderItemInfo->Controller = Controller;
	RenderItemInfo->InputMap = InputMap;
	RenderItemInfo->OnComplete = OnComplete;
	
	// TODO what if item becomes invalid while we load this?
	// TODO what if subsystem becomes invalid while we load this?
	
	Item->EnsureProfileURILoaded().Next([this, Item, RenderItemInfo, VariantID](bool bResult)
	{
		Item->EnsureContextTreeLoaded().Next([this, Item, RenderItemInfo, VariantID](bool bResult)
		{
			if (!IsSubsystemValid()) return;
			
			RenderItemInfo->RenderData = FUBFRenderDataContainer::GetFromData(Item->GetRenderData(), VariantID);
				
			RenderItemInternal(RenderItemInfo);
		});
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemFromRenderData(const FUBFRenderData& RenderData, const FString& VariantID, 
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FRenderItemInfo> RenderItemInfo = MakeShared<FRenderItemInfo>();
	RenderItemInfo->RenderData = FUBFRenderDataContainer::GetFromData(RenderData, VariantID);
	RenderItemInfo->Controller = Controller;
	RenderItemInfo->InputMap = InputMap;
	RenderItemInfo->OnComplete = OnComplete;
	
	RenderItemInternal(RenderItemInfo);
}

void UFutureverseUBFControllerSubsystem::RenderItemTreeFromRenderData(const FUBFRenderData& RenderData, const FString& VariantID, 
	UUBFRuntimeController* Controller, const TMap<FString, UUBFBindingObject*>& InputMap, const FOnComplete& OnComplete)
{
	TSharedPtr<FRenderItemInfo> RenderItemInfo = MakeShared<FRenderItemInfo>();
	RenderItemInfo->RenderData = FUBFRenderDataContainer::GetFromData(RenderData, VariantID);
	RenderItemInfo->Controller = Controller;
	RenderItemInfo->InputMap = InputMap;
	RenderItemInfo->OnComplete = OnComplete;
	
	RenderItemTreeInternal(RenderItemInfo);
}

TFuture<TMap<FString, UUBFBindingObject*>> UFutureverseUBFControllerSubsystem::GetTraitsForItem(
	const FString& ParsingGraphId, const TWeakObjectPtr<UUBFRuntimeController>& Controller, const TMap<FString, UBF::FDynamicHandle>& ParsingInputs) const
{
	TSharedPtr<TPromise<TMap<FString, UUBFBindingObject*>>> Promise = MakeShareable(new TPromise<TMap<FString, UUBFBindingObject*>>());
	TFuture<TMap<FString, UUBFBindingObject*>> Future = Promise->GetFuture();
	
	if (!IsSubsystemValid() || !Controller.IsValid() || !IsValid(Controller.Get()) || !IsValid(Controller->RootComponent))
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

void UFutureverseUBFControllerSubsystem::ParseInputsThenExecute(TSharedPtr<FRenderItemInfo> RenderItemInfo,
								const bool bShouldBuildContextTree) const
{
	// get metadata json string from original json
	UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UFutureverseUBFControllerSubsystem::ParseInputs Parsing Metadata: %s"), *RenderItemInfo->RenderData->GetMetadataJson());
	TMap<FString, UBF::FDynamicHandle> ParsingInputs =
	{
		{TEXT("metadata"), UBF::FDynamicHandle::String(RenderItemInfo->RenderData->GetMetadataJson()) }
	};
				
	const auto ParsingGraphId = RenderItemInfo->AssetProfiles.Get(RenderItemInfo->RenderData->GetAssetID()).GetParsingBlueprintId(RenderItemInfo->RenderData->GetVariantID());
	GetTraitsForItem(ParsingGraphId, RenderItemInfo->Controller, ParsingInputs).Next(
		[this, RenderItemInfo, bShouldBuildContextTree]
		(const TMap<FString, UUBFBindingObject*>& Traits)
	{
		if (!IsSubsystemValid()) return;
			
		RenderItemInfo->InputMap.Append(Traits);
		ExecuteGraph(RenderItemInfo, bShouldBuildContextTree);
	});
}

void UFutureverseUBFControllerSubsystem::ExecuteGraph(TSharedPtr<FRenderItemInfo> RenderItemInfo, const bool bShouldBuildContextTree) const
{
	FBlueprintExecutionData ExecutionData;

	FString RenderBlueprintId = RenderItemInfo->AssetProfiles.Get(RenderItemInfo->RenderData->GetAssetID()).GetRenderBlueprintId(RenderItemInfo->RenderData->GetVariantID());

	if (bShouldBuildContextTree)
	{
		CreateBlueprintInstancesFromContextTree(RenderItemInfo, RenderItemInfo->RenderData->GetContextTreeRef(), RenderItemInfo->RenderData->GetAssetID(), ExecutionData.BlueprintInstances);
	}
	else
	{
		ExecutionData.BlueprintInstances.Add(UBF::FExecutionInstanceData(RenderBlueprintId));
	}
	
	ExecutionData.InputMap = RenderItemInfo->InputMap;

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
	
	if (!RenderItemInfo->Controller.IsValid() || !IsValid(RenderItemInfo->Controller.Get()) && !IsValid(RenderItemInfo->Controller->RootComponent))
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteGraph null Controller or null root component provided. Cannot render."));
		return;
	}
	
	RenderItemInfo->Controller->ExecuteBlueprint(InstanceID, ExecutionData, RenderItemInfo->OnComplete);
}

TFuture<FLoadLinkedAssetProfilesResult> UFutureverseUBFControllerSubsystem::EnsureAssetDatasLoaded(
	const TArray<FFutureverseAssetLoadData>& LoadDatas) const
{
	TSharedPtr<TPromise<FLoadLinkedAssetProfilesResult>> Promise = MakeShareable(new TPromise<FLoadLinkedAssetProfilesResult>());
	TFuture<FLoadLinkedAssetProfilesResult> Future = Promise->GetFuture();
	
	const auto AssetProfileRegistry = UAssetProfileRegistrySubsystem::Get(GetWorld());
	check(AssetProfileRegistry);
	
	if (!AssetProfileRegistry)
	{
		UE_LOG(LogFutureverseUBFController, Error,
			TEXT("UFutureverseUBFControllerSubsystem Failed to EnsureAssetDatasLoaded because AssetProfileRegistrySubsystem was null!"));
		
		auto FailResult = FLoadLinkedAssetProfilesResult();
		FailResult.SetFailure();
		Promise->SetValue(FailResult);
		return Future;
	}
	
	AssetProfileRegistry->GetLinkedAssetProfiles(LoadDatas).Next([this, Promise
		](const FLoadLinkedAssetProfilesResult& Result)
	{
		if (!IsSubsystemValid())
		{
			auto FailResult = FLoadLinkedAssetProfilesResult();
			FailResult.SetFailure();
			Promise->SetValue(FailResult);
			return;
		}
		
		Promise->SetValue(Result);
	});

	return Future;
}

TFuture<FLoadAssetProfileResult> UFutureverseUBFControllerSubsystem::EnsureAssetDataLoaded(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<FLoadAssetProfileResult>> Promise = MakeShareable(new TPromise<FLoadAssetProfileResult>());
	TFuture<FLoadAssetProfileResult> Future = Promise->GetFuture();
	
	EnsureAssetProfilesLoaded(LoadData).Next([LoadData, this, Promise]
		(const FLoadAssetProfileResult& Result)
	{
		if (!IsSubsystemValid() || !Result.bSuccess)
		{
			FLoadAssetProfileResult OutResult;
			OutResult.SetFailure();
			Promise->SetValue(OutResult);
			return;
		}
		FAssetProfile AssetProfile = Result.Value;
		EnsureCatalogsLoaded(LoadData, AssetProfile).Next([Promise, AssetProfile, this](bool bResult)
		{
			FLoadAssetProfileResult OutResult;
			if (!IsSubsystemValid() || !bResult)
			{
				OutResult.SetFailure();
				Promise->SetValue(OutResult);
				return;
			}
			
			OutResult.SetResult(AssetProfile);
			Promise->SetValue(OutResult);
		});
	});

	return Future;
}

TFuture<FLoadAssetProfileResult> UFutureverseUBFControllerSubsystem::EnsureAssetProfilesLoaded(const FFutureverseAssetLoadData& LoadData) const
{
	TSharedPtr<TPromise<FLoadAssetProfileResult>> Promise = MakeShareable(new TPromise<FLoadAssetProfileResult>());
	TFuture<FLoadAssetProfileResult> Future = Promise->GetFuture();

	const auto AssetProfileRegistry = UAssetProfileRegistrySubsystem::Get(GetWorld());
	check(AssetProfileRegistry);
	if (!AssetProfileRegistry)
	{
		FLoadAssetProfileResult Result;
		UE_LOG(LogFutureverseUBFController,
				Error,
				TEXT("UFutureverseUBFControllerSubsystem Failed to get AssetProfile because AssetProfileRegistrySubsystem was null!"));

		Result.SetFailure();
		Promise->SetValue(Result);
		return Future;
	}

	AssetProfileRegistry->GetAssetProfile(LoadData).Next([this, Promise]
		(const FLoadAssetProfileResult& AssetProfileResult)
	{
		FLoadAssetProfileResult OutResult;
		if (!IsSubsystemValid())
		{
			OutResult.SetFailure();
			Promise->SetValue(OutResult);
			return;
		}

		if (AssetProfileResult.bSuccess)
		{
			OutResult.SetResult(AssetProfileResult.Value);
			Promise->SetValue(OutResult);
		}
		else
		{
			OutResult.SetFailure();
			Promise->SetValue(OutResult);
		}
	});

	return Future;
}

TFuture<bool> UFutureverseUBFControllerSubsystem::EnsureCatalogsLoaded(const FFutureverseAssetLoadData& LoadData,
	const FAssetProfile& AssetProfile)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	if (IsCatalogLoaded(LoadData))
	{
		Promise->SetValue(true);
		return Future;
	}

	TSharedPtr<FLoadAssetCatalogAction> LoadAssetCatalogAction = MakeShared<FLoadAssetCatalogAction>();

	LoadAssetCatalogAction->TryLoadAssetCatalog(AssetProfile, LoadData, MemoryCacheLoader)
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

bool UFutureverseUBFControllerSubsystem::IsCatalogLoaded(const FFutureverseAssetLoadData& LoadData) const
{
	return LoadedVariantCatalogs.Contains(LoadData.GetCombinedVariantID());
}

void UFutureverseUBFControllerSubsystem::ExecuteItemGraph(TSharedPtr<FRenderItemInfo> RenderItemInfo, const bool bShouldBuildContextTree) const
{
	const auto& AssetProfile = RenderItemInfo->AssetProfiles.Get(RenderItemInfo->RenderData->GetAssetID());
		
	if (!AssetProfile.GetParsingBlueprintId(RenderItemInfo->RenderData->GetVariantID()).IsEmpty())
	{
		ParseInputsThenExecute(RenderItemInfo, bShouldBuildContextTree);
		return;
	}

	if (AssetProfile.GetRenderBlueprintId(RenderItemInfo->RenderData->GetVariantID()).IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::ExecuteItemGraph Item %s provided invalid Rendering Graph Instance. Cannot render."), *RenderItemInfo->RenderData->GetAssetID());
		RenderItemInfo->OnComplete.ExecuteIfBound(false, FUBFExecutionReport::Failure());
		return;
	}
	
	ExecuteGraph(RenderItemInfo, bShouldBuildContextTree);
}

void UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree(TSharedPtr<FRenderItemInfo> RenderItemInfo,
	const TArray<FUBFContextTreeData>& UBFContextTree, const FString& RootAssetId, TArray<UBF::FExecutionInstanceData>& OutBlueprintInstances) const
{
	if (UBFContextTree.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Can't build context tree beacuse UBFContextTree is empty."));
		
		auto ParentNodeRenderBlueprint = RenderItemInfo->AssetProfiles.Get(RootAssetId).GetRenderBlueprintId(RenderItemInfo->RenderData->GetVariantID());
		UBF::FExecutionInstanceData BlueprintInstance(ParentNodeRenderBlueprint);
		OutBlueprintInstances.Reset();
		OutBlueprintInstances.Add(BlueprintInstance);
		return;
	}

	TMap<FString, UBF::FExecutionInstanceData> AssetIdToInstanceMap;

	for (const auto& ContextTreeData : UBFContextTree)
	{
		if (!RenderItemInfo->AssetProfiles.Contains(ContextTreeData.RootNodeID))
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ContextTreeData.RootNodeID);
			continue;
		}
		
		auto ParentNodeRenderBlueprint = RenderItemInfo->AssetProfiles.Get(ContextTreeData.RootNodeID).GetRenderBlueprintId(RenderItemInfo->RenderData->GetVariantID());
		UBF::FExecutionInstanceData BlueprintInstance(ParentNodeRenderBlueprint);

		UE_LOG(LogFutureverseUBFController, Verbose, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree Adding BlueprintInstance to mapping with Key: %s Value: %s.")
			, *ContextTreeData.RootNodeID, *BlueprintInstance.ToString());
		AssetIdToInstanceMap.Add(ContextTreeData.RootNodeID, BlueprintInstance);
	}

	for (const auto& ContextTreeData : UBFContextTree)
	{
		for (const auto& Relationship : ContextTreeData.Relationships)
		{
			if (!RenderItemInfo->AssetProfiles.Contains(Relationship.ChildAssetID))
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *Relationship.ChildAssetID);
				continue;
			}

			if (!AssetIdToInstanceMap.Contains(Relationship.ChildAssetID))
			{
				if (!RenderItemInfo->AssetProfiles.Contains(ContextTreeData.RootNodeID))
				{
					UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::CreateBlueprintInstancesFromContextTree AssetDataMap does not contain %s."), *ContextTreeData.RootNodeID);
					continue;
				}

				auto ChildNodeRenderBlueprint = RenderItemInfo->AssetProfiles.Get(Relationship.ChildAssetID).GetRenderBlueprintId(RenderItemInfo->RenderData->GetVariantID());
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

void UFutureverseUBFControllerSubsystem::RenderItemInternal(TSharedPtr<FRenderItemInfo> RenderItemInfo)
{
	
	FFutureverseAssetLoadData LoadData = FFutureverseAssetLoadData(RenderItemInfo->RenderData->GetAssetID(), RenderItemInfo->RenderData->GetProfileURI());
	LoadData.VariantID = RenderItemInfo->RenderData->GetVariantID();
	
	EnsureAssetDataLoaded(LoadData).Next([this, RenderItemInfo, LoadData]
		(const FLoadAssetProfileResult& Result)
	{
		if (!IsSubsystemValid()) return;
			
		if (!Result.bSuccess)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItem Item %s provided invalid AssetProfile. Cannot render."), *RenderItemInfo->RenderData->GetAssetID());
			RenderItemInfo->OnComplete.ExecuteIfBound(false, FUBFExecutionReport::Failure());
			return;
		}
		RenderItemInfo->AssetProfiles.Add(LoadData.AssetID, Result.Value);
		ExecuteItemGraph(RenderItemInfo, false);
	});
}

void UFutureverseUBFControllerSubsystem::RenderItemTreeInternal(TSharedPtr<FRenderItemInfo> RenderItemInfo)
{
	TArray<FFutureverseAssetLoadData> AssetLoadDatas = RenderItemInfo->RenderData->GetLinkedAssetLoadData();

	for (FFutureverseAssetLoadData& AssetLoadData : AssetLoadDatas)
	{
		AssetLoadData.VariantID = RenderItemInfo->RenderData->GetVariantID();
	}

	if (AssetLoadDatas.IsEmpty())
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree AssetLoadDatas empty for Item %s."), *RenderItemInfo->RenderData->GetAssetID());
	
	EnsureAssetDatasLoaded(AssetLoadDatas).Next([this, RenderItemInfo]
		(const FLoadLinkedAssetProfilesResult& Result)
	{
		if (!IsSubsystemValid()) return;
		
		if (!Result.bSuccess)
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFutureverseUBFControllerSubsystem::RenderItemTree Item %s asset tree failed to load one or many AssetDatas. This will cause asset tree to not render fully"), *RenderItemInfo->RenderData->GetAssetID());
		}
		RenderItemInfo->AssetProfiles = Result.Value;
		ExecuteItemGraph(RenderItemInfo, true);
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
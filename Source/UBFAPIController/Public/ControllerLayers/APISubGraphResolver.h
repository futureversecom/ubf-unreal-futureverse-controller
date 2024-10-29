#pragma once
#include "ContextTree.h"
#include "SubGraphResolver.h"

class FAPISubGraphResolver : public ISubGraphResolver
{
public:
	FAPISubGraphResolver(const TSharedPtr<FContextTree>& ContextTree) : ContextTree(ContextTree) {}
	
	virtual bool TryResolveSubGraph(
		FString Key, const TMap<FString, UBF::FDynamicHandle>& ParentGraphInputs,
		FString& BlueprintId, TMap<FString, UBF::FDynamicHandle>& ResolvedInputs) override
	{
		UE_LOG(LogUBFAPIController, Verbose, TEXT("FAPISubGraphResolver::TryResolveSubGraph for Key %s BlueprintId: %s"), *Key, *BlueprintId);
		
		TSharedPtr<FContextData> NewMarker;
		if (!ContextTree->TryMoveMarker(Key, NewMarker))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("Failed to move marker for relationship %s"), *Key);
			ResolvedInputs = ParentGraphInputs;
			BlueprintId = "";
			return false;
		}

		auto Traits = NewMarker->GetTraits(); // This assumes that the marker has been moved to the new item in SpawnSocket
		BlueprintId = NewMarker->GetBlueprintID();

		ResolvedInputs = TMap<FString, UBF::FDynamicHandle>();
		for (auto KVP : Traits)
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("Traits: %s: %s"), *KVP.Key, *KVP.Value.ToString());
			ResolvedInputs.Add(KVP.Key, KVP.Value);
		}

		for (auto Input : ParentGraphInputs)
		{
			if (!Input.Value.IsNull() && Input.Key != FString(TEXT("Key")))
			{
				if (ResolvedInputs.Contains(Input.Key))
				{
					ResolvedInputs[Input.Key] = Input.Value;
				}
				else
				{
					ResolvedInputs.Add(Input.Key, Input.Value);
				}
			}
		}

		return true;
	};

private:
	TSharedPtr<FContextTree> ContextTree;
};

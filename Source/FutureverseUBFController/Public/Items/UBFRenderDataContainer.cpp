// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "UBFRenderDataContainer.h"

#include "FutureverseAssetLoadData.h"

FUBFRenderDataContainer::FUBFRenderDataContainer(const FUBFRenderData& InData) : RenderData(InData)
{
	
}

FUBFRenderDataPtr FUBFRenderDataContainer::GetFromData(const FUBFRenderData& InData)
{
	return MakeShared<FUBFRenderDataContainer>(InData);
}

TArray<FFutureverseAssetLoadData> FUBFRenderDataContainer::GetLinkedAssetLoadData() const
{
	TArray<FFutureverseAssetLoadData> OutContractIds;
		
	for (auto& ContextTreeData : RenderData.ContextTree)
	{
		TArray<FString> Out;

		ContextTreeData.RootNodeID.ParseIntoArray(Out, TEXT(":"), true);
		if (!Out.IsEmpty())
		{
			OutContractIds.Add(FFutureverseAssetLoadData(ContextTreeData.RootNodeID, Out[0]));
		}
		
		for (auto& Relationship: ContextTreeData.Relationships)
		{
			Relationship.ChildAssetID.ParseIntoArray(Out, TEXT(":"), true);

			if (Out.IsEmpty()) continue;
			
			OutContractIds.Add(FFutureverseAssetLoadData(Relationship.ChildAssetID, Out[0]));
		}
	}

	if (OutContractIds.IsEmpty())
		OutContractIds.Add(FFutureverseAssetLoadData(GetAssetID(), GetContractID()));
		
	return OutContractIds;
}

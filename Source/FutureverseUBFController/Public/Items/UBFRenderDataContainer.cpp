// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "UBFRenderDataContainer.h"

#include "FutureverseAssetLoadData.h"

FUBFRenderDataContainer::FUBFRenderDataContainer(const FUBFRenderData& InData, const FString& VariantID) : VariantID(VariantID), RenderData(InData)
{
	if (this->VariantID.IsEmpty())
		this->VariantID = FString(TEXT("Default"));
}

FUBFRenderDataPtr FUBFRenderDataContainer::GetFromData(const FUBFRenderData& InData, const FString& VariantID)
{
	return MakeShared<FUBFRenderDataContainer>(InData, VariantID);
}

TArray<FFutureverseAssetLoadData> FUBFRenderDataContainer::GetLinkedAssetLoadData() const
{
	TArray<FFutureverseAssetLoadData> OutContractIds;
		
	for (auto& ContextTreeData : RenderData.ContextTree)
	{
		// Ids are in this format {chainId}:{chainType}:{contract}:{token}
		OutContractIds.Add(FFutureverseAssetLoadData(ContextTreeData.RootNodeID));
		
		for (auto& Relationship: ContextTreeData.Relationships)
		{
			OutContractIds.Add(FFutureverseAssetLoadData(Relationship.ChildAssetID));
		}
	}

	if (OutContractIds.IsEmpty())
		OutContractIds.Add(FFutureverseAssetLoadData(GetAssetID()));
		
	return OutContractIds;
}

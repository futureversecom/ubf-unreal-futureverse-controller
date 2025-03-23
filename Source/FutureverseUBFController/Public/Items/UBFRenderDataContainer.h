// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "UBFInventoryItem.h"

class FUBFRenderDataContainer;

typedef TSharedPtr<FUBFRenderDataContainer> FUBFRenderDataPtr;

class FUBFRenderDataContainer
{
public:
	FUBFRenderDataContainer(const FUBFRenderData& InData);

	static FUBFRenderDataPtr GetFromData(const FUBFRenderData& InData);
	
	FString GetAssetID() const { return RenderData.AssetID; }
	FString GetContractID() const { return RenderData.ContractID; }
	FString GetMetadataJson() const { return RenderData.MetadataJson; }
	TArray<FUBFContextTreeData>& GetContextTreeRef() { return RenderData.ContextTree; }
	TArray<FFutureverseAssetLoadData> GetLinkedAssetLoadData() const;
private:
	FUBFRenderData RenderData;
};

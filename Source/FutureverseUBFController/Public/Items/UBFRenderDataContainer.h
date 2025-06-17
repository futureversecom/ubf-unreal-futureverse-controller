// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "FutureverseAssetLoadData.h"

#include "UBFInventoryItem.h"

class FUBFRenderDataContainer;

typedef TSharedPtr<FUBFRenderDataContainer> FUBFRenderDataPtr;

class FUBFRenderDataContainer
{
public:
	FUBFRenderDataContainer(const FUBFRenderData& InData, const FString& VariantID);

	static FUBFRenderDataPtr GetFromData(const FUBFRenderData& InData, const FString& VariantID);
	
	FString GetAssetID() const { return RenderData.AssetID; }
	FString GetMetadataJson() const { return RenderData.MetadataJson; }
	TArray<FUBFContextTreeData>& GetContextTreeRef() { return RenderData.ContextTree; }
	TArray<FFutureverseAssetLoadData> GetLinkedAssetLoadData() const;

	FString GetVariantID() const {return VariantID;}
private:
	FString VariantID;
	FUBFRenderData RenderData;
};

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "AssetIdUtils.h"

struct FFutureverseAssetLoadData
{
	FFutureverseAssetLoadData(){}
	FFutureverseAssetLoadData(const FString& AssetID) : AssetID(AssetID){}

	FString AssetID;
	FString VariantID = FString(TEXT("Default"));

	FString GetCombinedVariantID() const { return FString::Printf(TEXT("%s-%s"), *AssetID, *VariantID); }
	
	FString GetCollectionID() const { return AssetIdUtils::GetCollectionID(AssetID); }

	FString GetContractID() const{ return AssetIdUtils::GetContractID(AssetID); }
	
	FString GetTokenID() const { return AssetIdUtils::GetTokenID(AssetID); }
};

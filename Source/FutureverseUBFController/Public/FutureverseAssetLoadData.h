// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

struct FFutureverseAssetLoadData
{
	FFutureverseAssetLoadData(){}
	FFutureverseAssetLoadData(const FString& AssetID, const FString& ContractID) : AssetID(AssetID), ContractID(ContractID){}

	FString AssetID;
	FString ContractID;
	FString VariantID = FString(TEXT("Default"));

	FString GetCombinedVariantID() const
	{
		return FString::Printf(TEXT("%s-%s"), *AssetID, *VariantID);
	}

	// Function to extract AssetName from AssetID
	FString GetAssetName() const
	{
		// Find the position of the ':' separator
		int32 SeparatorIndex;
		if (AssetID.FindChar(':', SeparatorIndex))
		{
			// Extract and return the substring after the ':'
			return AssetID.Mid(SeparatorIndex + 1);
		}
        
		// If no ':' is found, return an empty string
		return "";
	}
};

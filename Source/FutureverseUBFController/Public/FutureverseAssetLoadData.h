#pragma once

struct FFutureverseAssetLoadData
{
	FFutureverseAssetLoadData(){}
	FFutureverseAssetLoadData(const FString& AssetID, const FString& ContractID) : AssetID(AssetID), ContractID(ContractID){}

	FString AssetID;
	FString ContractID;

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

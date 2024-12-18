#pragma once

struct FFutureverseAssetLoadData
{
	FFutureverseAssetLoadData(){}
	FFutureverseAssetLoadData(const FString& AssetID, const FString& ContractID) : AssetID(AssetID), ContractID(ContractID){}

	FString AssetID;
	FString ContractID;
};

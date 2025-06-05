#pragma once

namespace AssetIdUtils
{
	inline FString GetCollectionID(const FString& CombinedID)
	{
		// CombinedID is in this format e.g. {chain}:{chainType}:{contract}:{token}
		int32 Index;
		if (CombinedID.FindLastChar(TEXT(':'), Index))
		{
			// returning {chain}:{chainType}:{contract}
			return CombinedID.Left(Index);
		}
		return TEXT("");
	}

	inline FString GetContractID(const FString& CombinedID)
	{
		FString CollectionID = GetCollectionID(CombinedID);
		
		// CollectionID has this format {chainId}:{chainType}:{contract}
		TArray<FString> Parts;
		CollectionID.ParseIntoArray(Parts, TEXT(":"), true);
		
		// If no ':' is found, return an empty string
		if (Parts.IsEmpty()) return "";
		
		return Parts.Last();
	}
	
	inline FString GetTokenID(const FString& CombinedID)
	{
		// CombinedID has this format {chainId}:{chainType}:{contract}:{token}
		TArray<FString> Parts;
		CombinedID.ParseIntoArray(Parts, TEXT(":"), true);
		
		// If no ':' is found, return an empty string
		if (Parts.IsEmpty()) return "";
		
		return Parts.Last();
	}
	
	inline FString GetAssetID(const FString& CombinedID)
	{
		// CombinedID is in this format e.g. {chain}:{chainType}:{contract}:{token}
		TArray<FString> Segments;
		CombinedID.ParseIntoArray(Segments, TEXT(":"), true);

		if (Segments.Num() >= 2)
		{
			// returning {contract}:{token}
			return Segments[Segments.Num() - 2] + TEXT(":") + Segments[Segments.Num() - 1];
		}

		return TEXT("");
	}
	
	inline FString FormatAssetId(const FString& AssetId)
	{
		return AssetId.ToLower().Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT(""));
	}

	inline FString ConvertAssetIdToOverrideId(const FString& CombinedId)
	{
		return FString::Printf(TEXT("%s:%s"), *GetCollectionID(CombinedId), TEXT("override"));
	}
};

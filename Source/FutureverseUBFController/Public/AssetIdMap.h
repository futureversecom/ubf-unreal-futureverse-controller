#pragma once

template<typename T>
class TAssetIdMap
{
public:
	static FString ConvertAssetIdToOverrideId(const FString& AssetId)
	{
		FString CollectionId, AssetName;

		// Split the AssetId into CollectionId and AssetName based on the delimiter ':'
		if (AssetId.Split(TEXT(":"), &CollectionId, &AssetName))
		{
			// Combine the CollectionId with "override" to create the OverrideId
			return FString::Printf(TEXT("%s:override"), *CollectionId);
		}
		
		return AssetId;
	}
	
	static FString FormatAssetId(const FString& AssetId)
	{
		return AssetId.ToLower().Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT(""));
	}
	
	bool Contains(const FString& AssetId) const
	{
		FString FormatedAssetId = FormatAssetId(AssetId);
		if (InternalMap.Contains(FormatedAssetId))
			return true;

		if (InternalMap.Contains(ConvertAssetIdToOverrideId(AssetId)))
			return true;

		return false;
	}
	
	T Get(const FString& AssetId) const
	{
		FString FormatedAssetId = FormatAssetId(AssetId);
		if (InternalMap.Contains(FormatedAssetId))
			return InternalMap[FormatedAssetId];

		FString OverrideKey = ConvertAssetIdToOverrideId(AssetId);
		if (InternalMap.Contains(OverrideKey))
			return InternalMap[OverrideKey];

		return T();
	}
	
	void Add(const FString& AssetId, const T& Value)
	{
		InternalMap.Add(FormatAssetId(AssetId), Value);
	}
	void Remove(const FString& AssetId)
	{
		InternalMap.Remove(FormatAssetId(AssetId));
	}
	void Clear()
	{
		InternalMap.Reset();
	}
private:
	TMap<FString, T> InternalMap;
	
};

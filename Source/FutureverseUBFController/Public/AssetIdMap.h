// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "AssetIdUtils.h"

template<typename T>
class TAssetIdMap
{
public:
	
	bool Contains(const FString& AssetId) const
	{
		FString FormatedAssetId = AssetIdUtils::FormatAssetId(AssetId);
		if (InternalMap.Contains(FormatedAssetId))
			return true;

		if (InternalMap.Contains(AssetIdUtils::ConvertAssetIdToOverrideId(AssetId)))
			return true;

		return false;
	}
	
	T Get(const FString& AssetId) const
	{
		FString FormatedAssetId = AssetIdUtils::FormatAssetId(AssetId);
		if (InternalMap.Contains(FormatedAssetId))
			return InternalMap[FormatedAssetId];

		FString OverrideKey = AssetIdUtils::ConvertAssetIdToOverrideId(AssetId);
		if (InternalMap.Contains(OverrideKey))
			return InternalMap[OverrideKey];

		return T();
	}
	
	void Add(const FString& AssetId, const T& Value)
	{
		InternalMap.Add(AssetIdUtils::FormatAssetId(AssetId), Value);
	}
	void Remove(const FString& AssetId)
	{
		InternalMap.Remove(AssetIdUtils::FormatAssetId(AssetId));
	}
	void Clear()
	{
		InternalMap.Reset();
	}

	
private:
	TMap<FString, T> InternalMap;
	
};

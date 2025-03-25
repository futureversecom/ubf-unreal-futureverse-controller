// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "ICacheLoader.h"
#include "UBFAPIControllerLog.h"

class UBFAPICONTROLLER_API FMemoryCacheLoader final : public ICacheLoader
{
public:
	virtual void CacheBytes(const FString& Uri, const FString& Hash, const TArray<uint8>& Bytes) override
	{
		if (CachedBytesMap.Contains(Uri))
		{
			CachedBytesMap[Uri] = Bytes;
		}
		else
		{
			CachedBytesMap.Add(Uri, Bytes);
		}

		UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("FMemoryCacheLoader::CacheBytes Successfully cached bytes for %s"), *Uri);
	}
	
	virtual bool TryGetCachedBytes(const FString& Uri, const FString& Hash, TArray<uint8>& CachedBytes) const override
	{
		if (CachedBytesMap.Contains(Uri))
		{
			CachedBytes = CachedBytesMap[Uri];
			UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("FMemoryCacheLoader::TryGetCachedBytes Cache exists for %s"), *Uri);
			return true;
		}
		UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("FMemoryCacheLoader::TryGetCachedBytes Cache doesn't exist for %s"), *Uri);
		return false;
	}
	
private:
	TMap<FString, TArray<uint8>> CachedBytesMap;
};

#pragma once

class UBFAPICONTROLLER_API ICacheLoader
{
public:
	virtual void CacheBytes(const FString& Uri, const FString& Hash, const TArray<uint8>& Bytes) = 0;
	
	virtual bool TryGetCachedBytes(const FString& Uri, const FString& Hash, TArray<uint8>& CachedBytes) const = 0;

	virtual ~ICacheLoader() {}
};




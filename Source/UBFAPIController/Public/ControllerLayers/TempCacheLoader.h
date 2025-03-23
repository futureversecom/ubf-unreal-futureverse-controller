// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "APIUtils.h"
#include "ICacheLoader.h"
#include "UBFAPIControllerLog.h"

class UBFAPICONTROLLER_API FTempCacheLoader final : public ICacheLoader
{
public:
	
	virtual void CacheBytes(const FString& Uri, const FString& Hash, const TArray<uint8>& Bytes) override
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FTempCacheLoader::CacheBytes);
		
		if (Hash.IsEmpty())
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("FTempCacheLoader::CacheBytes Failed to cache bytes to %s because provided hash was empty!"), *Uri);
			return;
		}

		const auto CachePath = FPaths::Combine(BaseCachePath, "UBF", Hash);
		const auto Directory = FPaths::GetPath(CachePath);

		IFileManager& FileManager = IFileManager::Get();
		if (FileManager.DirectoryExists(*Directory))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("FTempCacheLoader::CacheBytes DirectoryExists at: %s"), *Directory);
			if (FFileHelper::SaveArrayToFile(Bytes, *CachePath))
			{
				UE_LOG(LogUBFAPIController, Verbose, TEXT("FTempCacheLoader::CacheBytes Successfully saved file at: %s"), *CachePath);
			}
			else
			{
				UE_LOG(LogUBFAPIController, Warning, TEXT("FTempCacheLoader::CacheBytes Failed to save file at: %s"), *CachePath);
			}
		}
		else
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("FTempCacheLoader::CacheBytes Directory Doesn't Exist at: %s"), *Directory);
			
			if (FileManager.MakeDirectory(*Directory, true))
			{
				if (FFileHelper::SaveArrayToFile(Bytes, *CachePath))
				{
					UE_LOG(LogUBFAPIController, Verbose, TEXT("FTempCacheLoader::CacheBytes Successfully saved file at: %s"), *CachePath);
				}
				else
				{
					UE_LOG(LogUBFAPIController, Warning, TEXT("FTempCacheLoader::CacheBytes Failed to save file at: %s"), *CachePath);
				}
			}
			else
			{
				UE_LOG(LogUBFAPIController, Warning, TEXT("FTempCacheLoader::CacheBytes Failed to create directory at %s"), *Directory);
			}
		}
	}
	
	virtual bool TryGetCachedBytes(const FString& Uri, const FString& Hash, TArray<uint8>& CachedBytes) const override
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FTempCacheLoader::TryGetCachedBytes);
		
		if (Hash.IsEmpty())
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("FTempCacheLoader::TryGetCachedBytes Failed to get cache from %s because provided hash was empty!"), *Uri);
			return false;
		}

		auto CachePath = FPaths::Combine(BaseCachePath, "UBF", Hash);
		auto CacheDirectory = FPaths::GetPath(CachePath);
		
		IFileManager& FileManager = IFileManager::Get();
		
		if (!FileManager.FileExists(*CachePath))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("FTempCacheLoader::TryGetCachedBytes Cache doesn't exist at %s"), *CachePath);
			return false;
		}
		
		if (!APIUtils::LoadLocalFileToData(CachePath, CachedBytes))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("FTempCacheLoader::TryGetCachedBytes Failed to load cache from %s"), *CachePath);
			return true;
		}
		
		UE_LOG(LogUBFAPIController, Verbose, TEXT("FTempCacheLoader::TryGetCachedBytes Successfully loaded cache from %s"), *Uri);
		return false;
	}
	
private:
	FString BaseCachePath = FPlatformProcess::UserTempDir();
};

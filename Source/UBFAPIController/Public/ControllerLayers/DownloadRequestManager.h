// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "GraphProvider.h"
#include "ICacheLoader.h"

struct FActiveRequestData
{
public:
	FActiveRequestData() {}
	FActiveRequestData(const TSharedPtr<TPromise<UBF::FLoadDataArrayResult>>& Promise)
	{
		DataPromises.Add(Promise);
	}
	FActiveRequestData(const TSharedPtr<TPromise<UBF::FLoadStringResult>>& Promise)
	{
		StringPromises.Add(Promise);
	}
	
	TArray<TSharedPtr<TPromise<UBF::FLoadDataArrayResult>>> DataPromises;
	TArray<TSharedPtr<TPromise<UBF::FLoadStringResult>>> StringPromises;
};

class UBFAPICONTROLLER_API FDownloadRequestManager
{
public:
	static FDownloadRequestManager* GetInstance();
	
	TFuture<UBF::FLoadDataArrayResult> LoadDataFromURI(const FString& DownloadId, const FString& Path, const FString& Hash, TSharedPtr<ICacheLoader> CacheLoader = nullptr);
	TFuture<UBF::FLoadStringResult> LoadStringFromURI(const FString& DownloadId, const FString& Path, const FString& Hash, TSharedPtr<ICacheLoader> CacheLoader = nullptr);
private:
	void PrintLog(const FString& DownloadId, const FString& Path);
	
	TMap<FString, int> RequestCountMap;
	TMap<FString, FActiveRequestData> ActiveRequestPaths;
	FCriticalSection ActiveRequestPathsLock;  // Mutex for thread safety

	static FDownloadRequestManager DownloadRequestManager;
};

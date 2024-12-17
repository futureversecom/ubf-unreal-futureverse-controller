#pragma once

#include "GraphProvider.h"
#include "ICacheLoader.h"

class UBFAPICONTROLLER_API FDownloadRequestManager
{
public:
	static FDownloadRequestManager* GetInstance();
	
	TFuture<UBF::FLoadDataArrayResult> LoadDataFromURI(const FString& DownloadId, const FString& Path, const FString& Hash, ICacheLoader* CacheLoader = nullptr);
	TFuture<UBF::FLoadStringResult> LoadStringFromURI(const FString& DownloadId, const FString& Path, const FString& Hash, ICacheLoader* CacheLoader = nullptr);
private:
	void PrintLog(const FString& DownloadId, const FString& Path);
	
	TMap<FString, int> RequestCountMap;
	TSet<FString> ActiveRequestPaths;

	static FDownloadRequestManager DownloadRequestManager;
};

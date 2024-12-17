#include "DownloadRequestManager.h"

#include "APIUtils.h"

FDownloadRequestManager FDownloadRequestManager::DownloadRequestManager;

static bool bLogDownloadRequests = false;
static TAutoConsoleVariable<bool> CVarLogDownloadRequests(
TEXT("UBFAPIController.Logging.LogDownloadRequests"),
bLogDownloadRequests,
TEXT("Enable verbose debug logging for DownloadRequestManager."));

FDownloadRequestManager* FDownloadRequestManager::GetInstance()
{
	return &DownloadRequestManager;
}

TFuture<UBF::FLoadDataArrayResult> FDownloadRequestManager::LoadDataFromURI(const FString& DownloadId,
                                                                            const FString& Path, const FString& Hash, ICacheLoader* CacheLoader)
{
	TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShareable(new TPromise<UBF::FLoadDataArrayResult>());
	TFuture<UBF::FLoadDataArrayResult> Future = Promise->GetFuture();

	if (ActiveRequestPaths.Contains(Path))
	{
		UE_LOG(LogUBFAPIController, Warning, TEXT("Duplicate path requests found for Path: %s DownloadId: %s"), *Path, *DownloadId);
	}

	RequestCountMap.FindOrAdd(DownloadId) += 1;
	ActiveRequestPaths.Add(Path);
	PrintLog(DownloadId, Path);
	
	APIUtils::LoadDataFromURI(Path, Hash, CacheLoader).Next([Promise, Path, DownloadId, this](const UBF::FLoadDataArrayResult& Result)
	{
		ActiveRequestPaths.Remove(Path);
		RequestCountMap.FindOrAdd(DownloadId) -= 1;
		Promise->SetValue(Result);
	});

	return Future;
}

TFuture<UBF::FLoadStringResult> FDownloadRequestManager::LoadStringFromURI(const FString& DownloadId,
                                                                           const FString& Path, const FString& Hash, ICacheLoader* CacheLoader)
{
	TSharedPtr<TPromise<UBF::FLoadStringResult>> Promise = MakeShareable(new TPromise<UBF::FLoadStringResult>());
	TFuture<UBF::FLoadStringResult> Future = Promise->GetFuture();

	if (ActiveRequestPaths.Contains(Path))
	{
		UE_LOG(LogUBFAPIController, Warning, TEXT("Duplicate path requests found for Path: %s DownloadId: %s"), *Path, *DownloadId);
	}

	RequestCountMap.FindOrAdd(DownloadId) += 1;
	ActiveRequestPaths.Add(Path);
	PrintLog(DownloadId, Path);
	
	APIUtils::LoadStringFromURI(Path, Hash, CacheLoader).Next([Promise, Path, DownloadId, this](const UBF::FLoadStringResult& Result)
	{
		ActiveRequestPaths.Remove(Path);
		RequestCountMap.FindOrAdd(DownloadId) -= 1;
		Promise->SetValue(Result);
	});

	return Future;
}

void FDownloadRequestManager::PrintLog(const FString& DownloadId, const FString& Path)
{
	if (CVarLogDownloadRequests.GetValueOnAnyThread())
		UE_LOG(LogUBFAPIController, Log, TEXT("[DownloadRequestManager] [Stats] DownloadId: %s Num: %d | Path: %s"), *DownloadId, RequestCountMap.FindOrAdd(DownloadId), *Path);
}

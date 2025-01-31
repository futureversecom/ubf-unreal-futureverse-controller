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


TFuture<UBF::FLoadDataArrayResult> FDownloadRequestManager::LoadDataFromURI(
	const FString& DownloadId, 
	const FString& Path, 
	const FString& Hash, 
	TSharedPtr<ICacheLoader> CacheLoader)
{
	TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShareable(new TPromise<UBF::FLoadDataArrayResult>());
	TFuture<UBF::FLoadDataArrayResult> Future = Promise->GetFuture();

	{
		FScopeLock Lock(&ActiveRequestPathsLock);  // Lock starts here

		if (ActiveRequestPaths.Contains(Path))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("Duplicate path requests found for Path: %s DownloadId: %s. Returning existing future"), *Path, *DownloadId);
			ActiveRequestPaths[Path].DataPromises.Add(Promise);
			return Future;
		}

		RequestCountMap.FindOrAdd(DownloadId) += 1;
		ActiveRequestPaths.Add(Path, Promise);
	}  // Lock releases here

	PrintLog(DownloadId, Path);

	APIUtils::LoadDataFromURI(Path, Hash, CacheLoader).Next([Path, DownloadId, this](const UBF::FLoadDataArrayResult& Result)
	{
		FScopeLock Lock(&ActiveRequestPathsLock);  // Lock starts here

		RequestCountMap.FindOrAdd(DownloadId) -= 1;

		if (ActiveRequestPaths.Contains(Path))
		{
			TArray<TSharedPtr<TPromise<UBF::FLoadDataArrayResult>>> DataPromises = ActiveRequestPaths[Path].DataPromises;
			ActiveRequestPaths.Remove(Path);
			
			for (const auto& Promise : DataPromises)
			{
				Promise->SetValue(Result);
			}
		}  // Lock releases here
	});

	return Future;
}

TFuture<UBF::FLoadStringResult> FDownloadRequestManager::LoadStringFromURI(const FString& DownloadId,
                                                                           const FString& Path, const FString& Hash, TSharedPtr<ICacheLoader> CacheLoader)
{
	TSharedPtr<TPromise<UBF::FLoadStringResult>> Promise = MakeShareable(new TPromise<UBF::FLoadStringResult>());
	TFuture<UBF::FLoadStringResult> Future = Promise->GetFuture();

	{
		FScopeLock Lock(&ActiveRequestPathsLock);  // Lock starts here

		if (ActiveRequestPaths.Contains(Path))
		{
			UE_LOG(LogUBFAPIController, Verbose, TEXT("Duplicate path requests found for Path: %s DownloadId: %s. Returning existing future"), *Path, *DownloadId);
			ActiveRequestPaths[Path].StringPromises.Add(Promise);
			return Future;
		}

		RequestCountMap.FindOrAdd(DownloadId) += 1;
		ActiveRequestPaths.Add(Path, Promise);
	}  // Lock releases here

	PrintLog(DownloadId, Path);

	APIUtils::LoadStringFromURI(Path, Hash, CacheLoader).Next([Path, DownloadId, this](const UBF::FLoadStringResult& Result)
	{
		FScopeLock Lock(&ActiveRequestPathsLock);  // Lock starts here

		RequestCountMap.FindOrAdd(DownloadId) -= 1;

		if (ActiveRequestPaths.Contains(Path))
		{
			TArray<TSharedPtr<TPromise<UBF::FLoadStringResult>>> DataPromises = ActiveRequestPaths[Path].StringPromises;
			ActiveRequestPaths.Remove(Path);
			
			for (const auto& Promise : DataPromises)
			{
				Promise->SetValue(Result);
			}
		}  // Lock releases here
	});

	return Future;
}

void FDownloadRequestManager::PrintLog(const FString& DownloadId, const FString& Path)
{
	if (CVarLogDownloadRequests.GetValueOnAnyThread())
		UE_LOG(LogUBFAPIController, Log, TEXT("[DownloadRequestManager] [Stats] DownloadId: %s Num: %d | Path: %s"), *DownloadId, RequestCountMap.FindOrAdd(DownloadId), *Path);
}

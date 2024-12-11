#include "LoadMultipleAssetProfilesAction.h"

TFuture<bool> FLoadMultipleAssetProfilesAction::TryLoadAssetProfiles(const TArray<FString>& ContractIds,
	const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetProfilesAction> SharedThis = AsShared();
	
	for (const FString& Id : ContractIds)
	{
		TSharedPtr<FLoadAssetProfilesAction> LoadAssetProfilesAction = MakeShared<FLoadAssetProfilesAction>();
		LoadAssetProfilesActions.Add(LoadAssetProfilesAction);

		AddPendingLoad();
		LoadAssetProfilesAction->TryLoadAssetProfile(Id, MemoryCacheLoader, TempCacheLoader)
		.Next([SharedThis](bool bSuccess)
		{
			SharedThis->OnCompleteLoad();
		});
	}

	CheckAllLoadingComplete();

	return Future;
}

void FLoadMultipleAssetProfilesAction::OnCompleteLoad()
{
	{
		FScopeLock Lock(&CriticalSection);
		PendingLoads--;
	}

	CheckAllLoadingComplete();
}

void FLoadMultipleAssetProfilesAction::CheckAllLoadingComplete()
{
	bool bShouldSetValue = false;

	// Lock the critical section to safely read PendingLoads
	{
		FScopeLock Lock(&CriticalSection);
		if (PendingLoads <= 0)
		{
			bShouldSetValue = true;
		}
	}

	// Set the value outside the lock to avoid deadlocks
	if (bShouldSetValue)
	{
		Promise->SetValue(true);
	}
}

void FLoadMultipleAssetProfilesAction::AddPendingLoad()
{
	FScopeLock Lock(&CriticalSection);
	PendingLoads++;
}

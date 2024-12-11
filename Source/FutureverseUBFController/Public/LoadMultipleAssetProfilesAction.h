#pragma once
#include "LoadAssetProfilesAction.h"

class FTempCacheLoader;
class FMemoryCacheLoader;

class FLoadMultipleAssetProfilesAction : public TSharedFromThis<FLoadMultipleAssetProfilesAction>
{
public:
	FLoadMultipleAssetProfilesAction() {}
	TFuture<bool> TryLoadAssetProfiles(const TArray<FString>& ContractIds, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader);

	TArray<TSharedPtr<FLoadAssetProfilesAction>> LoadAssetProfilesActions;
private:
	void OnCompleteLoad();
	void CheckAllLoadingComplete();
	void AddPendingLoad();
	
	int PendingLoads = 0;
	FCriticalSection CriticalSection;
	TSharedPtr<TPromise<bool>> Promise;
};

#include "LoadAssetProfileDatasAction.h"

#include "LoadAssetProfileDataAction.h"

TFuture<bool> FLoadAssetProfileDatasAction::TryLoadAssetProfileDatas(const TArray<FAssetProfile>& AssetProfiles,
	const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadAssetProfileDatasAction> SharedThis = AsShared();

	bBlockCompletion = true;
	for (const FAssetProfile& AssetProfile : AssetProfiles)
	{
		AddPendingLoad();

		TSharedPtr<FLoadAssetProfileDataAction> LoadAction = MakeShared<FLoadAssetProfileDataAction>();
		
		LoadAction->TryLoadAssetProfileData(AssetProfile, MemoryCacheLoader, TempCacheLoader).Next([SharedThis](bool bSuccess)
		{
			if (!bSuccess)
				SharedThis->bFailure = true;
			
			SharedThis->CompletePendingLoad();
		});

		ProfileLoadActions.Add(LoadAction);
	}
	bBlockCompletion = false;

	CheckPendingLoadsComplete();

	return Future;
}

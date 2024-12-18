#include "LoadMultipleAssetDatasAction.h"

TFuture<bool> FLoadMultipleAssetDatasAction::TryLoadAssetProfiles(const TArray<FFutureverseAssetLoadData>& AssetLoadDatas, UFutureverseUBFControllerSubsystem* FutureverseUbfControllerSubsystem)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetDatasAction> SharedThis = AsShared();
	
	for (const FFutureverseAssetLoadData& AssetLoadData : AssetLoadDatas)
	{
		AddPendingLoad();
		FutureverseUbfControllerSubsystem->TryLoadAssetData(AssetLoadData).Next([SharedThis](bool bSuccess)
		{
			if (!bSuccess)
				SharedThis->bFailure = true;
			
			SharedThis->CompletePendingLoad();
		});
	}

	CheckPendingLoadsComplete();

	return Future;
}



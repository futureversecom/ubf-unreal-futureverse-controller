#include "LoadMultipleAssetDatasAction.h"

#include "FutureverseUBFControllerSubsystem.h"

TFuture<bool> FLoadMultipleAssetDatasAction::TryLoadAssetProfiles(const TArray<FFutureverseAssetLoadData>& AssetLoadDatas, UFutureverseUBFControllerSubsystem* FutureverseUbfControllerSubsystem)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetDatasAction> SharedThis = AsShared();

	bBlockCompletion = true;
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
	bBlockCompletion = false;

	CheckPendingLoadsComplete();

	return Future;
}



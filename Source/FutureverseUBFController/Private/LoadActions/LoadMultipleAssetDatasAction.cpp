// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadMultipleAssetDatasAction.h"
#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerSubsystem.h"

TFuture<bool> FLoadMultipleAssetDatasAction::TryLoadMultipleAssetDatasAction(const TArray<FFutureverseAssetLoadData>& AssetLoadDatas, UFutureverseUBFControllerSubsystem* FutureverseUbfControllerSubsystem)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetDatasAction> SharedThis = AsShared();

	bBlockCompletion = true;
	for (const FFutureverseAssetLoadData& AssetLoadData : AssetLoadDatas)
	{
		AddPendingLoad();
		FutureverseUbfControllerSubsystem->EnsureAssetDataLoaded(AssetLoadData).Next([SharedThis](bool bSuccess)
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



// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadMultipleAssetDatasAction.h"
#include "FutureverseAssetLoadData.h"
#include "AssetProfile/AssetProfileRegistrySubsystem.h"

TFuture<bool> FLoadMultipleAssetDatasAction::TryLoadMultipleAssetDatasAction(const TArray<FFutureverseAssetLoadData>& AssetLoadDatas,
	UAssetProfileRegistrySubsystem* AssetProfileRegistrySubsystem)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadMultipleAssetDatasAction> SharedThis = AsShared();

	bBlockCompletion = true;
	for (const FFutureverseAssetLoadData& AssetLoadData : AssetLoadDatas)
	{
		AddPendingLoad();
		AssetProfileRegistrySubsystem->GetAssetProfile(AssetLoadData).Next([SharedThis]
			(const FLoadAssetProfileResult& Result)
		{
			if (!Result.bSuccess)
				SharedThis->bFailure = true;
			
			SharedThis->CompletePendingLoad();
		});
	}
	bBlockCompletion = false;

	CheckPendingLoadsComplete();

	return Future;
}



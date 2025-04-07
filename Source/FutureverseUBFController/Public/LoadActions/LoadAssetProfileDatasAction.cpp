// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadAssetProfileDatasAction.h"

#include "LoadAssetProfileDataAction.h"

TFuture<bool> FLoadAssetProfileDatasAction::TryLoadAssetProfileDatas(const TArray<FAssetProfile>& AssetProfiles,
	const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	TSharedPtr<FLoadAssetProfileDatasAction> SharedThis = AsShared();

	bBlockCompletion = true;
	for (const FAssetProfile& AssetProfile : AssetProfiles)
	{
		AddPendingLoad();

		TSharedPtr<FLoadAssetProfileDataAction> LoadAction = MakeShared<FLoadAssetProfileDataAction>();
		
		LoadAction->TryLoadAssetProfileData(AssetProfile, MemoryCacheLoader).Next([SharedThis](bool bSuccess)
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

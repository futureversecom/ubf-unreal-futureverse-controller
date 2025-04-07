// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadAssetProfilesAction.h"

#include <FutureverseUBFControllerSettings.h>

#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "GlobalArtifactProvider/DownloadRequestManager.h"

TFuture<bool> FLoadAssetProfilesAction::TryLoadAssetProfile(const FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	FString ProfileRemotePath;
	const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
	check(Settings);
	if (Settings)
	{
		FString AssetNameFormatted = LoadData.GetAssetName().ToLower().Replace(TEXT("-"), TEXT(""));
		
		ProfileRemotePath = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
			FString::Printf(TEXT("profiles_%s.json"), *LoadData.ContractID));
		
		ProfileRemotePath = ProfileRemotePath.Replace(TEXT(" "), TEXT(""));
	}
	else
	{
		UE_LOG(LogFutureverseUBFController, Error, TEXT("FLoadAssetProfilesAction::TryLoadAssetProfile  UFutureverseUBFControllerSettings was null cannot fetch asset profile"));
		Promise->SetValue(false);
	}

	TSharedPtr<FLoadAssetProfilesAction> SharedThis = AsShared();
	// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath, "", MemoryCacheLoader).Next(
		[SharedThis, ProfileRemotePath, LoadData](const UBF::FLoadStringResult& AssetProfileResult)
	{
		if(!AssetProfileResult.Result.Key)
		{
			UE_LOG(LogFutureverseUBFController, Error, TEXT("UFutureverseUBFControllerSubsystem::LoadRemoteAssetProfile failed to load remote AssetProfile from %s"), *ProfileRemotePath);
			SharedThis->Promise->SetValue(false);
			return;
		}
			
		TArray<FAssetProfile> AssetProfileEntries;
		AssetProfileUtils::ParseAssetProfileJson(AssetProfileResult.Result.Value, AssetProfileEntries);
			
		for (FAssetProfile& AssetProfile : AssetProfileEntries)
		{
			// no need to provide base path here as the values are remote not local
			AssetProfile.RelativePath = "";
			
			if (AssetProfile.Id.IsEmpty())
				AssetProfile.Id = LoadData.AssetID;
			
			if (!AssetProfile.Id.Contains(LoadData.ContractID))
				AssetProfile.Id = FString::Printf(TEXT("%s:%s"), *LoadData.ContractID, *AssetProfile.Id);
			
			SharedThis->AssetProfiles.Add(AssetProfile.Id, AssetProfile);
		};

		SharedThis->Promise->SetValue(true);
	});
	
	return Future;
}

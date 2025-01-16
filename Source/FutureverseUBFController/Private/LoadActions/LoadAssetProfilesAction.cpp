#include "LoadAssetProfilesAction.h"

#include <FutureverseUBFControllerSettings.h>

#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"

TFuture<bool> FLoadAssetProfilesAction::TryLoadAssetProfile(const FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader)
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
			FString::Printf(TEXT("%s_%s_profile.json"), *LoadData.ContractID, *AssetNameFormatted));
		
		ProfileRemotePath = ProfileRemotePath.Replace(TEXT(" "), TEXT(""));
	}
	else
	{
		Promise->SetValue(false);
	}

	TSharedPtr<FLoadAssetProfilesAction> SharedThis = AsShared();
	// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath, "", MemoryCacheLoader.Get()).Next(
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
			SharedThis->AssetProfiles.Add(AssetProfile.Id, AssetProfile);
		};

		SharedThis->Promise->SetValue(true);
	});
	
	return Future;
}

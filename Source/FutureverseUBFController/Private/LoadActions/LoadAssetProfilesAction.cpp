#include "LoadAssetProfilesAction.h"

#include <FutureverseUBFControllerSettings.h>

#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "ControllerLayers/TempCacheLoader.h"

TFuture<bool> FLoadAssetProfilesAction::TryLoadAssetProfile(const FString& ContractId, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader, const TSharedPtr<FTempCacheLoader>& TempCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	FString ProfileRemotePath;
	const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
	check(Settings);
	if (Settings)
	{
		ProfileRemotePath = FString::Printf(TEXT("%s/profiles_%s.json"), *Settings->GetDefaultAssetProfilePath(), *ContractId);
	}
	else
	{
		Promise->SetValue(false);
	}

	TSharedPtr<FLoadAssetProfilesAction> SharedThis = AsShared();
	// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath, "", MemoryCacheLoader.Get()).Next(
		[SharedThis, ProfileRemotePath](const UBF::FLoadStringResult& AssetProfileResult)
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
			SharedThis->AssetProfiles.Add(AssetProfile.Id, AssetProfile);
		};

		SharedThis->Promise->SetValue(true);
	});
	
	return Future;
}

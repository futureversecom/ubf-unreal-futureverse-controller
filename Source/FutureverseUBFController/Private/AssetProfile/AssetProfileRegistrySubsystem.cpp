// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "AssetProfile/AssetProfileRegistrySubsystem.h"

#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "GlobalArtifactProvider/DownloadRequestManager.h"
#include "Kismet/GameplayStatics.h"

UAssetProfileRegistrySubsystem* UAssetProfileRegistrySubsystem::Get(const UObject* WorldContext)
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GameInstance->GetSubsystem<UAssetProfileRegistrySubsystem>();
	}

	return nullptr;
}

TFuture<FLoadAssetProfileResult> UAssetProfileRegistrySubsystem::GetAssetProfile(const FFutureverseAssetLoadData& LoadData)
{
	TSharedPtr<TPromise<FLoadAssetProfileResult>> Promise = MakeShareable(new TPromise<FLoadAssetProfileResult>());
	TFuture<FLoadAssetProfileResult> Future = Promise->GetFuture();

	if (AssetProfiles.Contains(LoadData.AssetID))
	{
		auto Result = FLoadAssetProfileResult();
		Result.SetResult(AssetProfiles.Get(LoadData.AssetID));
		Promise->SetValue(Result);
		return Future;
	}
	
	FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), LoadData.ProfileURI).Next(
	[this, Promise, LoadData] (const UBF::FLoadStringResult& AssetProfileResult)
	{
		auto Result = FLoadAssetProfileResult();
				
		if (!AssetProfileResult.bSuccess)
		{
			UE_LOG(LogFutureverseUBFController, Error, TEXT("UAssetProfileRegistrySubsystem::GetAssetProfile failed to load remote AssetProfile from URI '%s'"), *LoadData.ProfileURI);
			Result.SetFailure();
			Promise->SetValue(Result);
			return;
		}
					
		TArray<FAssetProfile> AssetProfileEntries;
		AssetProfileUtils::ParseAssetProfileJson(AssetProfileResult.Value, AssetProfileEntries);
					
		for (FAssetProfile& AssetProfile : AssetProfileEntries)
		{
			// no need to provide base path here as the values are remote not local
			AssetProfile.OverrideRelativePaths("");
			
			// when parsing a single profile, it will have an empty id
			if (AssetProfile.GetId().IsEmpty())
				AssetProfile.ModifyId(LoadData.AssetID);

			// when parsing multiple profiles, it will have a token Id
			if (!AssetProfile.GetId().Contains(LoadData.GetContractID()))
				AssetProfile.ModifyId(FString::Printf(TEXT("%s:%s"), *LoadData.GetCollectionID(), *AssetProfile.GetId()));
			
			AssetProfiles.Add(AssetProfile.GetId(), AssetProfile);
			UE_LOG(LogFutureverseUBFController, VeryVerbose, TEXT("UAssetProfileRegistrySubsystem::GetAssetProfile AssetId %s AssetProfile %s loaded."), *AssetProfile.GetId(), *AssetProfile.ToString());
		}
			
		Result.SetResult(AssetProfiles.Get(LoadData.AssetID));
		Promise->SetValue(Result);
	});
	
	return Future;
	
}

bool UAssetProfileRegistrySubsystem::IsSubsystemValid() const
{
	return IsValid(this) && bIsInitialized;
}

void UAssetProfileRegistrySubsystem::Deinitialize()
{
	Super::Deinitialize();

	bIsInitialized = false;
}

void UAssetProfileRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	bIsInitialized = true;
}

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetIdMap.h"
#include "GraphProvider.h"
#include "ControllerLayers/AssetProfile.h"
#include "UObject/Object.h"
#include "AssetProfileRegistrySubsystem.generated.h"

struct FFutureverseAssetLoadData;

struct FLoadAssetProfileResult final : UBF::TLoadResult<FAssetProfile> {};
struct FLoadLinkedAssetProfilesResult final : UBF::TLoadResult<TAssetIdMap<FAssetProfile>> {};

/**
 * 
 */
UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UAssetProfileRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UAssetProfileRegistrySubsystem* Get(const UObject* WorldContext);
	
	TFuture<FLoadAssetProfileResult> GetAssetProfile(const FFutureverseAssetLoadData& LoadData);
	TFuture<FLoadLinkedAssetProfilesResult> GetLinkedAssetProfiles(const TArray<FFutureverseAssetLoadData>& LoadDatas);

	bool IsSubsystemValid() const;
	
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	TAssetIdMap<FAssetProfile> AssetProfiles;

	bool bIsInitialized = false;
};

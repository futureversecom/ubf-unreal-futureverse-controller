// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetIdMap.h"
#include "GraphProvider.h"
#include "ControllerLayers/AssetProfile.h"
#include "UObject/Object.h"
#include "AssetProfileRegistrySubsystem.generated.h"

struct FFutureverseAssetLoadData;

template<typename T>
struct FUTUREVERSEUBFCONTROLLER_API TAssetProfileLoadResult
{
	bool bSuccess = false;
	T Value;

	void SetResult(const T& InValue)
	{
		bSuccess = true;
		Value = InValue;
	}

	void SetFailure()
	{
		bSuccess = false;
	}
};

struct FLoadAssetProfileResult final : TAssetProfileLoadResult<FAssetProfile> {};
struct FLoadLinkedAssetProfilesResult final : TAssetProfileLoadResult<TAssetIdMap<FAssetProfile>> {};

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

	bool IsSubsystemValid() const;
	
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	TAssetIdMap<FAssetProfile> AssetProfiles;

	bool bIsInitialized = false;
};

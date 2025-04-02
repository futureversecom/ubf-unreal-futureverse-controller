// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CollectionTestWidget.h"

#include "CollectionTestInputBindingObject.h"
#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerSettings.h"
#include "FutureverseUBFControllerSubsystem.h"
#include "UBFAssetTestLog.h"
#include "ControllerLayers/APIGraphProvider.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "ControllerLayers/DownloadRequestManager.h"
#include "ControllerLayers/MemoryCacheLoader.h"
#include "TestData/CollectionTestData.h"

void UCollectionTestWidget::InitializeWidget()
{
	MemoryCacheLoader = MakeShared<FMemoryCacheLoader>();
}

void UCollectionTestWidget::LoadAllTestAssets(const UCollectionTestData* TestData, const FOnLoadCompleted& OnLoadCompleted)
{
	if (!TestData)
	{
		UE_LOG(LogUBFAssetTest, Warning, TEXT("UCollectionTestWidget::LoadAllTestAssets TestData is null. Returning"));
		return;
	}
	
	const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
	if (!Settings)
	{
		UE_LOG(LogUBFAssetTest, Error, TEXT("UCollectionTestWidget::LoadAllTestAssets UFutureverseUBFControllerSettings was null cannot fetch asset profile"));
		return;
	}
	
	const auto SubSystem = UFutureverseUBFControllerSubsystem::Get(this);
	if (!SubSystem)
	{
		UE_LOG(LogUBFAssetTest, Error, TEXT("UCollectionTestWidget::LoadAllTestAssets UFutureverseUBFControllerSubsystem was null cannot fetch asset profile"));
		return;
	}
	
	int32 NumProfiles = TestData->TestAssetDefinitions.Num();
	NumberOfDownloadedProfiles = 0;
	
	UE_LOG(LogUBFAssetTest, Verbose, TEXT("UCollectionTestWidget::LoadAllTestAssets attempting to download %d profiles"), NumProfiles);
	
	for (auto TestAssetDefinition : TestData->TestAssetDefinitions)
	{
		const auto ContractID = TestAssetDefinition.ContractID;
		FString ProfileRemotePath = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
			FString::Printf(TEXT("profiles_%s.json"), *TestAssetDefinition.ContractID));
		
		ProfileRemotePath = ProfileRemotePath.Replace(TEXT(" "), TEXT(""));
			
		// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath, "", MemoryCacheLoader).Next(
			[this, ProfileRemotePath, NumProfiles, SubSystem, ContractID, OnLoadCompleted](const UBF::FLoadStringResult& AssetProfileResult)
		{
			NumberOfDownloadedProfiles++;
				
			if (!AssetProfileResult.Result.Key)
			{
				UE_LOG(LogUBFAssetTest, Error, TEXT("UCollectionTestWidget::LoadAllTestAssets "
					"failed to load remote AssetProfile from %s "), *ProfileRemotePath);

				if (NumberOfDownloadedProfiles >= NumProfiles)
				{
					OnLoadCompleted.ExecuteIfBound();
				}
				return;
			}
			
			TArray<FAssetProfile> AssetProfileEntries;
			AssetProfileUtils::ParseAssetProfileJson(AssetProfileResult.Result.Value, AssetProfileEntries);

			TArray<FFutureverseAssetLoadData> AssetLoadDatas;
			for (FAssetProfile& AssetProfile : AssetProfileEntries)
			{
				// no need to provide base path here as the values are remote not local
				AssetProfile.RelativePath = "";
			
				if (AssetProfile.Id.IsEmpty())
					AssetProfile.Id = "override";
			
				SubSystem->RegisterAssetProfile(AssetProfile);
			
				AssetLoadDatas.Add(FFutureverseAssetLoadData(AssetProfile.Id, ContractID));

				UUBFInventoryItem* UBFItem = NewObject<UUBFInventoryItem>(this);
				FUBFItemData ItemData;
				ItemData.AssetID = AssetProfile.Id;
				ItemData.ContractID = ContractID;
				UBFItem->SetItemData(ItemData);
				
				TestAssetInventory.Add(UBFItem);
				UE_LOG(LogUBFAssetTest, Verbose, TEXT("UCollectionTestWidget::LoadAllTestAssets Registering AssetProfile for %s and created UBFItem"), *AssetProfile.Id);
			}
		
			auto NumberOfTestAssets = AssetLoadDatas.Num();
			SubSystem->TryLoadAssetDatas(AssetLoadDatas).Next([this, NumberOfTestAssets, OnLoadCompleted](const bool bAssetsLoaded)
			{
				if (!bAssetsLoaded)
				{
					UE_LOG(LogUBFAssetTest, Warning, TEXT("UCollectionTestWidget::LoadAllTestAssets Failed to load Test Assets"));
				}

				UE_LOG(LogUBFAssetTest, Log, TEXT("UCollectionTestWidget::LoadAllTestAssets Successfully loaded %d Test Assets"), NumberOfTestAssets);
			});
				
			if (NumberOfDownloadedProfiles >= NumProfiles)
			{
				OnLoadCompleted.ExecuteIfBound();
			}
		});
	}
}

UUBFInventoryItem* UCollectionTestWidget::GetItemForAsset(const FString& AssetID)
{
	for (const auto TestAssetItem : TestAssetInventory)
	{
		if (TestAssetItem->GetAssetID() == AssetID)
		{
			return TestAssetItem;
		}
	}

	return nullptr;
}

TArray<FUBFContextTreeData> UCollectionTestWidget::MakeContextTree(const FString& RootAssetID,
	const TArray<UCollectionTestInputBindingObject*>& Inputs)
{
	TArray<FUBFContextTreeRelationshipData> Relationships;
	for (const UCollectionTestInputBindingObject* Input : Inputs)
	{
		Relationships.Add(FUBFContextTreeRelationshipData(Input->GetId(), Input->GetValue()));
	}

	const TArray ContextTree {FUBFContextTreeData(RootAssetID, Relationships)};
	return ContextTree;
}

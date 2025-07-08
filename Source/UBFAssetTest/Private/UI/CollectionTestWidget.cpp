// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CollectionTestWidget.h"

#include "CollectionTestInputBindingObject.h"
#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerSettings.h"
#include "FutureverseUBFControllerSubsystem.h"
#include "UBFAssetTestLog.h"
#include "AssetProfile/AssetProfileRegistrySubsystem.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "GlobalArtifactProvider/DownloadRequestManager.h"
#include "TestData/CollectionTestData.h"

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
	const auto CollectionID = TestData->CollectionID;
	for (auto TestAssetDefinition : TestData->TestAssetDefinitions)
	{
		const auto ContractID = TestAssetDefinition.ContractID;

		// todo: use query from AR SDK
		FString ProfileRemotePath = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
			FString::Printf(TEXT("%s.json"), *TestAssetDefinition.ContractID));
		
		ProfileRemotePath = ProfileRemotePath.Replace(TEXT(" "), TEXT(""));
			
		// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath).Next(
			[this, ProfileRemotePath, NumProfiles, SubSystem, ContractID, CollectionID, OnLoadCompleted](const UBF::FLoadStringResult& AssetProfileResult)
		{
			NumberOfDownloadedProfiles++;
				
			if (!AssetProfileResult.bSuccess)
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
			AssetProfileUtils::ParseAssetProfileJson(AssetProfileResult.Value, AssetProfileEntries);

			TArray<FFutureverseAssetLoadData> AssetLoadDatas;
			for (FAssetProfile& AssetProfile : AssetProfileEntries)
			{
				if (AssetProfile.GetId().IsEmpty())
					AssetProfile.ModifyId(ContractID);
			
				if (!AssetProfile.GetId().Contains(ContractID))
					AssetProfile.ModifyId(FString::Printf(TEXT("%s:%s"), *ContractID, *AssetProfile.GetId()));

				// TODO fix this mike
				//SubSystem->RegisterAssetProfile(AssetProfile);

				// TODO add back ProfileURI
				AssetLoadDatas.Add(FFutureverseAssetLoadData(AssetProfile.GetId(), TEXT("")));

				UUBFItem* UBFItem = NewObject<UUBFItem>(this);
				FUBFItemData ItemData;
				ItemData.AssetID = AssetProfile.GetId();
				ItemData.ContractID = ContractID;
				ItemData.CollectionID = FString::Printf(TEXT("%s:%s"), *CollectionID, *ContractID);
				UBFItem->SetItemData(ItemData);
				
				TestAssetInventory.Add(UBFItem);
				UE_LOG(LogUBFAssetTest, Verbose, TEXT("UCollectionTestWidget::LoadAllTestAssets "
					"Registering AssetProfile for %s and created UBFItem"), *AssetProfile.GetId());
			}
		
			auto NumberOfTestAssets = AssetLoadDatas.Num();
			SubSystem->EnsureAssetDatasLoaded(AssetLoadDatas).Next([this, NumberOfTestAssets, OnLoadCompleted]
				(const FLoadLinkedAssetProfilesResult& Result)
			{
				if (!Result.bSuccess)
				{
					UE_LOG(LogUBFAssetTest, Warning, TEXT("UCollectionTestWidget::LoadAllTestAssets "
						"Failed to load Test Assets"));
				}

				UE_LOG(LogUBFAssetTest, Log, TEXT("UCollectionTestWidget::LoadAllTestAssets "
					"Successfully loaded %d Test Assets"), NumberOfTestAssets);
			});
				
			if (NumberOfDownloadedProfiles >= NumProfiles)
			{
				OnLoadCompleted.ExecuteIfBound();
			}
		});
	}
}

UUBFItem* UCollectionTestWidget::GetItemForAsset(const FString& AssetID)
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
		// TODO add back profile URI
		Relationships.Add(FUBFContextTreeRelationshipData(Input->GetId(), Input->GetValue(), TEXT("")));
	}

	const TArray ContextTree {FUBFContextTreeData(RootAssetID, Relationships)};
	return ContextTree;
}

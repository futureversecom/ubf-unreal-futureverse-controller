// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadActions/LoadAssetProfileDataAction.h"

#include "FutureverseUBFControllerLog.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "GlobalArtifactProvider/DownloadRequestManager.h"

namespace LoadBlueprintInstanceUtil
{
	FString ParseBlueprintInstanceID(const FString& Path)
	{
		FString CleanPath = Path;

		// Check if it's a URL and strip everything before the last '/'
		if (Path.Contains(TEXT("://")))
		{
			int32 LastSlashIndex;
			if (Path.FindLastChar('/', LastSlashIndex))
			{
				CleanPath = Path.Mid(LastSlashIndex + 1);
			}
		}

		// Now use FPaths to clean the filename and get the base filename
		FString FileName = FPaths::GetCleanFilename(CleanPath);
		FString FileNameWithoutExtension = FPaths::GetBaseFilename(FileName);

		return FileNameWithoutExtension;
	}
}

TFuture<bool> FLoadAssetProfileDataAction::TryLoadAssetProfileData(const FAssetProfile& AssetProfile, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	bBlockCompletion = true;

	TSharedPtr<FLoadAssetProfileDataAction> SharedThis = AsShared();
	AssetProfileLoaded = AssetProfile;
	if(!AssetProfile.RenderBlueprintInstanceUri.IsEmpty())
	{
		SharedThis->AddPendingLoad();
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Blueprint"),AssetProfile.GetRenderBlueprintInstanceUri(), AssetProfile.GetRenderBlueprintInstanceUri(), MemoryCacheLoader)
			.Next([SharedThis, AssetProfile, MemoryCacheLoader](const UBF::FLoadStringResult& LoadResult)
		{
			if (!LoadResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render blueprint instance from %s"), *AssetProfile.GetParsingBlueprintInstanceUri());
				SharedThis->CompletePendingLoad();
				return;
			}

			FBlueprintJson BlueprintInstance(LoadBlueprintInstanceUtil::ParseBlueprintInstanceID(AssetProfile.GetRenderBlueprintInstanceUri()), LoadResult.Result.Value);
			SharedThis->AssetData.RenderGraphInstance = BlueprintInstance;

			if(!AssetProfile.RenderCatalogUri.IsEmpty())
			{
				SharedThis->AddPendingLoad();
				FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Catalog"),AssetProfile.GetRenderCatalogUri(), AssetProfile.GetRenderCatalogUri(), MemoryCacheLoader)
					.Next([SharedThis, AssetProfile](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load render catalog from %s"), *AssetProfile.GetRenderCatalogUri());
						SharedThis->CompletePendingLoad();
						return;
					}
				
					TMap<FString, UBF::FCatalogElement> CatalogMap;
					AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, SharedThis->RenderCatalogMap);
					UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding rendering catalog from %s"), *AssetProfile.GetRenderCatalogUri());
					SharedThis->CompletePendingLoad();
				});
			}

			SharedThis->CompletePendingLoad();
		});
	}

	if(!AssetProfile.ParsingBlueprintInstanceUri.IsEmpty())
	{
		SharedThis->AddPendingLoad();
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Blueprint"),AssetProfile.GetParsingBlueprintInstanceUri(), AssetProfile.GetParsingBlueprintInstanceUri(), MemoryCacheLoader)
			.Next([SharedThis, AssetProfile, MemoryCacheLoader](const UBF::FLoadStringResult& LoadResult)
		{
			if (!LoadResult.Result.Key)
			{
				UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing blueprint from %s"), *AssetProfile.GetParsingBlueprintInstanceUri());
				SharedThis->CompletePendingLoad();
				return;
			}

				FBlueprintJson BlueprintInstance(LoadBlueprintInstanceUtil::ParseBlueprintInstanceID(AssetProfile.GetParsingBlueprintInstanceUri()), LoadResult.Result.Value);
			SharedThis->AssetData.ParsingGraphInstance = BlueprintInstance;
				
			if(!AssetProfile.ParsingCatalogUri.IsEmpty())
			{
				SharedThis->AddPendingLoad();
				FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("Catalog"), AssetProfile.GetParsingCatalogUri(), AssetProfile.GetParsingCatalogUri(), MemoryCacheLoader)
					.Next([SharedThis, AssetProfile, BlueprintInstance](const UBF::FLoadStringResult& LoadResult)
				{
					if (!LoadResult.Result.Key)
					{
						UE_LOG(LogFutureverseUBFController, Warning, TEXT("Failed to load parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri());
						SharedThis->CompletePendingLoad();
						return;
					}
						
					AssetProfileUtils::ParseCatalog(LoadResult.Result.Value, SharedThis->ParsingCatalogMap);
					UE_LOG(LogFutureverseUBFController, Verbose, TEXT("Adding parsing catalog from %s"), *AssetProfile.GetParsingCatalogUri());
					SharedThis->CompletePendingLoad();
				
				});
			}

			SharedThis->CompletePendingLoad();
		});
	}

	bBlockCompletion = false;
	CheckPendingLoadsComplete();

	return Future;
}

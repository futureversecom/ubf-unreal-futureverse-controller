// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "LoadAssetProfilesAction.h"

#include <FutureverseUBFControllerSettings.h>

#include "FutureverseAssetLoadData.h"
#include "FutureverseUBFControllerLog.h"
#include "HttpModule.h"
#include "ControllerLayers/AssetProfileUtils.h"
#include "GlobalArtifactProvider/DownloadRequestManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

TFuture<bool> FLoadAssetProfilesAction::TryLoadAssetProfile(const FFutureverseAssetLoadData& LoadData, const TSharedPtr<FMemoryCacheLoader>& MemoryCacheLoader)
{
	Promise = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = Promise->GetFuture();

	FString ProfileRemotePath;
	const UFutureverseUBFControllerSettings* Settings = GetDefault<UFutureverseUBFControllerSettings>();
	check(Settings);
	if (Settings)
	{
		ProfileRemotePath = FPaths::Combine(Settings->GetDefaultAssetProfilePath(),
			FString::Printf(TEXT("%s.json"), *LoadData.GetContractID()));
		
		ProfileRemotePath = ProfileRemotePath.Replace(TEXT(" "), TEXT(""));
	}
	else
	{
		UE_LOG(LogFutureverseUBFController, Error, TEXT("FLoadAssetProfilesAction::TryLoadAssetProfile  UFutureverseUBFControllerSettings was null cannot fetch asset profile"));
		Promise->SetValue(false);
	}

	TSharedPtr<FLoadAssetProfilesAction> SharedThis = AsShared();
	
	const auto HandleURL = [SharedThis, ProfileRemotePath, LoadData](const UBF::FLoadStringResult& AssetProfileResult)
	{
		if (!AssetProfileResult.bSuccess)
		{
			UE_LOG(LogFutureverseUBFController, Error, TEXT("UFutureverseUBFControllerSubsystem::LoadRemoteAssetProfile failed to load remote AssetProfile from %s"), *ProfileRemotePath);
			SharedThis->Promise->SetValue(false);
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
			
			SharedThis->AssetProfiles.Add(AssetProfile.GetId(), AssetProfile);
		};

		SharedThis->Promise->SetValue(true);
	};
	
	if (Settings->GetUseAssetRegisterProfiles())
	{
		GetAssetProfileURLFromAssetRegister(LoadData.GetCollectionID(), LoadData.GetTokenID()).Next(
		[SharedThis, HandleURL, this](const FString& OutURL)
		{
			if (OutURL.IsEmpty())
			{
				SharedThis->Promise->SetValue(false);
			}
			else
			{
				FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), OutURL).Next(HandleURL);
			}
		});
	}
	else
	{
		FDownloadRequestManager::GetInstance()->LoadStringFromURI(TEXT("AssetProfile"), ProfileRemotePath).Next(HandleURL);
	}
	
	return Future;
}

TFuture<FString> FLoadAssetProfilesAction::GetAssetProfileURLFromAssetRegister(
	const FString& CollectionId, const FString& TokenId)
{
	// fetch remote asset profile, then parse and register all the blueprint instances and catalogs
	TSharedPtr<TPromise<FString>> Promise = MakeShareable(new TPromise<FString>());
	TFuture<FString> Future = Promise->GetFuture();

	const TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	
	const FString URL = "https://ar-api.futureverse.app/graphql";
	// const FString URL = "https://ar-api.futureverse.cloud/graphql";

	const FString Content = R"(
	{
		"query": "query($assetIds: [AssetInput!]) { assetsByIds(assetIds: $assetIds) {profiles} }",
		"variables" : 
		{
			"assetIds" : [ 
			{ 
			  "tokenId" : ")" + TokenId + R"(", 
			  "collectionId": ")" + CollectionId + R"(" 
		    } 
		  ]
		}
	})";

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader("content-type", "application/json");
	Request->SetContentAsString(Content);
	Request->SetTimeout(60);
	
	auto RequestCallback = [Promise, TokenId, CollectionId]
	(FHttpRequestPtr Request, const FHttpResponsePtr& Response, bool bWasSuccessful) mutable
	{
		if (Response == nullptr)
		{
			UE_LOG(LogFutureverseUBFController, Error, TEXT("GetAssetProfileURLFromAssetRegister failed to load remote AssetProfile for %s:%s"), *CollectionId, *TokenId);
			Promise->SetValue(TEXT(""));
			return;
		}
		
		if (bWasSuccessful && Response.IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("GetAssetProfileURLFromAssetRegister Response: %s"), *Response->GetContentAsString());
		
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				TSharedPtr<FJsonObject> DataObject = JsonObject->GetObjectField(TEXT("data"));

				const TArray<TSharedPtr<FJsonValue>>* AssetsArray;
				if (DataObject->TryGetArrayField(TEXT("assetsByIds"), AssetsArray) && AssetsArray->Num() > 0)
				{
					// todo: there could be multiple profiles?
					TSharedPtr<FJsonObject> AssetObject = (*AssetsArray)[0]->AsObject();
					if (AssetObject.IsValid())
					{
						TSharedPtr<FJsonObject> ProfilesObject = AssetObject->GetObjectField(TEXT("profiles"));
						FString AssetProfileUrl = ProfilesObject->GetStringField(TEXT("asset-profile"));
						
						Promise->SetValue(AssetProfileUrl);
					}
				}
			}
			else
			{
				UE_LOG(LogFutureverseUBFController, Error, TEXT("GetAssetProfileURLFromAssetRegister Failed to prase ResponseJson: %s"), *Response->GetContentAsString());
				Promise->SetValue(TEXT(""));
			}
		}
		else
		{
			Promise->SetValue(TEXT(""));
		}
	};
	
	Request->OnProcessRequestComplete().BindLambda(RequestCallback);
	Request->ProcessRequest();

	return Future;
}

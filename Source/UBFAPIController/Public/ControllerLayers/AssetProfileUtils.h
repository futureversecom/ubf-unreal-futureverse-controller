// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "APIGraphProvider.h"
#include "UBFAPIControllerLog.h"

namespace AssetProfileUtils
{
	const FString RenderInstance = TEXT("render-instance");
	const FString RenderCatalog = TEXT("render-catalog");
	
	const FString ParsingInstance = TEXT("parsing-instance");
	const FString ParsingCatalog = TEXT("parsing-catalog");
	const FString AssetProfilesName = TEXT("asset-profiles");
	const FString AssetIdName = TEXT("asset-id");
	const FString ProfileName = TEXT("profile");
	
	inline FString JsonObjectToString(const FJsonObject& JsonObject)
	{
		FString OutputString;
		// Create a writer for the JSON string
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);

		// Serialize the FJsonObject to a string
		if (FJsonSerializer::Serialize(MakeShared<FJsonObject>(JsonObject), Writer))
		{
			return OutputString;
		}

		// If serialization fails, return an empty string or an error message
		return TEXT("Failed to serialize JSON object.");
	}
	
	inline void ParseAssetProfileJson(const FString& Json, TArray<FAssetProfile>& AssetProfileEntries)
	{
		// Create a JSON Reader
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		TSharedPtr<FJsonObject> JsonObject;
    
		// Deserialize the JSON string into a JSON object
		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfileUtils::ParseAssetProfileJson Failed to parse JSON string\n %s."), *Json);
			return;
		}

		// Handle case when this is single profile
		if (JsonObject->HasField(RenderInstance))
		{
			// Extract the RenderBlueprintUrl
			FString RenderBlueprintInstanceUri = JsonObject->HasField(RenderInstance)
					? JsonObject->GetStringField(RenderInstance)
					: "";
				
			FString RenderCatalogUri = JsonObject->HasField(RenderCatalog)
					? JsonObject->GetStringField(RenderCatalog)
					: "";
				
			FString ParsingBlueprintInstanceUri = JsonObject->HasField(ParsingInstance)
					? JsonObject->GetStringField(ParsingInstance)
					: "";
				
			FString ParsingCatalogUri = JsonObject->HasField(ParsingCatalog)
					? JsonObject->GetStringField(ParsingCatalog)
					: "";
				
			// Register the graph and catalog locations
			FAssetProfile AssetProfileEntry;
			AssetProfileEntry.Id = "";
			AssetProfileEntry.RenderBlueprintInstanceUri = RenderBlueprintInstanceUri;
			AssetProfileEntry.RenderCatalogUri = RenderCatalogUri;
			AssetProfileEntry.ParsingBlueprintInstanceUri = ParsingBlueprintInstanceUri;
			AssetProfileEntry.ParsingCatalogUri = ParsingCatalogUri;
				
			AssetProfileEntries.Add(AssetProfileEntry);
		}

		if (!JsonObject->HasField(AssetProfilesName)) return;
		
		// Get the "AssetProfiles" array from the JSON object
		TArray<TSharedPtr<FJsonValue>> AssetProfiles = JsonObject->GetArrayField(AssetProfilesName);

		// Iterate over each element in the "AssetProfiles" array
		for (const TSharedPtr<FJsonValue>& Value : AssetProfiles)
		{
			// Get the AssetProfile object
			TSharedPtr<FJsonObject> AssetProfileObject = Value->AsObject();
			
			if (!AssetProfileObject.IsValid())
				continue;

			if (!AssetProfileObject->HasField(AssetIdName))
			{
				UE_LOG(LogUBFAPIController, Warning, TEXT("ParseAssetProfileJson() missing '%s' field. Source Json: \n %s"), *AssetIdName, *Json);
			}

			if (!AssetProfileObject->HasField(ProfileName))
			{
				UE_LOG(LogUBFAPIController, Warning, TEXT("ParseAssetProfileJson() missing '%s' field. Source Json: \n %s"), *ProfileName, *Json);
			}
			
			// Extract the InventoryItemName
			FString AssetId = AssetProfileObject->GetStringField(AssetIdName);

			// Get the nested AssetProfile object
			TSharedPtr<FJsonObject> AssetProfile = AssetProfileObject->GetObjectField(ProfileName);
			
			if (AssetProfile.IsValid())
			{
				if (!AssetProfile->HasField(RenderInstance) || !AssetProfile->HasField(RenderCatalog))
				{
					UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfile json: \n %s \n doesn't have required '%s' or '%s' fields. Source Json: \n %s")
						, *JsonObjectToString(*AssetProfile.Get()), *RenderInstance, *RenderCatalog, *Json);
				}
				
				// Extract the RenderBlueprintUrl
				FString RenderBlueprintInstanceUri = AssetProfile->HasField(RenderInstance)
						? AssetProfile->GetStringField(RenderInstance)
						: "";
				
				FString RenderCatalogUri = AssetProfile->HasField(RenderCatalog)
						? AssetProfile->GetStringField(RenderCatalog)
						: "";
				
				FString ParsingBlueprintInstanceUri = AssetProfile->HasField(ParsingInstance)
						? AssetProfile->GetStringField(ParsingInstance)
						: "";
				
				FString ParsingCatalogUri = AssetProfile->HasField(ParsingCatalog)
						? AssetProfile->GetStringField(ParsingCatalog)
						: "";
				
				// Register the graph and catalog locations
				FAssetProfile AssetProfileEntry;
				AssetProfileEntry.Id = AssetId;
				AssetProfileEntry.RenderBlueprintInstanceUri = RenderBlueprintInstanceUri;
				AssetProfileEntry.RenderCatalogUri = RenderCatalogUri;
				AssetProfileEntry.ParsingBlueprintInstanceUri = ParsingBlueprintInstanceUri;
				AssetProfileEntry.ParsingCatalogUri = ParsingCatalogUri;
				
				AssetProfileEntries.Add(AssetProfileEntry);
			}
		}
	}
	
	inline void ParseCatalog(const FString& Json, TMap<FString, FCatalogElement>& CatalogElementMap)
	{
		// Create a shared pointer to hold the JSON object
		TSharedPtr<FJsonObject> JsonObject;

		// Create a JSON reader
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

		// Deserialize the JSON data into the JsonObject
		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfileUtils::ParseCatalog Failed to parse JSON string\n %s."), *Json);
			return;
		}
		
		// Get the "resources" array from the JSON
		TArray<TSharedPtr<FJsonValue>> ResourcesArray = JsonObject->GetArrayField(TEXT("resources"));

		// Iterate over each element in the resources array
		for (const TSharedPtr<FJsonValue>& Value : ResourcesArray)
		{
			// Each element is a JSON object, get it as such
			TSharedPtr<FJsonObject> ResourceObject = Value->AsObject();
			if (ResourceObject.IsValid())
			{
				if (!ResourceObject->HasField(TEXT("id"))
					|| !ResourceObject->HasField(TEXT("uri"))
					|| !ResourceObject->HasField(TEXT("type"))
					|| !ResourceObject->HasField(TEXT("hash")))
				{
					UE_LOG(LogUBFAPIController, Error, TEXT("Cannot parse Json, missing id, uri, type or hash field in element %s in json %s"), *Value->AsString(), *Json);
					continue;
				}
				FCatalogElement CatalogElement;
				CatalogElement.Id = ResourceObject->GetStringField(TEXT("id"));
				CatalogElement.Type = ResourceObject->GetStringField(TEXT("type"));
				CatalogElement.Uri = ResourceObject->GetStringField(TEXT("uri"));
				CatalogElement.Hash = ResourceObject->GetStringField(TEXT("hash"));
				CatalogElementMap.Add(CatalogElement.Id, CatalogElement);
				UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("AssetProfileUtils::ParseCatalog "
					"Added CatalogElement Id: %s Type: %s Uri: %s hash: %s"),
					*CatalogElement.Id, *CatalogElement.Type, *CatalogElement.Uri, *CatalogElement.Hash);
			}
		}
	}
}

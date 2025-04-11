// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "AssetProfile.h"
#include "Graph.h"
#include "UBFAPIControllerLog.h"
#include "GlobalArtifactProvider/CatalogElement.h"

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
		TSharedPtr<FJsonObject> MainJsonObject;
    
		// Deserialize the JSON string into a JSON object
		if (!FJsonSerializer::Deserialize(Reader, MainJsonObject) || !MainJsonObject.IsValid())
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfileUtils::ParseAssetProfileJson Failed to parse JSON string\n %s."), *Json);
			return;
		}
		
		for (const auto& AssetPair : MainJsonObject->Values)
		{
			// extract the AssetId name
			const FString AssetId = AssetPair.Key; 
	
			TSharedPtr<FJsonObject> AssetJsonObject = AssetPair.Value->AsObject();
			if (!AssetJsonObject.IsValid()) continue;

			TArray<FAssetProfileVariant> Variants;
			
			for (const auto& VariantTuple : AssetJsonObject->Values)
			{
				// extract the versions
				TArray<FString> Versions;
				TSharedPtr<FJsonObject> VariantObject = VariantTuple.Value->AsObject();
				VariantObject->Values.GetKeys(Versions);

				TArray<UBF::FGraphVersion> AssetProfileVersions;
				for (auto VersionString : Versions)
				{
					// only add the supported versions
					const auto AssetProfileVersion = UBF::FGraphVersion(VersionString);
					if (!(AssetProfileVersion >= UBF::MinSupportedGraphVersion && AssetProfileVersion <= UBF::MaxSupportedGraphVersion)) continue;
					AssetProfileVersions.Add(AssetProfileVersion);
				}

				if (AssetProfileVersions.IsEmpty())
				{
					UE_LOG(LogTemp, Warning, TEXT("ParseAssetProfileJson() AssetProfileVersions.IsEmpty() for JSON: %s"), *Json);
					continue;
				}

				// sort it by version and use the latest supported version
				AssetProfileVersions.Sort();
				const auto LatestSupportedVersion = AssetProfileVersions.Last().ToString();
				TSharedPtr<FJsonObject> AssetProfile = VariantObject->Values.Find(LatestSupportedVersion)->Get()->AsObject();
			
				if (!AssetProfile->HasField(RenderInstance) || !AssetProfile->HasField(RenderCatalog))
				{
					UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfile json: \n %s \n doesn't have required '%s' or '%s' fields. Source Json: \n %s")
						, *JsonObjectToString(*AssetProfile), *RenderInstance, *RenderCatalog, *Json);
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

				UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("AssetProfileUtils::ParseAssetProfileJson "
						"Added AssetProfile Id: %s Version: %s"), *AssetId, *LatestSupportedVersion);
						
				// Register the graph and catalog locations
				FString VariantId = VariantTuple.Key;
				FAssetProfileVariant Variant(VariantId, RenderBlueprintInstanceUri, ParsingBlueprintInstanceUri,
					RenderCatalogUri, ParsingCatalogUri);
					
				Variants.Add(Variant);
			}

			FAssetProfile AssetProfileEntry(AssetId, Variants);
			
			AssetProfileEntries.Add(AssetProfileEntry);
		}
	}
	
	inline void ParseCatalog(const FString& Json, TMap<FString, UBF::FCatalogElement>& CatalogElementMap)
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
					|| !ResourceObject->HasField(TEXT("hash")))
				{
					UE_LOG(LogUBFAPIController, Error, TEXT("Cannot parse Json, missing id, uri or hash field in element %s in json %s"), *Value->AsString(), *Json);
					continue;
				}
				UBF::FCatalogElement CatalogElement;
				CatalogElement.Id = ResourceObject->GetStringField(TEXT("id"));
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

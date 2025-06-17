// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "AssetProfile.h"
#include "Graph.h"
#include "UBFAPIControllerLog.h"

namespace AssetProfileUtils
{
	const FString ProfileVersion = TEXT("profile_verison");
	const FString UBFVariants = TEXT("ubf-variants");
	
	const FString RenderInstance = TEXT("render-instance");
	const FString RenderCatalog = TEXT("render-catalog");
	const FString ParsingInstance = TEXT("parsing-instance");
	const FString ParsingCatalog = TEXT("parsing-catalog");
	
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
	
	inline void ParseAssetProfileVariant(const FString& Json, TArray<FAssetProfile>& AssetProfileEntries, const FString& AssetId, TSharedPtr<FJsonObject> AssetJsonObject)
	{
		TSharedPtr<FJsonObject> VariantObjects = AssetJsonObject->GetObjectField(UBFVariants);
		TArray<FAssetProfileVariant> Variants;
			
		for (const auto& VariantTuple : VariantObjects->Values)
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
				UE_LOG(LogUBFAPIController, Warning, TEXT("ParseAssetProfileJson() Failed to find any supported asset profile version for: %s"), *VariantTuple.Key);
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
						"Added AssetProfile Variant: %s Id: %s Version: %s"), *VariantTuple.Key, *AssetId, *LatestSupportedVersion);
						
			// Register the graph and catalog locations
			FString VariantId = VariantTuple.Key;
			FAssetProfileVariant Variant(VariantId, RenderBlueprintInstanceUri, ParsingBlueprintInstanceUri,
										RenderCatalogUri, ParsingCatalogUri);
					
			Variants.Add(Variant);
		}

		FAssetProfile AssetProfileEntry(AssetId, Variants);
			
		AssetProfileEntries.Add(AssetProfileEntry);
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
		
		// Handle case when this is single profile
		if (MainJsonObject->HasField(UBFVariants))
		{
			ParseAssetProfileVariant(Json, AssetProfileEntries, "", MainJsonObject);
			return;
		}
		
		for (const auto& AssetPair : MainJsonObject->Values)
		{
			// extract the AssetId name
			const FString AssetId = AssetPair.Key; 
	
			TSharedPtr<FJsonObject> AssetJsonObject = AssetPair.Value->AsObject();

			if (!AssetJsonObject.IsValid()) continue;
			if (!AssetJsonObject->HasField(UBFVariants))
			{
				UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfile json: %s \n doesn't have required field: %s."),
					*JsonObjectToString(*AssetJsonObject), *UBFVariants);
				continue;
			}
			
			ParseAssetProfileVariant(Json, AssetProfileEntries, AssetId, AssetJsonObject);
		}
	}
}

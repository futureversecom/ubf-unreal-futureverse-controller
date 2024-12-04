#pragma once
#include "APIGraphProvider.h"
#include "UBFAPIControllerLog.h"

namespace AssetProfileUtils
{
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
		
		// Get the "AssetProfiles" array from the JSON object
		TArray<TSharedPtr<FJsonValue>> AssetProfiles = JsonObject->GetArrayField(TEXT("AssetProfiles"));

		// Iterate over each element in the "AssetProfiles" array
		for (const TSharedPtr<FJsonValue>& Value : AssetProfiles)
		{
			// Get the AssetProfile object
			TSharedPtr<FJsonObject> AssetProfileObject = Value->AsObject();
			
			if (!AssetProfileObject.IsValid())
				continue;
			
			// Extract the InventoryItemName
			FString InventoryItemName = AssetProfileObject->GetStringField(TEXT("InventoryItemName"));

			// Get the nested AssetProfile object
			TSharedPtr<FJsonObject> AssetProfile = AssetProfileObject->GetObjectField(TEXT("AssetProfile"));
			
			if (AssetProfile.IsValid())
			{
				// Extract the RenderBlueprintUrl
				FString RenderBlueprintInstanceUri = AssetProfile->GetStringField(TEXT("render-instance"));
				FString RenderCatalogUri = AssetProfile->GetStringField(TEXT("render-catalog"));
				FString ParsingBlueprintInstanceUri = AssetProfile->HasField(TEXT("parsing-instance"))
						? AssetProfile->GetStringField(TEXT("parsing-instance"))
						: "";
				FString ParsingCatalogUri = AssetProfile->HasField(TEXT("parsing-catalog"))
						? AssetProfile->GetStringField(TEXT("parsing-catalog"))
						: "";
				
				// Register the graph and catalog locations
				FAssetProfile AssetProfileEntry;
				AssetProfileEntry.Id = InventoryItemName;
				AssetProfileEntry.RenderBlueprintInstanceUri = RenderBlueprintInstanceUri;
				AssetProfileEntry.RenderCatalogUri = RenderCatalogUri;
				AssetProfileEntry.ParsingBlueprintInstanceUri = ParsingBlueprintInstanceUri;
				AssetProfileEntry.ParsingCatalogUri = ParsingCatalogUri;
				
				AssetProfileEntries.Add(AssetProfileEntry);
			}
		}
	}
	
	inline void ParseCatalog(const FString& Json, TMap<FString, FCatalogElement>& ResourceManifestElementMap)
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
				ResourceManifestElementMap.Add(CatalogElement.Id, CatalogElement);
				UE_LOG(LogUBFAPIController, Verbose, TEXT("AssetProfileUtils::ParseCatalog "
					"Added CatalogElement Id: %s Type: %s Uri: %s hash: %s"),
					*CatalogElement.Id, *CatalogElement.Type, *CatalogElement.Uri, *CatalogElement.Hash);
			}
		}
	}

	inline void ParseBlueprintInstanceJson(const FString& Json, FBlueprintInstance& BlueprintInstance)
	{
		// Create a shared pointer to hold the JSON object
		TSharedPtr<FJsonObject> JsonObject;

		// Create a JSON reader
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

		// Deserialize the JSON data into the JsonObject
		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("AssetProfileUtils::ParseBlueprintInstanceJson Failed to parse JSON string\n %s."), *Json);
			return;
		}
		FString BlueprintID = JsonObject->HasField(TEXT("id"))
				? JsonObject->GetStringField(TEXT("blueprintId"))
				: TEXT("InvalidBlueprintId");
			
		// Get the "bindings" array from the JSON
		TArray<TSharedPtr<FJsonValue>> BindingsArray = JsonObject->GetArrayField(TEXT("bindings"));

		// Iterate over each element in the bindings array
		for (const TSharedPtr<FJsonValue>& Value : BindingsArray)
		{
			// Each element is a JSON object, get it as such
			TSharedPtr<FJsonObject> BindingObject = Value->AsObject();
			if (BindingObject.IsValid())
			{
				if (!BindingObject->HasField(TEXT("id"))
					|| !BindingObject->HasField(TEXT("type"))
					|| !BindingObject->HasField(TEXT("value")))
				{
					UE_LOG(LogUBFAPIController, Error, TEXT("Cannot parse Json, missing id, type or value field in element %s in json %s"), *Value->AsString(), *Json);
					continue;
				}
				FBlueprintInstanceBinding Binding;
				Binding.Id = BindingObject->GetStringField(TEXT("id"));
				Binding.Type = BindingObject->GetStringField(TEXT("type"));
				Binding.Value = BindingObject->GetStringField(TEXT("uri"));
				BlueprintInstance.GetBindingsRef().Add(Binding.Id, Binding);
				UE_LOG(LogUBFAPIController, Verbose, TEXT("AssetProfileUtils::ParseBlueprintInstanceJson "
					"Added FBlueprintInstanceBidning Id: %s Type: %s Value: %s"),
					*Binding.Id, *Binding.Type, *Binding.Value);
			}
		}
	}
}

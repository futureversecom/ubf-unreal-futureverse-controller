#pragma once
#include "FutureverseUBFControllerLog.h"
#include "JsonObjectWrapper.h"

namespace MetadataJsonUtils
{
	inline void LogJsonString(const TSharedPtr<FJsonObject>& JsonObject)
	{
		FString JsonString;

		FJsonObjectWrapper ObjectWrapper;
		ObjectWrapper.JsonObject = JsonObject;
		ObjectWrapper.JsonObjectToString(JsonString);
	
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("Used JsonString: %s"), *JsonString);
	}

	inline FString GetAssetName(const TSharedPtr<FJsonObject>& JsonObject)
	{
		// Check if the main JSON object exists
		if (!JsonObject.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Invalid JsonObject in InventoryItem"));
		}

		// Check if the "node" field exists
		const TSharedPtr<FJsonObject>* NodeObject;
		if (!JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Missing or invalid 'node' field in JsonObject"));
			LogJsonString(JsonObject);
		}

		// Check if the "metadata" field exists
		const TSharedPtr<FJsonObject>* MetadataObject;
		if (!(*NodeObject)->TryGetObjectField(TEXT("metadata"), MetadataObject) || !MetadataObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Missing or invalid 'metadata' field in node object"));
			LogJsonString(JsonObject);
		}

		// Check if the "properties" field exists
		const TSharedPtr<FJsonObject>* PropertiesObject;
		if (!(*MetadataObject)->TryGetObjectField(TEXT("properties"), PropertiesObject) || !PropertiesObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Missing or invalid 'properties' field in metadata object"));
			LogJsonString(JsonObject);
		}

		// Check if the "name" field exists and retrieve its value
		FString AssetName;
		if (!(*PropertiesObject)->TryGetStringField(TEXT("name"), AssetName) || AssetName.IsEmpty())
		{
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("MetadataJsonUtils::TryGetAssetID Missing or empty 'name' field in properties object"));
			// LogJsonString(JsonObject);
		}

		// Safely generate the asset name
		return AssetName;
	}

	inline FString GetCollectionID(const TSharedPtr<FJsonObject>& JsonObject)
	{
		// Check if the "node" field exists
		const TSharedPtr<FJsonObject>* NodeObject;
		if (!JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetCollectionID Missing or invalid 'node' field in JsonObject"));
			LogJsonString(JsonObject);
		}
		
		// Check if the "collectionId" field exists and retrieve its value
		FString CollectionId;
		if (!(*NodeObject)->TryGetStringField(TEXT("collectionId"), CollectionId) || CollectionId.IsEmpty())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetCollectionID Missing or empty 'collectionId' field in node object"));
			LogJsonString(JsonObject);
		}
	
		return CollectionId;
	}
	
	inline FString GetMetadataJson(const TSharedPtr<FJsonObject>& JsonObject)
	{
		// Check if the "node" field exists
		const TSharedPtr<FJsonObject>* NodeObject;
		if (!JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetMetadataJson Missing or invalid 'node' field in JsonObject"));
			LogJsonString(JsonObject);
		}
		
		// Check if the "metadata" field exists
		const TSharedPtr<FJsonObject>* MetadataObject;
		if (!(*NodeObject)->TryGetObjectField(TEXT("metadata"), MetadataObject) || !MetadataObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetMetadataJson Missing or invalid 'metadata' field in node object"));
			LogJsonString(JsonObject);
		}
		
		FString MetadataJson;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MetadataJson);
		FJsonSerializer::Serialize(MetadataObject->ToSharedRef(), Writer);
	
		return MetadataJson;
	}

};

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

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
		FString AssetName;
		
		// Check if the main JSON object exists
		if (!JsonObject.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Invalid JsonObject in InventoryItem"));
			return AssetName;
		}

		// Check if the "node" field exists
		const TSharedPtr<FJsonObject>* NodeObject;
		if (!JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Missing or invalid 'node' field in JsonObject"));
			LogJsonString(JsonObject);
			return AssetName;
		}

		// Check if the "metadata" field exists
		const TSharedPtr<FJsonObject>* MetadataObject;
		if (!(*NodeObject)->TryGetObjectField(TEXT("metadata"), MetadataObject) || !MetadataObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Missing or invalid 'metadata' field in node object"));
			LogJsonString(JsonObject);
			return AssetName;
		}

		// Check if the "properties" field exists
		const TSharedPtr<FJsonObject>* PropertiesObject;
		if (!(*MetadataObject)->TryGetObjectField(TEXT("properties"), PropertiesObject) || !PropertiesObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::TryGetAssetID Missing or invalid 'properties' field in metadata object"));
			LogJsonString(JsonObject);
			return AssetName;
		}

		// Check if the "name" field exists and retrieve its value
		if (!(*PropertiesObject)->TryGetStringField(TEXT("name"), AssetName) || AssetName.IsEmpty())
		{
			UE_LOG(LogFutureverseUBFController, Verbose, TEXT("MetadataJsonUtils::TryGetAssetID Missing or empty 'name' field in properties object"));
		}
		
		return AssetName;
	}

	inline FString GetCollectionID(const TSharedPtr<FJsonObject>& JsonObject)
	{
		FString CollectionId;
		// Check if the "node" field exists
		const TSharedPtr<FJsonObject>* NodeObject;
		if (!JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetCollectionID Missing or invalid 'node' field in JsonObject"));
			LogJsonString(JsonObject);
			return CollectionId;
		}
		
		// Check if the "collectionId" field exists and retrieve its value
		if (!NodeObject->Get()->TryGetStringField(TEXT("collectionId"), CollectionId) || CollectionId.IsEmpty())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetCollectionID Missing or empty 'collectionId' field in node object"));
		}
		
		return CollectionId;
	}
	
	inline FString GetMetadataJson(const TSharedPtr<FJsonObject>& JsonObject)
	{
		FString MetadataJson;
		
		// Check if the "node" field exists
		const TSharedPtr<FJsonObject>* NodeObject;
		if (!JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetMetadataJson Missing or invalid 'node' field in JsonObject"));
			LogJsonString(JsonObject);
			return MetadataJson;
		}
		
		// Check if the "metadata" field exists
		const TSharedPtr<FJsonObject>* MetadataObject;
		if (!NodeObject->Get()->TryGetObjectField(TEXT("metadata"), MetadataObject) || !MetadataObject->IsValid())
		{
			UE_LOG(LogFutureverseUBFController, Warning, TEXT("MetadataJsonUtils::GetMetadataJson Missing or invalid 'metadata' field in node object"));
			LogJsonString(JsonObject);
		}
		
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MetadataJson);
		FJsonSerializer::Serialize(MetadataObject->ToSharedRef(), Writer);
	
		return MetadataJson;
	}

};

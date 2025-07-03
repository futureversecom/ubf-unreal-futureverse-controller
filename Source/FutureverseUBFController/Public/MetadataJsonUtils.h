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
	
	inline TSharedPtr<FJsonValue> FindFieldRecursively(const TSharedPtr<FJsonObject>& JsonObject, const FString& TargetField)
	{
		if (!JsonObject.IsValid()) return nullptr;

		for (const auto& Pair : JsonObject->Values)
		{
			// Direct match
			if (Pair.Key == TargetField)
			{
				return Pair.Value;
			}

			// If value is an object, search inside it
			if (Pair.Value->Type == EJson::Object)
			{
				TSharedPtr<FJsonObject> SubObject = Pair.Value->AsObject();
				if (SubObject.IsValid())
				{
					TSharedPtr<FJsonValue> Found = FindFieldRecursively(SubObject, TargetField);
					if (Found.IsValid())
					{
						return Found;
					}
				}
			}

			// If value is an array, search inside any object elements
			else if (Pair.Value->Type == EJson::Array)
			{
				const TArray<TSharedPtr<FJsonValue>>& Array = Pair.Value->AsArray();
				for (const TSharedPtr<FJsonValue>& Element : Array)
				{
					if (Element.IsValid() && Element->Type == EJson::Object)
					{
						TSharedPtr<FJsonObject> ElementObj = Element->AsObject();
						TSharedPtr<FJsonValue> Found = FindFieldRecursively(ElementObj, TargetField);
						if (Found.IsValid())
						{
							return Found;
						}
					}
				}
			}
		}

		return nullptr;
	}

	inline void FindAllFieldsRecursively(const TSharedPtr<FJsonObject>& JsonObject, const FString& TargetField, TArray<TSharedPtr<FJsonValue>>& OutValues)
	{
		if (!JsonObject.IsValid()) return;

		for (const auto& Pair : JsonObject->Values)
		{
			if (Pair.Key == TargetField)
			{
				OutValues.Add(Pair.Value);
			}

			if (Pair.Value->Type == EJson::Object)
			{
				FindAllFieldsRecursively(Pair.Value->AsObject(), TargetField, OutValues);
			}
			else if (Pair.Value->Type == EJson::Array)
			{
				for (const auto& Element : Pair.Value->AsArray())
				{
					if (Element->Type == EJson::Object)
					{
						FindAllFieldsRecursively(Element->AsObject(), TargetField, OutValues);
					}
				}
			}
		}
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

		if (const auto MetadataProperty = FindFieldRecursively(JsonObject, TEXT("metadata")))
		{
			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MetadataJson);
			FJsonSerializer::Serialize(MetadataProperty->AsObject().ToSharedRef(), Writer);
		}
		
		return MetadataJson;
	}
};

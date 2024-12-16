// Fill out your copyright notice in the Description page of Project Settings.


#include "FuturePassInventoryItem.h"

#include "FutureverseUBFControllerLog.h"

void UFuturePassInventoryItem::Initialize(const FEmergenceInventoryItem& EmergenceInventoryItem)
{
	InventoryItem = EmergenceInventoryItem;
}

FString UFuturePassInventoryItem::GetCollectionID() const
{
	const auto CollectionId =
		InventoryItem.OriginalData.JsonObject->GetObjectField(TEXT("node"))->GetStringField(TEXT("collectionId"));
	
	if (CollectionId.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning,
			TEXT("UFuturePassInventoryItem::GetCollectionID failed parse [node.collectionId] from json object: %s"), *InventoryItem.OriginalData.JsonString);
		return TEXT("InvalidCollectionID");
	}
	
	return CollectionId;
}

FString UFuturePassInventoryItem::GetCombinedID() const
{
	return FString::Printf(TEXT("%s:%s"), *GetCollectionID(), *InventoryItem.tokenId);
}

FString UFuturePassInventoryItem::MapCollectionIdToName(const FString& CollectionId) const
{
	return CollectionId;
}

void UFuturePassInventoryItem::LogDataString(TSharedPtr<FJsonObject> JsonObject)
{
	FString JsonString;

	FJsonObjectWrapper ObjectWrapper;
	ObjectWrapper.JsonObject = JsonObject;
	ObjectWrapper.JsonObjectToString(JsonString);
	
	UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFuturePassInventoryItem::GetAssetID failed parse [node.metadata.properties.name] from json object: %s"), *JsonString);
}


FString UFuturePassInventoryItem::GetAssetID() const
{
	// Check if the main JSON object exists
	if (!InventoryItem.OriginalData.JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UFuturePassInventoryItem::GetAssetID Invalid JsonObject in InventoryItem"));
	}

	// Check if the "node" field exists
	const TSharedPtr<FJsonObject>* NodeObject;
	if (!InventoryItem.OriginalData.JsonObject->TryGetObjectField(TEXT("node"), NodeObject) || !NodeObject->IsValid())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFuturePassInventoryItem::GetAssetID Missing or invalid 'node' field in JsonObject"));
		LogDataString(InventoryItem.OriginalData.JsonObject);
	}

	// Check if the "metadata" field exists
	const TSharedPtr<FJsonObject>* MetadataObject;
	if (!(*NodeObject)->TryGetObjectField(TEXT("metadata"), MetadataObject) || !MetadataObject->IsValid())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFuturePassInventoryItem::GetAssetID Missing or invalid 'metadata' field in node object"));
		LogDataString(InventoryItem.OriginalData.JsonObject);
	}

	// Check if the "properties" field exists
	const TSharedPtr<FJsonObject>* PropertiesObject;
	if (!(*MetadataObject)->TryGetObjectField(TEXT("properties"), PropertiesObject) || !PropertiesObject->IsValid())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFuturePassInventoryItem::GetAssetID Missing or invalid 'properties' field in metadata object"));
		LogDataString(InventoryItem.OriginalData.JsonObject);
	}

	// Check if the "name" field exists and retrieve its value
	FString MetadataName;
	if (!(*PropertiesObject)->TryGetStringField(TEXT("name"), MetadataName) || MetadataName.IsEmpty())
	{
		UE_LOG(LogFutureverseUBFController, Warning, TEXT("UFuturePassInventoryItem::GetAssetID Missing or empty 'name' field in properties object"));
		LogDataString(InventoryItem.OriginalData.JsonObject);
	}

	// Safely generate the asset ID string
	return FString::Printf(TEXT("%s:%s"), *MapCollectionIdToName(InventoryItem.contract), *MetadataName);
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "FuturePassInventoryItem.h"

#include "FutureverseUBFControllerLog.h"
#include "FutureverseUBFControllerSubsystem.h"

void UFuturePassInventoryItem::Initialize(const FEmergenceInventoryItem& EmergenceInventoryItem)
{
	InventoryItem = EmergenceInventoryItem;

	if (UFutureverseUBFControllerSubsystem* Subsystem = UFutureverseUBFControllerSubsystem::Get(this))
	{
		AssetProfile = Subsystem->GetAssetProfile(GetAssetID());
	}
}

const FAssetProfile& UFuturePassInventoryItem::GetAssetProfileRef() const
{
	return AssetProfile;
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
	// Retrieve the "name" field as a string, ensuring it exists
	FString MetadataName = InventoryItem.OriginalData.JsonObject
														->GetObjectField(TEXT("node"))
														->GetObjectField(TEXT("metadata"))
														->GetObjectField(TEXT("properties"))
														->GetStringField(TEXT("name"));

	if (MetadataName.IsEmpty())
	{
		LogDataString( InventoryItem.OriginalData.JsonObject);
	}
	
	return FString::Printf(TEXT("%s:%s"), *MapCollectionIdToName(InventoryItem.contract), *MetadataName);
}


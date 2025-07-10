// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "InventoryComponents/UBFInventoryComponent.h"
#include "InventoryComponents/ItemRegistry.h"

UUBFInventoryComponent::UUBFInventoryComponent()
{
	ItemRegistry = MakeShared<FItemRegistry>();
}

UUBFItem* UUBFInventoryComponent::GetItem(const FString& ItemId) const
{
	return ItemRegistry->GetItem(ItemId);
}

void UUBFInventoryComponent::RegisterItem(const FString& ItemId, UUBFItem* Item)
{
	ItemRegistry->RegisterItem(ItemId, Item);
}

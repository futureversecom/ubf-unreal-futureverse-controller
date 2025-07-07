// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "InventoryComponents/UBFInventoryComponent.h"
#include "InventoryComponents/ItemRegistry.h"

void UUBFInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ItemRegistry = MakeShared<FItemRegistry>();
}

UUBFItem* UUBFInventoryComponent::GetItem(const FString& ItemId)
{
	return ItemRegistry->GetItem(ItemId);
}

void UUBFInventoryComponent::RegisterItem(const FString& ItemId, UUBFItem* Item)
{
	ItemRegistry->RegisterItem(ItemId, Item);
}

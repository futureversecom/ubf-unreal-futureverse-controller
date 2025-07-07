#include "ItemRegistry.h"

UUBFItem* FItemRegistry::GetItem(const FString& ItemId)
{
	return ItemMap.Contains(ItemId) ? ItemMap[ItemId] : nullptr;
}

void FItemRegistry::RegisterItem(const FString& ItemId, UUBFItem* Item)
{
	if (ItemMap.Contains(ItemId))
	{
		ItemMap[ItemId] = Item;
	}
	else
	{
		ItemMap.Add(ItemId, Item);
	}
}

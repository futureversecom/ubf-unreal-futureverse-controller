#pragma once
#include "Items/UBFItem.h"

class FItemRegistry
{

public:
	virtual ~FItemRegistry() = default;
	UUBFItem* GetItem(const FString& ItemId);
	void RegisterItem(const FString& ItemId, UUBFItem* Item);

protected:
	TMap<FString, UUBFItem*> ItemMap;
};

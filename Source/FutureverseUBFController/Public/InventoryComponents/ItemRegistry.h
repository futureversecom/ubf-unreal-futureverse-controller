#pragma once

#include "CoreMinimal.h"

class UUBFItem;

class FUTUREVERSEUBFCONTROLLER_API FItemRegistry
{
public:
	FItemRegistry(){}
	virtual ~FItemRegistry() = default;
	
	UUBFItem* GetItem(const FString& ItemId) const;
	void RegisterItem(const FString& ItemId, UUBFItem* Item);

protected:
	TMap<FString, UUBFItem*> ItemMap;
};

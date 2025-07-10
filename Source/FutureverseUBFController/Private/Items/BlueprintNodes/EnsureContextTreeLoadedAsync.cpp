// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Items/BlueprintNodes/EnsureContextTreeLoadedAsync.h"

UEnsureContextTreeLoadedAsync* UEnsureContextTreeLoadedAsync::EnsureContextTreeLoaded(UUBFItem* Item)
{
	UEnsureContextTreeLoadedAsync* Node = NewObject<UEnsureContextTreeLoadedAsync>();
	Node->ItemRef = Item;
	return Node;
}

void UEnsureContextTreeLoadedAsync::Activate()
{
	if (!ItemRef)
	{
		OnCompleted.Broadcast(false);
		return;
	}

	ItemRef->EnsureContextTreeLoaded().Next([this](bool bResult)
	{
		OnCompleted.Broadcast(bResult);
	});
}
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
	TWeakObjectPtr<UEnsureContextTreeLoadedAsync> WeakThis = this;
	ItemRef->EnsureContextTreeLoaded().Next([WeakThis](bool bResult)
	{
		if (WeakThis.IsValid())
		{
			WeakThis->OnCompleted.Broadcast(bResult);
		}
	});
}
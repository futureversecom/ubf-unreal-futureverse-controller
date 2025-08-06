// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Items/BlueprintNodes/EnsureProfileURILoadedAsync.h"

UEnsureProfileURILoadedAsync* UEnsureProfileURILoadedAsync::EnsureProfileURILoaded(UUBFItem* Item)
{
	UEnsureProfileURILoadedAsync* Node = NewObject<UEnsureProfileURILoadedAsync>();
	Node->ItemRef = Item;
	return Node;
}

void UEnsureProfileURILoadedAsync::Activate()
{
	if (!ItemRef)
	{
		OnCompleted.Broadcast(false);
		return;
	}
	TWeakObjectPtr<UEnsureProfileURILoadedAsync> WeakThis = this;
	ItemRef->EnsureProfileURILoaded().Next([WeakThis](bool bResult)
	{
		if (WeakThis.IsValid())
		{
			WeakThis->OnCompleted.Broadcast(bResult);
		}
	});
}
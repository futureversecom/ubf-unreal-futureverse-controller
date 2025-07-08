// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Items/UBFItem.h"

void UUBFItem::InitializeFromRenderData(const FUBFRenderData& RenderData)
{
	ItemData.AssetID = RenderData.AssetID;
	ItemData.MetadataJson = RenderData.MetadataJson;
	ContextTree = RenderData.ContextTree;
}

TFuture<bool> UUBFItem::EnsureContextTreeLoaded()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
	TFuture<bool> Future = Promise->GetFuture();

	if (!ContextTree.IsEmpty())
	{
		Promise->SetValue(true);
		return Future;
	}

	LoadContextTree().Next([Promise](bool bSuccess)
	{
		Promise->SetValue(bSuccess);
	});
	
	return Future;
}

TFuture<bool> UUBFItem::EnsureProfileURILoaded()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
	TFuture<bool> Future = Promise->GetFuture();

	if (!ProfileURI.IsEmpty())
	{
		Promise->SetValue(true);
		return Future;
	}

	LoadProfileURI().Next([Promise](bool bSuccess)
	{
		Promise->SetValue(bSuccess);
	});
	
	return Future;
}

TFuture<bool> UUBFItem::LoadContextTree()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
	TFuture<bool> Future = Promise->GetFuture();

	Promise->SetValue(false);
	
	return Future;
}

TFuture<bool> UUBFItem::LoadProfileURI()
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
	TFuture<bool> Future = Promise->GetFuture();

	Promise->SetValue(false);
	
	return Future;
}


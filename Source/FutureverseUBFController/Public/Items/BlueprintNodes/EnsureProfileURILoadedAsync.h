// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Items/UBFItem.h"
#include "EnsureProfileURILoadedAsync.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProfileURILoadedDelegate, bool, bSuccess);

UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UEnsureProfileURILoadedAsync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FProfileURILoadedDelegate OnCompleted;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UEnsureProfileURILoadedAsync* EnsureProfileURILoaded(UUBFItem* Item);

	virtual void Activate() override;

private:
	UPROPERTY()
	UUBFItem* ItemRef;
};
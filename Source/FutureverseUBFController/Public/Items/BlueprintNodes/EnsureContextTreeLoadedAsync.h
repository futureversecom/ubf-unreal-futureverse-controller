#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Items/UBFItem.h"
#include "EnsureContextTreeLoadedAsync.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FContextTreeLoadedDelegate, bool, bSuccess);

UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UEnsureContextTreeLoadedAsync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FContextTreeLoadedDelegate OnCompleted;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UEnsureContextTreeLoadedAsync* EnsureContextTreeLoaded(UUBFItem* Item);

	virtual void Activate() override;

private:
	UPROPERTY()
	UUBFItem* ItemRef;
};
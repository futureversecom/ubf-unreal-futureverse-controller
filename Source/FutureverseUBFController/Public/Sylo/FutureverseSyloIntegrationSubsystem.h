// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FuturepassUser.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FutureverseSyloIntegrationSubsystem.generated.h"

class FFuturepassSyloAccessSource;
/**
 * Manages integrating with SyloSDK, automatically set first Futurepass LoggedIn user as SyloAccessSource for Futureverse sylos
 */
UCLASS()
class FUTUREVERSEUBFCONTROLLER_API UFutureverseSyloIntegrationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void OverrideCurrentSource(UFuturepassUser* User); 

	/* Can be null */
	UFUNCTION(BlueprintCallable)
	UFuturepassUser* GetCurrentUserSource() const;

	UFUNCTION(BlueprintCallable)
	void FindAndSetBestUserAsSource();
private:
	UFUNCTION()
	void OnLoginComplete(UFuturepassUser* User);
	UFUNCTION()
	void OnLogout(UFuturepassUser* User);

	void SetUserAsSource(UFuturepassUser* User);
	void ClearCurrentAccessSource();

	bool HasSyloAccessSource() const;

	TSharedPtr<FFuturepassSyloAccessSource> CurrentAccessSource;
};

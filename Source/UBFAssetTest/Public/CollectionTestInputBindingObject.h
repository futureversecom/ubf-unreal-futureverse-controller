// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CollectionTestInputBindingObject.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class UBFASSETTEST_API UCollectionTestInputBindingObject : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void Initialize(const FString& NewID, const FString& NewType, const FString& NewTargetContractID, const TArray<FString>& NewOptions)
	{
		ID = NewID;
		Type = NewType;
		TargetContractID = NewTargetContractID;
		Options = NewOptions;
		Value = "";
	}

	UFUNCTION(BlueprintCallable)
	FString GetId() const { return ID; }

	UFUNCTION(BlueprintCallable)
	FString GetType() const { return Type; }
	
	UFUNCTION(BlueprintCallable)
	FString GetTargetContractID() const { return TargetContractID; }

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetOptions() const { return Options; }

	UFUNCTION(BlueprintCallable)
	FString GetValue() const { return Value; }
	
	UFUNCTION(BlueprintCallable)
	void SetValue(const FString& NewValue) { Value = NewValue ; }

private:
	FString ID;
	FString Type;
	FString TargetContractID;
	FString Value;
	TArray<FString> Options;
};
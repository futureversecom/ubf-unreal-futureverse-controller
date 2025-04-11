// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AssetProfile.generated.h"

USTRUCT()
struct UBFAPICONTROLLER_API FAssetProfileVariant
{
	GENERATED_BODY()
public:
	FAssetProfileVariant() {}
	FAssetProfileVariant(const FString& Id, const FString& RenderId, const FString& ParsingId,
		const FString& RenderCatalogUri, const FString& ParsingCatalogUri);

	FString GetRenderBlueprintId() const;
	FString GetRenderCatalogUri() const;
	FString GetParsingBlueprintId() const;
	FString GetParsingCatalogUri() const;
	FString GetVariantId() const{return VariantId;}
	bool IsValid() const {return VariantId != FString("Invalid");}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("VariantId: %s\nRenderBlueprintId: %s\nParsingID: %s\nRenderCatalogUri: %s\nParsingCatalogUri: %s\nRelativePath: %s"),
			*VariantId,
			*RenderBlueprintId,
			*ParsingBlueprintId,
			*RenderCatalogUri,
			*ParsingCatalogUri,
			*RelativePath
		);
	}
	
	UPROPERTY()
	FString RelativePath;
private:
	UPROPERTY()
	FString VariantId = FString("Invalid");
	
	UPROPERTY()
	FString RenderBlueprintId;
	UPROPERTY()
	FString ParsingBlueprintId;
	UPROPERTY()
	FString RenderCatalogUri;
	UPROPERTY()
	FString ParsingCatalogUri;
};


USTRUCT(BlueprintType)
struct UBFAPICONTROLLER_API FAssetProfile
{
	GENERATED_BODY()
public:
	FAssetProfile(){}
	FAssetProfile(const FString& Id, const TArray<FAssetProfileVariant>& Variants) : Id(Id), Variants(Variants){}
	
	FString GetRenderBlueprintId(const FString& Variant) const;
	FString GetRenderCatalogUri(const FString& Variant) const;
	FString GetParsingBlueprintId(const FString& Variant) const;
	FString GetParsingCatalogUri(const FString& Variant) const;
	FString GetId() const {return Id;}
	bool IsValid() const {return Id != FString("Invalid");}

	void OverrideRelativePaths(const FString& NewRelativePath);
	void ModifyId(const FString& NewId);

	FString ToString() const;
	
private:
	int GetIndexForVariant(const FString& Variant) const;
	
	UPROPERTY()
	FString Id = FString("Invalid");
	
	UPROPERTY()
	TArray<FAssetProfileVariant> Variants;
};

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "ControllerLayers/AssetProfile.h"

FAssetProfileVariant::FAssetProfileVariant(const FString& Id, const FString& RenderId, const FString& ParsingId,
										   const FString& RenderCatalogUri, const FString& ParsingCatalogUri)
		: VariantId(Id), RenderBlueprintId(RenderId), ParsingBlueprintId(ParsingId),
			RenderCatalogUri(RenderCatalogUri), ParsingCatalogUri(ParsingCatalogUri)
{
	
}

FString FAssetProfileVariant::GetRenderBlueprintId() const
{
	return RenderBlueprintId;
}

FString FAssetProfileVariant::GetRenderCatalogUri() const
{
	return RelativePath + RenderCatalogUri;
}

FString FAssetProfileVariant::GetParsingBlueprintId() const
{
	return ParsingBlueprintId;
}

FString FAssetProfileVariant::GetParsingCatalogUri() const
{
	return RelativePath + ParsingCatalogUri;
}

FString FAssetProfile::GetRenderBlueprintId(const FString& Variant) const
{
	int Index = GetIndexForVariant(Variant);
	if (!Variants.IsValidIndex(Index)) return FString();

	return Variants[Index].GetRenderBlueprintId();
}

FString FAssetProfile::GetRenderCatalogUri(const FString& Variant) const
{
	int Index = GetIndexForVariant(Variant);
	if (!Variants.IsValidIndex(Index)) return FString();

	return Variants[Index].GetRenderCatalogUri();
}

FString FAssetProfile::GetParsingBlueprintId(const FString& Variant) const
{
	int Index = GetIndexForVariant(Variant);
	if (!Variants.IsValidIndex(Index)) return FString();

	return Variants[Index].GetParsingBlueprintId();
}

FString FAssetProfile::GetParsingCatalogUri(const FString& Variant) const
{
	int Index = GetIndexForVariant(Variant);
	if (!Variants.IsValidIndex(Index)) return FString();

	return Variants[Index].GetParsingCatalogUri();
}

void FAssetProfile::OverrideRelativePaths(const FString& NewRelativePath)
{
	for (FAssetProfileVariant& Variant : Variants)
	{
		Variant.RelativePath = NewRelativePath;
	}
}

void FAssetProfile::ModifyId(const FString& NewId)
{
	Id = NewId;
}

FString FAssetProfile::ToString() const
{
	FString VariantsString;
	for (const auto& Variant : Variants)
	{
		VariantsString += Variant.ToString() + ", ";
	}
	return FString::Printf(TEXT("Id: %s Variants: %s"), *GetId(), *VariantsString);
}

int FAssetProfile::GetIndexForVariant(const FString& Variant) const
{
	for (int i = 0; i < Variants.Num(); i++)
	{
		if (Variants[i].GetVariantId() == Variant)
			return i;
	}

	return -1;
}

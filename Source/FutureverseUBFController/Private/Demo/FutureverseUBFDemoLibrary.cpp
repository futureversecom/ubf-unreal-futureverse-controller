// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Demo/FutureverseUBFDemoLibrary.h"

#include "MetadataJsonUtils.h"

FString UFutureverseUBFDemoLibrary::GetImagePropertyFromJsonString(const FJsonObjectWrapper& JsonObjectWrapper,
                                                                   const FString& ImagePropertyName)
{
	// Check if the root JSON object is valid
	if (!JsonObjectWrapper.JsonObject.IsValid())
	{
		return FString();
	}

	const auto ImageProperty = MetadataJsonUtils::FindFieldRecursively(JsonObjectWrapper.JsonObject, ImagePropertyName);
	if (!ImageProperty)
	{
		return FString();
	}
	
	return ImageProperty->AsString();
}

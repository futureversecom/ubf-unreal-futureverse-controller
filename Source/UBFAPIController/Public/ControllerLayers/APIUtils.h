#pragma once

#include "Graph.h"
#include "GraphProvider.h"
#include "HttpModule.h"
#include "ICacheLoader.h"
#include "UBFAPIControllerLog.h"
#include "UBFUtils.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

namespace APIUtils
{
	static TFuture<TSharedPtr<IHttpResponse>> MakeDownloadRequest(const FString& Path)
	{
		TSharedPtr<TPromise<TSharedPtr<IHttpResponse>>> Promise = MakeShareable(new TPromise<TSharedPtr<IHttpResponse>>());
		TFuture<TSharedPtr<IHttpResponse>> Future = Promise->GetFuture();
	
		const TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();

		// encode space character
		const FString EncodedUrl = Path.Replace(TEXT(" "), TEXT("%20"));
		Request->SetURL(EncodedUrl);
		Request->SetVerb(TEXT("GET"));
	
		auto Callback = [Promise](FHttpRequestPtr Request, const FHttpResponsePtr& Response, bool bWasSuccessful) mutable
		{
			if (Response == nullptr)
			{
				Promise->SetValue(nullptr);
				return;
			}
		
			if (bWasSuccessful && Response.IsValid())
			{
				Promise->SetValue(Response);
			}
			else
			{
				Promise->SetValue(nullptr);
			}
		};
	
		Request->OnProcessRequestComplete().BindLambda(Callback);
		Request->ProcessRequest();

		return Future;
	}

	static bool LoadLocalFileToData(const FString& ResourcePath, TArray<uint8>& Data)
	{
		if (!FPaths::FileExists(ResourcePath))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("Could not find file with given path: %s"), *ResourcePath);
			return false;
		}
	
		if (!FFileHelper::LoadFileToArray(Data, *ResourcePath))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("Failed to load file from path: %s"), *ResourcePath);
			return false;
		}

		return true;
	}

	static bool LoadLocalFileToString(const FString& FilePath, FString& LoadedString)
	{
		if (!FPaths::FileExists(FilePath))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("Could not find file with given path: %s"), *FilePath);
			return false;
		}
		
		if (!FFileHelper::LoadFileToString(LoadedString, *FilePath))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("Failed to read file: %s"), *FilePath);
			return false;
		}
	
		return true;
	}
	
	static TFuture<UBF::FLoadDataArrayResult> LoadDataFromURI(const FString& Path, const FString& Hash, ICacheLoader* CacheLoader)
	{
		TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShareable(new TPromise<UBF::FLoadDataArrayResult>());
		TFuture<UBF::FLoadDataArrayResult> Future = Promise->GetFuture();

		TArray<uint8> CachedData;
		
		if (CacheLoader != nullptr && CacheLoader->TryGetCachedBytes(Path, Hash, CachedData))
		{
			UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("Loaded data from cached bytes : %s"), *Path);
			
			UBF::FLoadDataArrayResult CacheLoadResult;
			CacheLoadResult.Result = TPair<bool, TArray<uint8>>(true, CachedData);
			Promise->SetValue(CacheLoadResult);
			return Future;
		}
		
		if (Path.StartsWith(TEXT("http")))
		{
			MakeDownloadRequest(Path).Next([Promise, Path, Hash, CacheLoader](const TSharedPtr<IHttpResponse>& Response)
			{
				UBF::FLoadDataArrayResult LoadResult;
				TArray<uint8> Data;
				
				if (Response.IsValid() && Response->GetResponseCode() == EHttpResponseCodes::Ok)
				{
					Data = Response->GetContent();
					if (CacheLoader != nullptr)
					{
						CacheLoader->CacheBytes(Path, Hash, Data);
					}

					LoadResult.Result = TPair<bool, TArray<uint8>>(true, Data);
					Promise->SetValue(LoadResult);
				}
				else if (Response.IsValid())
				{
					FString ErrorMessage = GetDescription(static_cast<EHttpResponseCodes::Type>(Response->GetResponseCode())).ToString();
					UE_LOG(LogUBFAPIController, Error, TEXT("Failed to download with URL: %s Message: %s"), *Path, *ErrorMessage);
					LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
					Promise->SetValue(LoadResult);
				}
				else
				{
					UE_LOG(LogUBFAPIController, Error, TEXT("Invalid Response from URL: %s"), *Path);
					LoadResult.Result = TPair<bool, TArray<uint8>>(false, Data);
					Promise->SetValue(LoadResult);
				}
			});
		}
		else
		{
			// path is local
			UBF::FLoadDataArrayResult LoadResult;
			TArray<uint8> Data;
			
			bool WasSuccessful = LoadLocalFileToData(Path, Data);
			if (!WasSuccessful)
			{
				UE_LOG(LogUBFAPIController, Error, TEXT("Failed to load file to data from path: %s"), *Path);
			}
			
			if (WasSuccessful && CacheLoader != nullptr)
			{
				CacheLoader->CacheBytes(Path, Hash, Data);
			}

			LoadResult.Result = TPair<bool, TArray<uint8>>(WasSuccessful, Data);
			Promise->SetValue(LoadResult);
		}
		
		return Future;
	}

	static TFuture<UBF::FLoadStringResult> LoadStringFromURI(const FString& Path, const FString& Hash, ICacheLoader* CacheLoader)
	{
		TSharedPtr<TPromise<UBF::FLoadStringResult>> Promise = MakeShareable(new TPromise<UBF::FLoadStringResult>());
		TFuture<UBF::FLoadStringResult> Future = Promise->GetFuture();
		
		TArray<uint8> CachedData;
		if (CacheLoader != nullptr && CacheLoader->TryGetCachedBytes(Path, Hash, CachedData))
		{
			// Convert TArray<uint8> (UTF-8) back to FString
			FString LoadedString = FString(StringCast<TCHAR>(reinterpret_cast<const char*>(CachedData.GetData())));
			UE_LOG(LogUBFAPIController, VeryVerbose, TEXT("Loaded string from cached bytes : %s"), *LoadedString);
	
			UBF::FLoadStringResult CacheLoadResult;
			CacheLoadResult.Result = TPair<bool, FString>(true, LoadedString);
			Promise->SetValue(CacheLoadResult);
			return Future;
		}
		
		if (Path.StartsWith(TEXT("http")))
		{
			MakeDownloadRequest(Path).Next([Promise, Path, Hash, CacheLoader](const TSharedPtr<IHttpResponse>& Response)
			{
				UBF::FLoadStringResult LoadResult;
				FString LoadedString;
				if (Response.IsValid() && Response->GetResponseCode() == EHttpResponseCodes::Ok)
				{
					LoadedString = Response->GetContentAsString();
				
					if (CacheLoader != nullptr)
					{
						// Convert FString to UTF-8 encoded TArray<uint8>
						TArray<uint8> Data;
						const auto UTF8String = StringCast<UTF8CHAR>(*LoadedString);
						Data.Append(reinterpret_cast<const uint8*>(UTF8String.Get()), UTF8String.Length());
							
						// Append a null-terminator to ensure the UTF-8 string is properly terminated
						Data.Add(0);
							
						CacheLoader->CacheBytes(Path, Hash, Data);
					}

					LoadResult.Result = TPair<bool, FString>(true, LoadedString);
					Promise->SetValue(LoadResult);

				}
				else if (Response.IsValid())
				{
					FString ErrorMessage = GetDescription(static_cast<EHttpResponseCodes::Type>(Response->GetResponseCode())).ToString();
					UE_LOG(LogUBFAPIController, Error, TEXT("Failed to download with URL: %s Message: %s"), *Path, *ErrorMessage);
					LoadResult.Result = TPair<bool, FString>(false, LoadedString);
					Promise->SetValue(LoadResult);
				}
				else
				{
					UE_LOG(LogUBFAPIController, Error, TEXT("Invalid Response from URL: %s"), *Path);
					LoadResult.Result = TPair<bool, FString>(false, LoadedString);
					Promise->SetValue(LoadResult);
				}
			});
		}
		else
		{
			// path is local
			UBF::FLoadStringResult LoadResult;
			FString LoadedString;
			
			bool WasSuccessful = LoadLocalFileToString(Path, LoadedString);
			if (!WasSuccessful)
			{
				UE_LOG(LogUBFAPIController, Error, TEXT("Failed to load file to string from path: %s"), *Path);
			}
			
			if (WasSuccessful && CacheLoader != nullptr)
			{
				// Convert FString to UTF-8 encoded TArray<uint8>
				TArray<uint8> Data;
				const auto UTF8String = StringCast<UTF8CHAR>(*LoadedString);
				Data.Append(reinterpret_cast<const uint8*>(UTF8String.Get()), UTF8String.Length());
				
				// Append a null-terminator to ensure the UTF-8 string is properly terminated
				Data.Add(0);
				
				CacheLoader->CacheBytes(Path, Hash, Data);
			}

			LoadResult.Result = TPair<bool, FString>(WasSuccessful, LoadedString);
			Promise->SetValue(LoadResult);
		}
		
		return Future;
	}
}

// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Sylo/SyloURIResolver.h"

#include "SyloSubsystem.h"
#include "SyloUtils.h"
#include "GraphProvider.h"

bool USyloURIResolver::CanResolveURI(const FString& URI)
{
	return SyloUtils::IsValidDID(URI);
}

TFuture<UBF::FLoadDataArrayResult> USyloURIResolver::ResolveURI(const FString& TypeId, const FString& URI)
{
	TSharedPtr<TPromise<UBF::FLoadDataArrayResult>> Promise = MakeShared<TPromise<UBF::FLoadDataArrayResult>>();
	if (USyloSubsystem* SyloSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USyloSubsystem>())
	{
		SyloSubsystem->LoadSyloDIDFuture(URI).Next([Promise](const FSyloLoadResult& Result)
		{
			if (!Result.bSuccess)
			{
				Promise->SetValue(UBF::FLoadDataArrayResult());
				return;
			}

			UBF::FLoadDataArrayResult DataArrayResult;
			DataArrayResult.SetResult(Result.Data);
			Promise->SetValue(DataArrayResult);
		});
	}
	else
	{
		Promise->SetValue(UBF::FLoadDataArrayResult());
	}

	return Promise->GetFuture();
}

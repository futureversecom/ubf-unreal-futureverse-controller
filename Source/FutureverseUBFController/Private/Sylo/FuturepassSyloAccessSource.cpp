#include "Sylo/FuturepassSyloAccessSource.h"

FString FFuturepassSyloAccessSource::GetAccessToken()
{
	if (User.IsValid()) return User->GetUserData().AccessToken;

	return FString("INVALID");
}

TFuture<bool> FFuturepassSyloAccessSource::RefreshAccessToken()
{
	SharedPromise = MakeShared<TPromise<bool>>();

	if (User.IsValid())
	{
		TSharedPtr<TPromise<bool>> SharedPromiseCopy = SharedPromise;
		
		User->RefreshAccessToken_Future().Next([SharedPromiseCopy](bool bSuccess)
		{
			SharedPromiseCopy->SetValue(bSuccess);
		});
	}
	else
	{
		SharedPromise->SetValue(false);
	}

	return SharedPromise->GetFuture();
}

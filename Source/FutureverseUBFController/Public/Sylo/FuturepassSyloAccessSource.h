#pragma once

#include "FuturepassUser.h"
#include "SyloAccessSource/ISyloAccessSource.h"

class FFuturepassSyloAccessSource : public ISyloAccessSource
{
public:
	FFuturepassSyloAccessSource(UFuturepassUser* User) : User(User) {}
	
	virtual FString GetAccessToken() override;
	virtual TFuture<bool> RefreshAccessToken() override;

	bool IsTargetUserIsValid() const {return User.IsValid();}
	UFuturepassUser* GetTargetUser() const {return User.Get();}
private:
	TWeakObjectPtr<UFuturepassUser> User;
	TSharedPtr<TPromise<bool>> SharedPromise;
};
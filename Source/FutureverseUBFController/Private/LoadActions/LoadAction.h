#pragma once

template<class T>
class TLoadAction : public TSharedFromThis<T>
{
public:
protected:
	void CompletePendingLoad();
	void CheckPendingLoadsComplete();
	void AddPendingLoad();
	
	TSharedPtr<TPromise<bool>> Promise;

	std::atomic<bool> bFailure = false;
private:
	
	int PendingLoads = 0;
	FCriticalSection CriticalSection;
	bool bPromiseSet = false;
};

template <class T>
void TLoadAction<T>::CompletePendingLoad()
{
	if (!ensure(!bPromiseSet)) return;
	
	{
		FScopeLock Lock(&CriticalSection);
		PendingLoads--;
	}

	CheckPendingLoadsComplete();
}

template <class T>
void TLoadAction<T>::CheckPendingLoadsComplete()
{
	if (!ensure(!bPromiseSet)) return;
	
	bool bShouldSetValue = false;

	// Lock the critical section to safely read PendingLoads
	{
		FScopeLock Lock(&CriticalSection);
		if (PendingLoads <= 0)
		{
			bShouldSetValue = true;
		}
	}

	// Set the value outside the lock to avoid deadlocks
	if (bShouldSetValue)
	{
		Promise->SetValue(!bFailure);
		bPromiseSet = true;
	}
}

template <class T>
void TLoadAction<T>::AddPendingLoad()
{
	if (!ensure(!bPromiseSet)) return;
	
	FScopeLock Lock(&CriticalSection);
	PendingLoads++;
}

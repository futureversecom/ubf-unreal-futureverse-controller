#pragma once

namespace LoadActionUtils
{
	template<typename T>
	TFuture<TArray<T>> WhenAll(TArray<TFuture<T>>& Futures)
	{
		TSharedPtr<TPromise<TArray<T>>> Promise = MakeShared<TPromise<TArray<T>>>();
		TSharedRef<TArray<T>> Results = MakeShared<TArray<T>>();
		TSharedRef<FThreadSafeCounter> Counter = MakeShared<FThreadSafeCounter>(Futures.Num());

		Results->SetNum(Futures.Num());

		if (Futures.IsEmpty())
		{
			Promise->SetValue(*Results);
			return Promise->GetFuture();
		}

		for (int32 Index = 0; Index < Futures.Num(); ++Index)
		{
			TFuture<T>& Future = Futures[Index]; // non-const ref
			Future.Next([Index, Results, Counter, Promise, Total = Futures.Num()](const T& Result)
			{
				(*Results)[Index] = Result;
				if (Counter->Decrement() == 0)
				{
					Promise->SetValue(*Results);
				}
			});
		}

		return Promise->GetFuture();
	}
}
// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#include "FutureverseUBFControllerLog.h"


DEFINE_LOG_CATEGORY(LogFutureverseUBFController);

namespace FutureverseUBFControllerLogging
{
	static TAutoConsoleVariable<bool> CVarFutureverseUBFControllerDebugLogging(
	TEXT("FutureverseUBFController.Logging.DebugLogging"),
	false,
	TEXT("Enable verbose debug logging for FutureverseUBFController."));

	bool DebugLoggingEnabled()
	{
		return CVarFutureverseUBFControllerDebugLogging.GetValueOnAnyThread();
	}
}

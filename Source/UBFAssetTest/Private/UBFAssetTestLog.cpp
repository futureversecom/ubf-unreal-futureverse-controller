#include "UBFAssetTestLog.h"

DEFINE_LOG_CATEGORY(LogUBFAssetTest);

namespace UBFAssetTestLogging
{
	static TAutoConsoleVariable<bool> CVarUBFAssetTestDebugLogging(
	TEXT("UBFAssetTest.Logging.DebugLogging"),
	false,
	TEXT("Enable verbose debug logging for UBF Asset Test."));

	bool DebugLoggingEnabled()
	{
		return CVarUBFAssetTestDebugLogging.GetValueOnAnyThread();
	}
}
#include "ControllerLayers/UBFAPIControllerLog.h"

DEFINE_LOG_CATEGORY(LogUBFAPIController);

namespace UBFLogging
{
	static TAutoConsoleVariable<bool> CVarUBFAPIControllerDebugLogging(
	TEXT("UBFAPIController.Logging.DebugLogging"),
	false,
	TEXT("Enable verbose debug logging for UBFAPIController."));

	bool DebugLoggingEnabled()
	{
		return CVarUBFAPIControllerDebugLogging.GetValueOnAnyThread();
	}
}

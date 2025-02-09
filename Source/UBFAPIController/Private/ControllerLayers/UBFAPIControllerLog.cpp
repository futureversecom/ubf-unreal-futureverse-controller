#include "ControllerLayers/UBFAPIControllerLog.h"

DEFINE_LOG_CATEGORY(LogUBFAPIController);

namespace UBFAPIControllerLogging
{
	static TAutoConsoleVariable<bool> CVarUBFAPIControllerDebugLogging(
	TEXT("UBFAPIController.Logging.DebugLogging"),
	false,
	TEXT("Enable verbose debug logging for UBFAPIController."));

	static TAutoConsoleVariable<bool> CVarUBFAPIControllerGraphLogging(
	TEXT("UBFAPIController.Logging.GraphLogging"),
	false,
	TEXT("Allows PrintBlueprintDebug() to print out Graph Jsons, this typically happens when a graph related error occurs such as failing to read input"));

	bool DebugLoggingEnabled()
	{
		return CVarUBFAPIControllerDebugLogging.GetValueOnAnyThread();
	}

	bool GraphLoggingEnabled()
	{
		return CVarUBFAPIControllerGraphLogging.GetValueOnAnyThread();
	}
}

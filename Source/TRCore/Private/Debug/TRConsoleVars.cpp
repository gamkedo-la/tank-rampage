#include "Debug/TRConsoleVars.h"

#if TR_DEBUG_ENABLED

namespace TR
{
	TAutoConsoleVariable<bool> CVarDebugTankForces(
		TEXT("tr.tank.showForces"),
		false,
		TEXT("Toggle on/off debug drawing of tank forces"),
		ECVF_Scalability | ECVF_RenderThreadSafe);
}

#endif
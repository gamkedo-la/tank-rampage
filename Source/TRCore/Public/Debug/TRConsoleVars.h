#pragma once

#include "CoreMinimal.h"
#include "TRConstants.h"

#ifndef TR_DEBUG_ENABLED
	#define TR_DEBUG_ENABLED !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif

#if TR_DEBUG_ENABLED

namespace TR
{
	extern TRCORE_API TAutoConsoleVariable<bool> CVarDebugTankForces;
}
#endif

#pragma once

#include "CoreMinimal.h"

#ifndef TR_DEBUG_ENABLED
	#define TR_DEBUG_ENABLED !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif

#define TR_AI_PATH_ACCEL 1
#define TR_AI_PATH_CROWD 1

#pragma once

#include "CoreMinimal.h"

#ifndef TR_DEBUG_ENABLED
	#define TR_DEBUG_ENABLED !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif

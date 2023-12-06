#pragma once

#include "CoreMinimal.h"
#include "TRConstants.h"

class UPrimitiveComponent;

namespace TR::DebugUtils
{
	#if TR_DEBUG_ENABLED
		TRCORE_API void DrawCenterOfMass(const UPrimitiveComponent* Component);
		TRCORE_API void DrawForceAtLocation(const UPrimitiveComponent* Component, const FVector& Force, const FVector& Location, const FColor& Color = FColor::Red, float Scale = 1e4);
	#else
		inline void DrawCenterOfMass(const UPrimitiveComponent* component) {}
		inline void DrawForceAtLocation(const UPrimitiveComponent* Component, const FVector& Force, const FVector& Location, const FColor& Color = FColor::Red, float Scale = 1e4) {}
#endif
}

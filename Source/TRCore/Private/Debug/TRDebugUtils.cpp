#include "Debug/TRDebugUtils.h"

#if TR_DEBUG_ENABLED

#include "Debug/TRConsoleVars.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

using namespace TR;

namespace TR::DebugUtils
{
	void DrawCenterOfMass(const UPrimitiveComponent* Component)
	{
		if (!Component || !CVarDebugTankForces.GetValueOnGameThread())
		{
			return;
		}
		auto BodyInstance = Component->GetBodyInstance();
		if (!BodyInstance)
		{
			return;
		}

		DrawDebugSphere(Component->GetWorld(), BodyInstance->GetCOMPosition(), 50, 8, FColor::Yellow, false, 0, 100);
	}

	void DrawForceAtLocation(const UPrimitiveComponent* Component, const FVector& Force, const FVector& Location, const FColor& Color, float Scale)
	{
		if (!Component || !CVarDebugTankForces.GetValueOnGameThread())
		{
			return;
		}

		const auto ForceDirection = Force.GetSafeNormal();

		DrawDebugDirectionalArrow(
			Component->GetWorld(),
			Location,
			Location + Force.Size() / Scale * ForceDirection,
			20,
			Color,
			false,
			0,
			100
		);
	}
}
#endif
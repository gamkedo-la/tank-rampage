// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankBarrelComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

void UTankBarrelComponent::Elevate(float RelativeSpeed)
{
	if (FMath::IsNearlyZero(RelativeSpeed))
	{
		return;
	}

	auto World = GetWorld();
	check(World);

	const auto ClampedRelativeSpeed = FMath::Clamp(RelativeSpeed, -1.0f, 1.0f);

	const auto DeltaSeconds = World->GetDeltaSeconds();
	const auto ElevationChange = ClampedRelativeSpeed * MaxDegreesPerSecond * DeltaSeconds;
	const auto RawNewElevation = GetRelativeRotation().Pitch + ElevationChange;
	const auto FinalElevation = FMath::Clamp(RawNewElevation, MinElevationDegrees, MaxElevationDegrees);

	SetRelativeRotation(FRotator{ FinalElevation, 0, 0 });

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Elevate - RelativeSpeed=%f; ElevationChange=%f; RawNewElevation=%f; FinalElevation=%f; DeltaTime=%fs"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), ClampedRelativeSpeed, ElevationChange, RawNewElevation, GetRelativeRotation().Pitch, DeltaSeconds);
}

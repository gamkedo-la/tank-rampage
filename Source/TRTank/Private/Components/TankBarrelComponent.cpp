// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankBarrelComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

bool UTankBarrelComponent::Elevate(float RelativeSpeed)
{
	if (FMath::IsNearlyZero(RelativeSpeed))
	{
		return false;
	}

	auto World = GetWorld();
	check(World);

	const auto ClampedRelativeSpeed = FMath::Clamp(RelativeSpeed, -1.0f, 1.0f);

	const auto DeltaSeconds = World->GetDeltaSeconds();
	const auto ElevationChange = ClampedRelativeSpeed * MaxDegreesPerSecond * DeltaSeconds;
	const auto RawNewElevation = GetRelativeRotation().Pitch + ElevationChange;
	const auto FinalElevation = FMath::Clamp(RawNewElevation, MinElevationDegrees, MaxElevationDegrees);

	const bool bChanged = !FMath::IsNearlyEqual(FinalElevation, MinElevationDegrees) && !FMath::IsNearlyEqual(FinalElevation, MaxElevationDegrees);

	// TODO: Need to detect oscillations as no change also - using TCircularBuffer

	SetRelativeRotation(FRotator{ FinalElevation, 0, 0 });

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Elevate - RotationChanged=%s; RelativeSpeed=%f; ElevationChange=%f; RawNewElevation=%f; FinalElevation=%f; DeltaTime=%fs"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), LoggingUtils::GetBoolString(bChanged), ClampedRelativeSpeed, ElevationChange, RawNewElevation, GetRelativeRotation().Pitch, DeltaSeconds);

	return bChanged;
}

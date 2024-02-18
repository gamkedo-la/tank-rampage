// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankBarrelComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

using namespace TR;

namespace
{
	constexpr float PitchChangeEpsilon = 1e-3;
}

UTankBarrelComponent::UTankBarrelComponent()
{
	bWantsInitializeComponent = true;
}

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
	const auto PreviousElevation = GetRelativeRotation().Pitch;
	const auto RawNewElevation = PreviousElevation + ElevationChange;
	const auto FinalElevation = FMath::Clamp(RawNewElevation, MinElevationDegrees, MaxElevationDegrees);
	const auto FinalElevationChange = FinalElevation - PreviousElevation;

	check(OscillationsBuffer);

	if (!FMath::IsNearlyZero(FinalElevationChange, PitchChangeEpsilon))
	{
		OscillationsBuffer->Add(FinalElevation - PreviousElevation);
	}

	const bool bOscillating = OscillationsBuffer->IsFull() && OscillationsBuffer->IsZero(OscillationThresholdDegrees);
	if (!bOscillating)
	{
		SetRelativeRotation(FRotator{ FinalElevation, 0, 0 });
	}

	const bool bChanged = !bOscillating && !FMath::IsNearlyEqual(FinalElevation, MinElevationDegrees) && !FMath::IsNearlyEqual(FinalElevation, MaxElevationDegrees);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Elevate - RotationChanged=%s; Oscillating=%s; RelativeSpeed=%f; ElevationChange=%f; RawNewElevation=%f; FinalElevation=%f; DeltaTime=%fs; CumulativeChange=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		LoggingUtils::GetBoolString(bChanged), LoggingUtils::GetBoolString(bOscillating),
		ClampedRelativeSpeed, ElevationChange, RawNewElevation, GetRelativeRotation().Pitch,
		DeltaSeconds, OscillationsBuffer->Sum());

	return bChanged;
}

void UTankBarrelComponent::InitializeComponent()
{
	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: InitializeComponent: Num Oscillation Samples=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), NumSamples);

	Super::InitializeComponent();

	if (!ensureMsgf(OscillationThresholdDegrees > 0, TEXT("OscillationThresholdDegrees=%f"), OscillationThresholdDegrees))
	{
		OscillationThresholdDegrees = 1e-3;
	}

	OscillationsBuffer = MakeUnique<BufferType>(NumSamples);
}

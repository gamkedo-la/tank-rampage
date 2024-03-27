// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankBarrelComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

using namespace TR;

namespace
{
	constexpr double PitchChangeEpsilon = 1e-3;
}

UTankBarrelComponent::UTankBarrelComponent()
{
	bWantsInitializeComponent = true;
}

bool UTankBarrelComponent::Elevate(double RelativeSpeed)
{
	if (FMath::IsNearlyZero(RelativeSpeed))
	{
		return false;
	}

	auto World = GetWorld();
	check(World);

	const auto ClampedRelativeSpeed = FMath::Clamp(RelativeSpeed, -1.0, 1.0);

	const auto DeltaSeconds = World->GetDeltaSeconds();

	const auto ElevationChange = [&]()
	{
		auto Value = ClampedRelativeSpeed * MaxDegreesPerSecond * DeltaSeconds;
		return RelativeSpeed >= 0 ? FMath::Min(RelativeSpeed, Value) : FMath::Max(RelativeSpeed, Value);
	}();

	const auto PreviousElevation = GetRelativeRotation().Pitch;
	const auto RawNewElevation = PreviousElevation + ElevationChange;
	const auto FinalElevation = FMath::Clamp(RawNewElevation, MinElevationDegrees, MaxElevationDegrees);
	const auto FinalElevationChange = FinalElevation - PreviousElevation;

	check(OscillationsBuffer);

	if (bEnableOscillationDetection && !FMath::IsNearlyZero(FinalElevationChange, PitchChangeEpsilon))
	{
		OscillationsBuffer->Add(FinalElevation - PreviousElevation);
	}

	const bool bOscillating = bEnableOscillationDetection && OscillationsBuffer->IsFull() && OscillationsBuffer->IsZero(OscillationThresholdDegrees);
	if (!bOscillating)
	{
		SetRelativeRotation(FRotator{ FinalElevation, 0, 0 });
	}

	const bool bChanged = !bOscillating && !FMath::IsNearlyEqual(FinalElevation, MinElevationDegrees) && !FMath::IsNearlyEqual(FinalElevation, MaxElevationDegrees);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Elevate - RotationChanged=%s; Oscillating=%s; InitialRelativeSpeed=%f; RelativeSpeed=%f; ElevationChange=%f; RawNewElevation=%f; FinalElevation=%f; DeltaTime=%fs; CumulativeChange=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		LoggingUtils::GetBoolString(bChanged), LoggingUtils::GetBoolString(bOscillating),
		RelativeSpeed, ClampedRelativeSpeed, ElevationChange, RawNewElevation, GetRelativeRotation().Pitch,
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

	OscillationsBuffer = MakeUnique<BufferType>(bEnableOscillationDetection ? NumSamples : 1);
}

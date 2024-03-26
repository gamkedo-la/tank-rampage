// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTurretComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	constexpr float YawChangeEpsilon = 1e-3;
}

UTankTurretComponent::UTankTurretComponent()
{
	bWantsInitializeComponent = true;
}

bool UTankTurretComponent::Rotate(double RelativeSpeed)
{
	if (FMath::IsNearlyZero(RelativeSpeed))
	{
		return false;
	}

	auto World = GetWorld();
	check(World);

	const auto ClampedRelativeSpeed = FMath::Clamp(RelativeSpeed, -1.0, 1.0);

	const auto DeltaSeconds = World->GetDeltaSeconds();
	const auto YawChange = ClampedRelativeSpeed * MaxDegreesPerSecond * DeltaSeconds;
	const auto RawYaw = GetRelativeRotation().Yaw + YawChange;
	const auto FinalYaw = RawYaw;

	check(OscillationsBuffer);
	if (bEnableOscillationDetection && !FMath::IsNearlyZero(YawChange, YawChangeEpsilon))
	{
		OscillationsBuffer->Add(YawChange);
	}

	const bool bOscillating = bEnableOscillationDetection && OscillationsBuffer->IsFull() && OscillationsBuffer->IsZero(OscillationThresholdDegrees);
	const bool bYawChange = !bOscillating && !FMath::IsNearlyZero(YawChange, YawChangeEpsilon);

	if (bYawChange)
	{
		SetRelativeRotation(FRotator{ 0, FinalYaw, 0 });
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Rotate - YawChange=%s; Oscillating=%s; RelativeSpeed=%f; YawChange=%f; RawNewYaw=%f; FinalYaw=%f; DeltaTime=%fs; CumulativeChange=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		LoggingUtils::GetBoolString(bYawChange), LoggingUtils::GetBoolString(bOscillating),
		ClampedRelativeSpeed, YawChange, RawYaw, GetRelativeRotation().Yaw,
		DeltaSeconds, OscillationsBuffer->Sum());

	return bYawChange;
}

void UTankTurretComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!ensureMsgf(OscillationThresholdDegrees > 0, TEXT("OscillationThresholdDegrees=%f"), OscillationThresholdDegrees))
	{
		OscillationThresholdDegrees = 1e-3;
	}

	OscillationsBuffer = MakeUnique<BufferType>(bEnableOscillationDetection ? NumSamples : 1);
}

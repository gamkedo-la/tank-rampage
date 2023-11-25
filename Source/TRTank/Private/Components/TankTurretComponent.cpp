// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTurretComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"


void UTankTurretComponent::Rotate(float RelativeSpeed)
{
	if (FMath::IsNearlyZero(RelativeSpeed))
	{
		return;
	}

	auto World = GetWorld();
	check(World);

	const auto ClampedRelativeSpeed = FMath::Clamp(RelativeSpeed, -1.0f, 1.0f);

	const auto DeltaSeconds = World->GetDeltaSeconds();
	const auto YawChange = ClampedRelativeSpeed * MaxDegreesPerSecond * DeltaSeconds;
	const auto RawYaw = GetRelativeRotation().Yaw + YawChange;
	const auto FinalYaw = RawYaw;

	SetRelativeRotation(FRotator{ 0, FinalYaw, 0 });

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Rotate - RelativeSpeed=%f; YawChange=%f; RawNewYaw=%f; FinalYaw=%f; DeltaTime=%fs"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), ClampedRelativeSpeed, YawChange, RawYaw, GetRelativeRotation().Yaw, DeltaSeconds);
}

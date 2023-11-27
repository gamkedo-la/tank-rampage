// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankMovementComponent.h"

#include "Components/TankTrackComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

void UTankMovementComponent::Initialize(const FInitParams& InitParams)
{
	UE_VLOG_UELOG(GetOwner(),LogTRTank, Log, TEXT("%s-%s: Initialize: %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*InitParams.ToString()
	);

	LeftTrack = InitParams.LeftTrack;
	RightTrack = InitParams.RightTrack;

	check(LeftTrack);
	check(RightTrack);
}

void UTankMovementComponent::MoveForward(float Throw)
{
	if (!LeftTrack || !RightTrack)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Warning, TEXT("%s-%s: Movement: Tracks are NULL"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	LeftTrack->SetThrottle(Throw);
	RightTrack->SetThrottle(Throw);
}

void UTankMovementComponent::TurnRight(float Throw)
{
	if (!LeftTrack || !RightTrack)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Warning, TEXT("%s-%s: Movement: Tracks are NULL"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	LeftTrack->SetThrottle(Throw);
	RightTrack->SetThrottle(-Throw);
}

FString UTankMovementComponent::FInitParams::ToString() const
{
	return FString::Printf(TEXT("LeftTrack=%s; RightTrack=%s"),
		*LoggingUtils::GetName(LeftTrack), *LoggingUtils::GetName(RightTrack));
}

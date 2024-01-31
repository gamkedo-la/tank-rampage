// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankMovementComponent.h"

#include "Components/TankTrackComponent.h"

#include "AbilitySystem/TRGameplayTags.h"

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

	if (!IsMovementAllowed())
	{
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

	if (!IsMovementAllowed())
	{
		return;
	}

	LeftTrack->SetThrottle(Throw);
	RightTrack->SetThrottle(-Throw);
}

void UTankMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	if (!IsMovementAllowed())
	{
		return;
	}

	const auto MoveDirection = MoveVelocity.GetSafeNormal();
	const auto& ForwardVector = GetOwner()->GetActorForwardVector();

	const auto ForwardThrow = MoveDirection | ForwardVector;

	MoveForward(ForwardThrow);

	const auto RightThrow = (ForwardVector ^ MoveDirection).Z;
	TurnRight(RightThrow);
}

bool UTankMovementComponent::IsMovementAllowed() const
{
	// Check if owning actor has a debuff to block movement
	if (TR::GameplayTags::HasExactTag(GetOwner(), TR::GameplayTags::MovementBlocked))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Verbose, TEXT("%s-%s: IsMovementAllowed: FALSE - By GameplayTag=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *TR::GameplayTags::MovementBlocked.ToString());

		return false;
	}

	return true;
}

FString UTankMovementComponent::FInitParams::ToString() const
{
	return FString::Printf(TEXT("LeftTrack=%s; RightTrack=%s"),
		*LoggingUtils::GetName(LeftTrack), *LoggingUtils::GetName(RightTrack));
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankMovementComponent.h"

#include "Components/TankTrackComponent.h"

#include "AbilitySystem/TRGameplayTags.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "TRConstants.h"

#include "VisualLogger/VisualLogger.h"

namespace
{
	constexpr double CmsToMph = 0.022369;
}

UTankMovementComponent::UTankMovementComponent()
{
	bUseAccelerationForPaths = TR_AI_PATH_ACCEL;
}

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
	// Called by default PathMovementComponent
	if (!IsMovementAllowed())
	{
		return;
	}

#if ENABLE_VISUAL_LOG
	LastMovementVector = MoveVelocity;
#endif

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: RequestDirectMove: MoveVelocity=%s; bForceMaxSpeed=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*MoveVelocity.ToCompactString(), LoggingUtils::GetBoolString(bForceMaxSpeed));

	MoveTo(MoveVelocity.GetSafeNormal());
}

void UTankMovementComponent::RequestPathMove(const FVector& MoveInput)
{
	// Called by CrowdFollowingComponent
	// MoveInput is a vector of length [0,1] where 1 is full strength and anything less is used for slower than max movement
	if (!IsMovementAllowed())
	{
		return;
	}

#if ENABLE_VISUAL_LOG
	LastMovementVector = MoveInput;
#endif

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: RequestPathMove: MoveInput=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *MoveInput.ToCompactString());

	MoveTo(MoveInput);
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

void UTankMovementComponent::MoveTo(const FVector& MoveDirectionStrength)
{
	const auto& ForwardVector = GetOwner()->GetActorForwardVector();

	const auto ForwardThrow = MoveDirectionStrength | ForwardVector;
	const auto RightThrow = (ForwardVector ^ MoveDirectionStrength).Z;

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: MoveTo: MoveDirectionStrength=%s; ForwardVector=%s; ForwardThrow=%.1f; RightThrow=%.1f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*MoveDirectionStrength.ToCompactString(), *ForwardVector.ToCompactString(), ForwardThrow, RightThrow);

	MoveForward(ForwardThrow);
	TurnRight(RightThrow);
}

FString UTankMovementComponent::FInitParams::ToString() const
{
	return FString::Printf(TEXT("LeftTrack=%s; RightTrack=%s"),
		*LoggingUtils::GetName(LeftTrack), *LoggingUtils::GetName(RightTrack));
}


#if ENABLE_VISUAL_LOG

void UTankMovementComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Tank Movement Component");

	const auto RawSpeed = GetComponentVelocity().Size();

	Category.Add(TEXT("Speed"), FString::Printf(TEXT("%.1f mph"), RawSpeed * CmsToMph));
	Category.Add(TEXT("Speed Pct"), FString::Printf(TEXT("%.1f"), RawSpeed / GetMaxSpeed() * 100));
	Category.Add(TEXT("AIMovementRequestSize"), FString::Printf(TEXT("%.2f"), LastMovementVector.Size()));

	Snapshot->Status.Add(Category);
}

#endif
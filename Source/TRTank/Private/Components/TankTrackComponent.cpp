// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTrackComponent.h"

#include "TankSockets.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"
#include "Debug/TRDebugUtils.h"

UTankTrackComponent::UTankTrackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BodyInstance.bNotifyRigidBodyCollision = false;
	BodyInstance.bUseCCD = false;
}

void UTankTrackComponent::SetThrottle(float InThrottle)
{
	CurrentThrottle = FMath::Clamp(InThrottle + CurrentThrottle, -1.0f, 1.0f);
}

bool UTankTrackComponent::IsGrounded() const
{
	auto World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	FHitResult HitResult;

	const auto& UpVector = GetUpVector();

	const auto StartLocation = GetComponentLocation() + UpVector * 20;
	const auto EndLocation = GetComponentLocation() - UpVector * 20;

	return World->LineTraceTestByChannel(
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Visibility,
		Params);
}

void UTankTrackComponent::DriveTrack(float Throttle)
{
	const auto& ForceLocation = GetSocketLocation(TankSockets::TreadThrottle);
	auto ForceRotation = GetSocketTransform(TankSockets::TreadThrottle, ERelativeTransformSpace::RTS_Component).GetRotation();
	if (Throttle < 0)
	{
		ForceRotation = ForceRotation.Inverse();
	}

	const auto& AdjustedLocalForward = ForceRotation.GetForwardVector();

	const auto& ForceDirection = GetComponentToWorld().TransformVector(AdjustedLocalForward);

	const auto ForceApplied = ForceDirection * Throttle * TrackMaxDrivingForce;

	auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (!RootComponent)
	{
		UE_LOG(LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to move"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwner()->GetRootComponent()));
		return;
	}

	UE_LOG(LogTRTank, Verbose, TEXT("%s-%s: SetThrottle: %f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Throttle);

	TR::DebugUtils::DrawForceAtLocation(RootComponent, ForceApplied, ForceLocation);

	RootComponent->AddForceAtLocation(ForceApplied, ForceLocation);
}

void UTankTrackComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTankTrackComponent::ApplySidewaysForce(float DeltaTime)
{
	const auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	if (!RootComponent)
	{
		UE_LOG(LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to correct for slippage"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwner()->GetRootComponent()));

		SetComponentTickEnabled(false);
		return;
	}

	const auto& RightVector = GetRightVector();
	const auto& Velocity = GetComponentVelocity();

	const auto SlippageSpeed = RightVector | Velocity;

	// Work out the required acceleration this frame to correct
	const auto CorrectionAcceleration = -SlippageSpeed / DeltaTime * RightVector;

	// Calculate and apply sideways force (F = ma)
	// Divide by 2 because there are two tracks
	auto CorrectionForce = RootComponent->GetMass() * CorrectionAcceleration * 0.5f;

	// Only correct up to the max drive force magnitude * 0.5f in direction of slippage
	const auto MaxForce = TrackMaxDrivingForce * 0.5f;
	const auto RawCorrectionForceMagnitude = CorrectionForce.Size();
	if (RawCorrectionForceMagnitude > MaxForce)
	{
		CorrectionForce /= RawCorrectionForceMagnitude / MaxForce;
	}

	TR::DebugUtils::DrawForceAtLocation(RootComponent, CorrectionForce, RootComponent->GetComponentLocation(), FColor::Orange);

	RootComponent->AddForce(CorrectionForce);
}

void UTankTrackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TR::DebugUtils::DrawCenterOfMass(Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()));

	if(IsGrounded())
	{
		DriveTrack(CurrentThrottle);
		ApplySidewaysForce(GetWorld()->GetDeltaSeconds());
	}

	CurrentThrottle = 0;
}

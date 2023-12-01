// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTrackComponent.h"

#include "TankSockets.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"
#include "VisualLogger/VisualLogger.h"

UTankTrackComponent::UTankTrackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BodyInstance.bNotifyRigidBodyCollision = true;
	BodyInstance.bUseCCD = true;
}

void UTankTrackComponent::SetThrottle(float InThrottle)
{
	CurrentThrottle = FMath::Clamp(InThrottle + CurrentThrottle, -1.0f, 1.0f);
}

void UTankTrackComponent::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	// ApplySidewaysForce(GetWorld()->GetDeltaSeconds());

	//DriveTrack(CurrentThrottle);

	//CurrentThrottle = 0;

	LastHitTimeSeconds = GetWorld()->GetTimeSeconds();
}

void UTankTrackComponent::DriveTrack(float Throttle)
{
	auto ForceApplied = GetForwardVector() * Throttle * TrackMaxDrivingForce;
	const auto& ForceLocation = GetSocketLocation(TankSockets::TreadThrottle);

	auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (!RootComponent)
	{
		UE_LOG(LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to move"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwner()->GetRootComponent()));
		return;
	}

	UE_LOG(LogTRTank, Verbose, TEXT("%s-%s: SetThrottle: %f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Throttle);

	RootComponent->AddForceAtLocation(ForceApplied, ForceLocation);
}

void UTankTrackComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
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

	const auto SlippageSpeed = RightVector | GetComponentVelocity();

	// Work out the required acceleration this frame to correct
	const auto CorrectionAcceleration = -SlippageSpeed / DeltaTime * RightVector;

	// Calculate and apply sideways force (F = ma)
	// Divide by 2 because there are two tracks
	const auto CorrectionForce = RootComponent->GetMass() * CorrectionAcceleration * 0.5f;

	RootComponent->AddForce(CorrectionForce);
}

void UTankTrackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const auto LastHitDeltaTime = GetWorld()->GetTimeSeconds() - LastHitTimeSeconds;
	if(LastHitDeltaTime <= LastHitMinDeltaTime)
	{
		ApplySidewaysForce(GetWorld()->GetDeltaSeconds());

		DriveTrack(CurrentThrottle);

		CurrentThrottle = 0;
	}
	else
	{
		UE_LOG(LogTRTank, Warning, TEXT("%s-%s: TickComponent: NotApplyingThrottle: Dt=%fs"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), LastHitDeltaTime);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankTrackComponent.h"

#include "TankSockets.h"
#include "AbilitySystem/TRGameplayTags.h"
#include "Suspension/SpringWheel.h"
#include "Components/SpawnPoint.h"

#include "Utils/CollisionUtils.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "VisualLogger/VisualLogger.h"
#include "Debug/TRDebugUtils.h"

#include "GameFramework/MovementComponent.h" 

DEFINE_VLOG_EVENT(EventTankStuck, Display, "Stuck")

namespace
{
	struct FResetGroundInfo
	{
		TR::CollisionUtils::FGroundData GroundData;
		float ResetAdditionalZOffset;
	};

	TOptional<FResetGroundInfo> GetResetGroundInfo(const AActor& Actor, const UObject* Context);
	std::tuple<FVector,float> GetFuzzyResetPosition(const AActor& Actor);
}

UTankTrackComponent::UTankTrackComponent()
{
	bWantsInitializeComponent = true;

	PrimaryComponentTick.bCanEverTick = true;
	// Affecting physics so need to tick before physics - setting explicitly to make this apparent
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;

	BodyInstance.bNotifyRigidBodyCollision = false;
	BodyInstance.bUseCCD = false;
}

void UTankTrackComponent::InitializeComponent()
{
	Super::InitializeComponent();

	check(GetOwner());

	auto MovementComponent = GetOwner()->FindComponentByClass<UMovementComponent>();

	if (MovementComponent)
	{
		// Tick after movement - note that movement is ticked after pawn player controlller which is what processes the input for the frame so will respond to throttle on same frame
		PrimaryComponentTick.AddPrerequisite(MovementComponent, MovementComponent->PrimaryComponentTick);
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Error, TEXT("%s-%s: Owner does not have a UMovementComponent available"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}

	InitStuckDetection();
}

void UTankTrackComponent::BeginPlay()
{
	Super::BeginPlay();

	auto World = GetWorld();
	check(World);

	// Next tick as the wheels get spawned on begin play
	World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::InitWheels);
}

void UTankTrackComponent::SetThrottle(float InThrottle)
{
	CurrentThrottle = FMath::Clamp(InThrottle + CurrentThrottle, -1.0f, 1.0f);

	if (HasSuspension())
	{
		DriveTrackWithSuspension(CurrentThrottle);
		CurrentThrottle = 0;
	}
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
	const auto& ReferenceLocation = GetComponentLocation();

	const auto StartLocation = ReferenceLocation + UpVector * GroundTraceExtent;
	const auto EndLocation = ReferenceLocation - UpVector * GroundTraceExtent;

	return World->LineTraceTestByChannel(
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Visibility,
		Params);
}

#if ENABLE_VISUAL_LOG

void UTankTrackComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Tank Track Component");

	const bool bSuspension = HasSuspension();

	Category.Add(TEXT("MaxDrivingForceMultiplier"), FString::Printf(TEXT("%.1f"), GetAdjustedMaxDrivingForce() / TrackMaxDrivingForce));
	Category.Add(TEXT("Suspension"), LoggingUtils::GetBoolString(bSuspension));
	if (bSuspension)
	{
		Category.Add(TEXT("Wheels"), FString::Printf(TEXT("%d"), Wheels.Num()));
	}

	Snapshot->Status.Add(Category);

	// Need to grab wheel snapshot after adding the category
	if (bSuspension)
	{
		for (auto Wheel : Wheels)
		{
			Wheel->GrabDebugSnapshot(Snapshot);
		}
	}
}

#endif

void UTankTrackComponent::DriveTrackNoSuspension( float Throttle)
{
	const auto& ForceLocation = GetSocketLocation(TankSockets::TreadThrottle);
	auto ForceRotation = GetSocketTransform(TankSockets::TreadThrottle, ERelativeTransformSpace::RTS_Component).GetRotation();
	if (Throttle < 0)
	{
		ForceRotation = ForceRotation.Inverse();
	}

	const auto& AdjustedLocalForward = ForceRotation.GetForwardVector();
	const auto& ForceDirection = GetComponentToWorld().TransformVector(AdjustedLocalForward);
	const auto ForceApplied = ForceDirection * Throttle * GetAdjustedMaxDrivingForce();

	auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (!RootComponent)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to move"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwner()->GetRootComponent()));
		return;
	}

	TR::DebugUtils::DrawForceAtLocation(RootComponent, ForceApplied, ForceLocation);

	RootComponent->AddForceAtLocation(ForceApplied, ForceLocation);

	UE_LOG(LogTRTank, VeryVerbose, TEXT("%s-%s: DriveTrackNoSuspension: Throttle=%f; ForceApplied=%s; ForceLocation=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Throttle, *ForceApplied.ToCompactString(), *ForceLocation.ToCompactString());

	// Slippage correction
	ApplySidewaysForce(GetWorld()->GetDeltaSeconds());
}

void UTankTrackComponent::DriveTrackWithSuspension(float Throttle)
{
	const auto ForceApplied = CurrentThrottle * GetAdjustedMaxDrivingForce();

	checkf(!Wheels.IsEmpty(), TEXT("%s: DriveTrackWithSuspension called with no wheels!"), *GetName());

	const auto ForcePerWheels = ForceApplied / Wheels.Num();

	for (const auto Wheel : Wheels)
	{
		check(Wheel);
		Wheel->AddDrivingForce(ForcePerWheels);
	}

	UE_LOG(LogTRTank, VeryVerbose, TEXT("%s-%s: DriveTrackWithSuspension: Throttle=%f; Wheels=%d; ForcePerWheels=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), Throttle, Wheels.Num(), ForcePerWheels);
}

bool UTankTrackComponent::HasSuspension() const
{
	return !Wheels.IsEmpty();
}

float UTankTrackComponent::GetAdjustedMaxDrivingForce() const
{
	const auto DrivingForceMultiplier = TR::GameplayTags::GetAttributeMultiplierFromTag(GetOwner(), TR::GameplayTags::SpeedMultiplier);
	const auto StuckBoostMultiplier = (bStuckBoostActive ? ThrottleBoostMultiplier : 1.0f);
	const auto AdjustedMaxDrivingForce = DrivingForceMultiplier * StuckBoostMultiplier * TrackMaxDrivingForce;

	UE_LOG(LogTRTank, VeryVerbose, TEXT("%s-%s: GetAdjustedMaxDrivingForce: multiplier=%f; AdjustedMaxDrivingForce=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), DrivingForceMultiplier, AdjustedMaxDrivingForce);

	return AdjustedMaxDrivingForce;
}

void UTankTrackComponent::InitWheels()
{
	Wheels = GetWheels();

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s: InitWheels: Found %d wheel%s"), *GetName(), Wheels.Num(), LoggingUtils::Pluralize(Wheels.Num()));

	// We do not need tick when driving with suspension
	if (HasSuspension())
	{
		// Disabling tick if we don't want to draw it to visualize the COM
#if !TR_DEBUG_ENABLED
		SetComponentTickEnabled(false);
#endif
	}
}

TArray<ASpringWheel*> UTankTrackComponent::GetWheels() const
{
	TArray<USceneComponent*> ChildComponents;
	GetChildrenComponents(true, ChildComponents);

	TArray<ASpringWheel*> DiscoveredWheels;

	// Wheels are spawned by the spawn point child components
	for (auto Component : ChildComponents)
	{
		auto WheelSpawner = Cast<USpawnPoint>(Component);
		if (!WheelSpawner)
		{
			continue;
		}

		if (auto Wheel = Cast<ASpringWheel>(WheelSpawner->GetSpawnedActor()); Wheel)
		{
			DiscoveredWheels.Add(Wheel);
		}
	}

	return DiscoveredWheels;
}

void UTankTrackComponent::InitStuckDetection()
{
	bStuckBoostActive = false;

	if (StuckCheckInterval >= 0)
	{
		bStuckCheckingEnabled = true;
		CalculatedStuckCheckInterval = StuckCheckInterval > 0 ? StuckCheckInterval : (PrimaryComponentTick.TickInterval > 0 ? PrimaryComponentTick.TickInterval : 1 / 60.0f);

		const int32 NumSamples = ThrottleSampleTime / CalculatedStuckCheckInterval;
		ThrottleBuffer.ClearAndResize(NumSamples);
		PositionBuffer.ClearAndResize(NumSamples);

		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: InitStuckDetection - Enabled: CalculatedStuckCheckInterval=%f; NumSamples=%d; StuckResetThresholdTime=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), CalculatedStuckCheckInterval, NumSamples, StuckResetThresholdTime);
	}
	else
	{
		bStuckCheckingEnabled = false;

		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: InitStuckDetection - Disabled"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
	}
}

bool UTankTrackComponent::ShouldCheckForBeingStuck() const
{
	if (!bStuckCheckingEnabled)
	{
		return false;
	}

	if (LastStuckCheckTime < 0)
	{
		return true;
	}

	auto World = GetWorld();
	check(World);

	const auto CurrentTimeSeconds = World->GetTimeSeconds();
	const auto DeltaTime = CurrentTimeSeconds - LastStuckCheckTime;

	return DeltaTime >= CalculatedStuckCheckInterval;
}

void UTankTrackComponent::CalculateStuck()
{
	// TODO: Do this in separate component and apply to both tracks

	auto World = GetWorld();
	check(World);

	LastStuckCheckTime = World->GetTimeSeconds();

	SampleStuckBuffers();

	const bool bIsStuck = IsStuck();

	if (!bStuckBoostActive && bIsStuck)
	{
		LastStuckTime = World->GetTimeSeconds();
	}
	else if (bIsStuck && StuckBeyondResetThreshold())
	{
		ResetTankTransform();
	}

	bStuckBoostActive = bIsStuck;
}

void UTankTrackComponent::SampleStuckBuffers()
{
	ThrottleBuffer.Add(CurrentThrottle);
	PositionBuffer.Add(GetComponentLocation());
}

bool UTankTrackComponent::IsStuck() const
{
	if (!ThrottleBuffer.IsFull())
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Stuck=FALSE - insufficient samples"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return false;
	}

	check(PositionBuffer.IsFull());

	// check position threshold
	const auto DeltaSize = PositionBuffer.Delta().SizeSquared();
	
	if (DeltaSize > FMath::Square(StuckDisplacementThreshold))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Stuck=FALSE - PositionDelta=%fm >= Threshold=%fm"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			FMath::Sqrt(DeltaSize) / 100,
			StuckDisplacementThreshold / 100);

		return false;
	}

	const auto ThrottleAverage = ThrottleBuffer.Average();

	if (FMath::Abs(ThrottleAverage) < ThrottleStuckDetectionThreshold)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Stuck=FALSE - PositionDelta=%fm >= Threshold=%fm but Abs(ThrottleAverage=%f) is below threshold=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			FMath::Sqrt(DeltaSize) / 100,
			StuckDisplacementThreshold / 100,
			ThrottleAverage,
			ThrottleStuckDetectionThreshold);

		return false;
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: Stuck=TRUE - PositionDelta=%fm < Threshold=%fm and Abs(ThrottleAverage=%f) is above threshold=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		FMath::Sqrt(DeltaSize) / 100,
		StuckDisplacementThreshold / 100,
		ThrottleAverage,
		ThrottleStuckDetectionThreshold);

	return true;
}

bool UTankTrackComponent::StuckBeyondResetThreshold() const
{
	auto World = GetWorld();
	check(World);

	return StuckResetThresholdTime > 0 && (World->GetTimeSeconds() - LastStuckTime > StuckResetThresholdTime);
}

void UTankTrackComponent::ResetTankTransform()
{
	auto Owner = GetOwner();
	check(Owner);

	if (auto GroundResetData = GetResetGroundInfo(*Owner, this); GroundResetData)
	{
		UE_VLOG_EVENT_WITH_DATA(Owner, EventTankStuck);
		UE_VLOG_LOCATION(Owner, LogTRTank, Log, Owner->GetActorLocation(), 25.0f, FColor::Yellow, TEXT("OriginalLocation"));
		UE_VLOG_LOCATION(Owner, LogTRTank, Log, GroundResetData->GroundData.Location, 25.0f, FColor::Green, TEXT("ResetLocation"));

		TR::CollisionUtils::ResetActorToGround(GroundResetData->GroundData, *Owner, GroundResetData->ResetAdditionalZOffset);
		ResetStuckBuffers();
	}
}

void UTankTrackComponent::ResetStuckBuffers()
{
	LastStuckTime = -1;
	ThrottleBuffer.Clear();
	PositionBuffer.Clear();
}

void UTankTrackComponent::ApplySidewaysForce(float DeltaTime)
{
	const auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	if (!RootComponent)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to correct for slippage"),
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
	const auto MaxForce = GetAdjustedMaxDrivingForce() * 0.5f;
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

	if (ShouldCheckForBeingStuck())
	{
		CalculateStuck();
	}

	if (!FMath::IsNearlyZero(CurrentThrottle) && !HasSuspension())
	{
		if (IsGrounded())
		{
			DriveTrackNoSuspension(CurrentThrottle);
		}

		CurrentThrottle = 0;
	}
}

namespace
{
	TOptional<FResetGroundInfo> GetResetGroundInfo(const AActor& Actor, const UObject* Context)
	{
		using namespace TR;

		// First attempt to randomly offset the reset location
		const auto [FuzzyResetPosition, ActorHalfHeight] = GetFuzzyResetPosition(Actor);

		auto GroundData = CollisionUtils::GetGroundData(Actor, FuzzyResetPosition);
		if (!GroundData)
		{
			UE_VLOG_UELOG(&Actor, LogTRTank, Log,
				TEXT("GetResetGroundInfo - %s-%s: Could not determine ground location at offset position=%s"),
				*Actor.GetName(), *LoggingUtils::GetName(Context), *FuzzyResetPosition.ToCompactString());
			UE_VLOG_LOCATION(&Actor, LogTRTank, Log, FuzzyResetPosition, 25.0f, FColor::Red, TEXT("FuzzyResetPosition"));

			// try the actor location
			GroundData = CollisionUtils::GetGroundData(Actor);
		}

		if (!GroundData)
		{
			UE_VLOG_UELOG(&Actor, LogTRTank, Warning,
				TEXT("GetResetGroundInfo - %s-%s: Could not determine ground location"),
				*Actor.GetName(), *LoggingUtils::GetName(Context));

			return {};
		}

		// Adjust the position up a bit to give the tank more momentum to get unstuck
		return FResetGroundInfo
		{
			.GroundData = *GroundData,
			.ResetAdditionalZOffset = ActorHalfHeight
		};
	}

	std::tuple<FVector, float> GetFuzzyResetPosition(const AActor& Actor)
	{
		// Get randomly within actor bounds
		const auto& TankBounds = TR::CollisionUtils::GetAABB(Actor);

		FVector Center, Extent;
		TankBounds.GetCenterAndExtents(Center, Extent);

		// Move up a bit so we are less likely to hit an obstacle
		Center.Z += Extent.Z;

		const auto RespawnExtent = FBox::BuildAABB(Center, Extent);

		return
		{
			FMath::RandPointInBox(RespawnExtent),
			Extent.Z
		};
	}
}

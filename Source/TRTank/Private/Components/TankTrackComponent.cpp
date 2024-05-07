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
	const FString TrackWheelSocketNamePrefix = "Wheel_";

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

	// Ensure first check for airborne cooldown succeeds without needing to explicitly check for < 0
	LastAirborneTime = -CounterAirborneCooldownTime - 0.01f;
	LastCounterTime = -CounterMangitudeMinInterval - 0.01f;

	check(GetOwner());

	SetNotifyRigidBodyCollision(true);

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
	InitTrackWheels();
}

void UTankTrackComponent::NotifyRelevantTankCollision(const FHitResult& Hit, const FVector& NormalImpulse)
{
	if (HasSuspension())
	{
		return;
	}

	auto World = GetWorld();
	check(World);

	const auto TimeSeconds = World->GetTimeSeconds();

	// countering is in cooldown and being airborne most likely not the result of the countering itself
	const auto LastCounterDeltaTime = TimeSeconds - LastCounterTime;

	if (LastCounterDeltaTime < CounterMangitudeMinInterval)
	{
		return;
	}

	const auto LastAirborneDeltaTime = TimeSeconds - LastAirborneTime;
	
	if (LastAirborneDeltaTime <= CounterAirborneCooldownTime && LastCounterDeltaTime > CounterAirborneCooldownTime)
	{
		return;
	}

	const auto& UpVector = GetUpVector();
	const auto DotProduct = UpVector | Hit.Normal;

	if (DotProduct >= RoadAlignmentCosineThreshold)
	{
		return;
	}

	// check impulse size
	const auto ImpulseSize = NormalImpulse.Size();

	if (ImpulseSize < CounterMangitudeThreshold)
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
		TEXT("%s-%s: NotifyRelevantTankCollision:  Normal DotProduct=%f < RoadAlignmentCosineThreshold=(%f); NormalImpulse=%s; Component=%s; Actor=%s; ObjectType=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		DotProduct,
		RoadAlignmentCosineThreshold,
		*NormalImpulse.ToCompactString(),
		*LoggingUtils::GetName(Hit.GetComponent()),
		*LoggingUtils::GetName(Hit.GetActor()),
		Hit.GetComponent() ? *LoggingUtils::GetName(Hit.GetComponent()->GetCollisionObjectType()) : TEXT("NULL")
	);

	// counteract the normal impulse at the location

	const auto CounterImpulseMagnitude = CounterMagnitudeMaxValue;

	const auto UpForceLocation = GetOwner()->GetActorLocation();
	const auto UpImpulse = UpVector * CounterImpulseMagnitude;
	AddImpulseAtLocation(UpImpulse, UpForceLocation);

	TR::DebugUtils::DrawForceAtLocation(this, UpImpulse, UpForceLocation, FColor::Orange);
	UE_VLOG_ARROW(GetOwner(), LogTRTank, Log, UpForceLocation, UpForceLocation + UpVector * 600, FColor::Orange, TEXT("Road Correction"));

	// Apply additional impulse at the track component location in direction of throttle
	if (!FMath::IsNearlyZero(LastThrottle))
	{
		const auto ImpulseDirection = FMath::Sign(LastThrottle) * GetForwardVector();
		const auto DirectionalImpulse = ImpulseDirection * CounterImpulseMagnitude;
		const auto& DirectionalImpulseLocation = GetComponentLocation();

		AddImpulseAtLocation(DirectionalImpulse, DirectionalImpulseLocation);

		TR::DebugUtils::DrawForceAtLocation(this, DirectionalImpulse, DirectionalImpulseLocation, FColor::Orange);
		UE_VLOG_ARROW(GetOwner(), LogTRTank, Log, DirectionalImpulseLocation, DirectionalImpulseLocation + ImpulseDirection * 600, FColor::Orange, TEXT("Road Correction"));
	}

	LastCounterTime = TimeSeconds;
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
	RecordThrottle(InThrottle);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: SetThrottle: %f"), *LoggingUtils::GetName(GetOwner()), *GetName(), CurrentThrottle);

	if (HasSuspension())
	{
		DriveTrackWithSuspension(CurrentThrottle);
		ClearThrottle();
	}
}

bool UTankTrackComponent::IsGrounded() const
{
	if (TrackWheels.IsEmpty())
	{
		return IsGroundedFallback();
	}

	return TrackWheels.ContainsByPredicate([](const auto& Wheel) { return Wheel.bGrounded;  });
}

void UTankTrackComponent::DriveTrackNoSuspension(float Throttle)
{
	auto RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (!RootComponent)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Error, TEXT("%s-%s: Owner root component %s is not a primitive component - unable to move"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(GetOwner()->GetRootComponent()));
		return;
	}

	if (!TrackWheels.IsEmpty())
	{
		int32 GroundedCount = 0;
		for (const auto& Wheel : TrackWheels)
		{
			if (Wheel.bGrounded)
			{
				++GroundedCount;
			}
		}

		// Should never enter this function without at least one grounded wheel
		checkf(GroundedCount, TEXT("Entered DriveTrackNoSuspension when not grounded!"));

		const auto WheelMultiplier = 1.0f / FMath::Max(GroundedCount, TrackWheels.Num() * 0.5f);

		for (const auto& Wheel : TrackWheels)
		{
			if (Wheel.bGrounded)
			{
				DriveTrackNoSuspension(Throttle, Wheel.SocketName, *RootComponent, WheelMultiplier);
			}
		}
	}
	else
	{
		// Old system
		DriveTrackNoSuspension(Throttle, TankSockets::TreadThrottle, *RootComponent, 1.0f);
	}

	// Slippage correction
	ApplySidewaysForce(GetWorld()->GetDeltaSeconds());
}

void UTankTrackComponent::DriveTrackNoSuspension(float Throttle, const FName& ForceSocket, UPrimitiveComponent& PrimitiveComponent, float ForceMultiplier)
{
	const auto& ForceLocation = GetSocketLocation(ForceSocket);
	auto ForceRotation = GetSocketTransform(ForceSocket, ERelativeTransformSpace::RTS_Component).GetRotation();
	if (Throttle < 0)
	{
		ForceRotation = ForceRotation.Inverse();
	}

	const auto& AdjustedLocalForward = ForceRotation.GetForwardVector();
	const auto& ForceDirection = GetComponentToWorld().TransformVector(AdjustedLocalForward);
	const auto ForceApplied = ForceDirection * Throttle * GetAdjustedMaxDrivingForce() * ForceMultiplier;

	TR::DebugUtils::DrawForceAtLocation(&PrimitiveComponent, ForceApplied, ForceLocation);

	PrimitiveComponent.AddForceAtLocation(ForceApplied, ForceLocation);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: DriveTrackNoSuspension: ForceSocket=%s; Throttle=%f; ForceApplied=%s; ForceLocation=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *ForceSocket.ToString(), Throttle, *ForceApplied.ToCompactString(), *ForceLocation.ToCompactString());
}

void UTankTrackComponent::RecordThrottle(float Value)
{
	CurrentThrottle = LastThrottle = FMath::Clamp(Value + CurrentThrottle, -1.0f, 1.0f);
}

void UTankTrackComponent::ClearThrottle()
{
	// Clear "LastThrottle" one frame behind so that external dependencies can query the value
	if (FMath::IsNearlyZero(CurrentThrottle))
	{
		LastThrottle = 0;
	}

	CurrentThrottle = 0;
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

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: DriveTrackWithSuspension: Throttle=%f; Wheels=%d; ForcePerWheels=%f"),
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

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: GetAdjustedMaxDrivingForce: multiplier=%f; AdjustedMaxDrivingForce=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), DrivingForceMultiplier, AdjustedMaxDrivingForce);

	return AdjustedMaxDrivingForce;
}

void UTankTrackComponent::InitWheels()
{
	Wheels = GetWheels();

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s: InitWheels: Found %d wheel%s"), *GetName(), Wheels.Num(), LoggingUtils::Pluralize(Wheels.Num()));
}

void UTankTrackComponent::InitTrackWheels()
{
	const TArray<FName> AllSocketNames = GetAllSocketNames();

	for (const auto& SocketName : AllSocketNames)
	{
		const FString SocketString = SocketName.ToString();
		if (SocketString.StartsWith(TrackWheelSocketNamePrefix))
		{
			TrackWheels.Add(FTrackWheel
			{
				.SocketName = SocketName
			});
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s: InitTrackWheels: Found %d virtual wheel%s"), *GetName(), TrackWheels.Num(), LoggingUtils::Pluralize(TrackWheels.Num()));
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

bool UTankTrackComponent::ShouldRecalculateGrounded() const
{
	const auto World = GetWorld();
	check(World);

	return !HasSuspension() && !TrackWheels.IsEmpty() && (LastGroundTraceTime < 0 || (World->GetTimeSeconds() - LastGroundTraceTime >= GroundTraceInterval));
}

void UTankTrackComponent::CalculateGrounded()
{
	const auto World = GetWorld();
	check(World);

	LastGroundTraceTime = World->GetTimeSeconds();

	for (auto& Wheel : TrackWheels)
	{
		const auto& Transform = GetSocketTransform(Wheel.SocketName);

		Wheel.bGrounded = IsGroundedLocation(Transform.GetLocation(), Transform.GetRotation().GetUpVector());
	}
}

bool UTankTrackComponent::IsGroundedFallback() const
{
	return IsGroundedLocation(GetComponentLocation(), GetUpVector());
}

bool UTankTrackComponent::IsGroundedLocation(const FVector& WorldLocation, const FVector& WorldUpVector) const
{
	auto World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	const auto StartLocation = WorldLocation + WorldUpVector * GroundTraceExtent;
	const auto EndLocation = WorldLocation - WorldUpVector * GroundTraceExtent;

#if ENABLE_VISUAL_LOG

	if (FVisualLogger::IsRecording())
	{
		FHitResult HitResult;

		const bool bResult = World->LineTraceSingleByChannel(
			HitResult,
			StartLocation,
			EndLocation,
			ECollisionChannel::ECC_Visibility,
			Params);

		if (bResult)
		{
			UE_VLOG_SEGMENT(GetOwner(), LogTRTank, Verbose, StartLocation, HitResult.Location, FColor::Green, TEXT("Ground"));
			UE_VLOG_BOX(GetOwner(), LogTRTank, Verbose, FBox::BuildAABB(HitResult.Location, FVector{ 5.0f, 5.0f, 5.0f }), FColor::Red, TEXT(""));
		}
		else
		{
			UE_VLOG_SEGMENT(GetOwner(), LogTRTank, Verbose, StartLocation, EndLocation, FColor::Red, TEXT("No Ground"));
		}

		return bResult;
	}
#endif

		return World->LineTraceTestByChannel(
			StartLocation,
			EndLocation,
			ECollisionChannel::ECC_Visibility,
			Params);
}

void UTankTrackComponent::ApplySidewaysForce(float DeltaTime)
{
	// Don't apply if in stuck
	if (bStuckBoostActive)
	{
		return;
	}

	check(GetOwner());

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

	if (ShouldRecalculateGrounded())
	{
		CalculateGrounded();
	}

	const bool bGrounded = IsGrounded();

	if (ShouldCheckForBeingStuck())
	{
		CalculateStuck();
	}

	if (bGrounded && !FMath::IsNearlyZero(CurrentThrottle) && !HasSuspension())
	{
		DriveTrackNoSuspension(CurrentThrottle);
	}

	// update last airborne time
	if (!bGrounded)
	{
		auto World = GetWorld();
		check(World);

		LastAirborneTime = World->GetTimeSeconds();
	}

	ClearThrottle();
}

#if ENABLE_VISUAL_LOG

void UTankTrackComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("Tank Track Component (%s)"), *GetName());

	const bool bSuspension = HasSuspension();

	Category.Add(TEXT("Throttle"), FString::Printf(TEXT("%.1f"), LastThrottle));
	Category.Add(TEXT("MaxDrivingForceMultiplier"), FString::Printf(TEXT("%.1f"), GetAdjustedMaxDrivingForce() / TrackMaxDrivingForce));
	Category.Add(TEXT("Grounded"), LoggingUtils::GetBoolString(IsGrounded()));
	Category.Add(TEXT("LastAirborneTime"), LastAirborneTime >= 0 ? FString::Printf(TEXT("%.1f"), LastAirborneTime) : TEXT("N/A"));
	Category.Add(TEXT("Suspension"), LoggingUtils::GetBoolString(bSuspension));
	Category.Add(TEXT("Wheels"), FString::Printf(TEXT("%d"), bSuspension ? Wheels.Num() : TrackWheels.Num()));

	Snapshot->Status.Add(Category);

	// Need to grab wheel snapshot after adding the category
	if (bSuspension)
	{
		for (auto Wheel : Wheels)
		{
			Wheel->GrabDebugSnapshot(Snapshot);
		}
	}
	else
	{
		for (const auto& TrackWheelInfo : TrackWheels)
		{
			TrackWheelInfo.GrabDebugSnapshot(Snapshot);
		}
	}
}

void UTankTrackComponent::FTrackWheel::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("Wheel (%s)"), *SocketName.ToString());

	Category.Add(TEXT("Grounded"), LoggingUtils::GetBoolString(bGrounded));

	// Push the category to correct nesting in the visual logger details panel
	auto& StatusArray = Snapshot->Status;
	// Should have called Snapshot->Status.Add(Category) prior to calling this member function
	check(!StatusArray.IsEmpty());

	// Nest under parent
	StatusArray.Last().AddChild(Category);
}

#endif

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

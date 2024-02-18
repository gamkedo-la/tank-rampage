// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankAimingComponent.h"

#include "TankSockets.h"
#include "Components/TankTurretComponent.h"
#include "Components/TankBarrelComponent.h"
#include "Interfaces/ArmedActor.h"

#include "AbilitySystem/TRGameplayTags.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"

namespace
{
	float ClampDeltaYaw(float Yaw);
}

UTankAimingComponent::UTankAimingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	bWantsInitializeComponent = true;
}

void UTankAimingComponent::SetTankComponents(const FTankComponents& TankComponents)
{
	Barrel = TankComponents.Barrel;
	Turret = TankComponents.Turret;

	check(Barrel);
	check(Turret);
}

void UTankAimingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTankAimingComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UE_LOG(LogTRTank, Log, TEXT("%s-%s: InitializeComponent"), *LoggingUtils::GetName(GetOwner()), *GetName());

	CurrentAimingMode = DefaultAimingMode;

	AimToleranceCosine = FMath::Cos(FMath::DegreesToRadians(AimToleranceDegrees));
	AssistedAimTargetThresholdDistSq = FMath::Square(AssistedAimTargetThresholdDistanceMeters * 100);
}

void UTankAimingComponent::AimAt(const FAimingData& AimingData, float LaunchSpeed)
{
	check(Barrel && Turret);

	if (!IsAimingAllowed())
	{
		return;
	}

	switch (CurrentAimingMode)
	{
	case EAimingMode::AssistedAim:
		AssistedAimAt(AimingData, LaunchSpeed);
		break;
	case EAimingMode::ManualAim:
		DirectAimAt(AimingData);
		break;
	default:
		UE_VLOG_UELOG(GetOwner(),LogTRTank,Error,TEXT("Unknown Aiming Mode Hit"));
		break;
	}
}
void UTankAimingComponent::AimAtWithNoLaunchSpeed(const FAimingData& AimingData)
{
	check(Barrel && Turret);

	if (!IsAimingAllowed())
	{
		return;
	}

	// Don't account for launch speed as there is none
	DirectAimAt(AimingData);
}

void UTankAimingComponent::AssistedAimAt(const FAimingData& AimingData, float LaunchSpeed)
{
	const auto AimDirectionOptional = GetAssistedAimDirection(AimingData, LaunchSpeed);
	if (!AimDirectionOptional)
	{
		FiringStatus = ETankFiringStatus::NoTarget;

		UE_VLOG_UELOG(GetOwner(), LogTRTank, Verbose, TEXT("%s-%s: AssistedAimAt - FAILED: Location=%s from barrelLocation=%s at LaunchSpeed=%f m/s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			*AimingData.HitLocation.ToCompactString(), *Barrel->GetComponentLocation().ToCompactString(), LaunchSpeed / 100);
		return;
	}

	const auto& AimDirection = *AimDirectionOptional;

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: AssistedAimAt - SUCCESS: Location=%s from barrelLocation=%s at LaunchSpeed=%f m/s with AimDirection=%s"),
	              *LoggingUtils::GetName(GetOwner()), *GetName(),
	              *AimingData.HitLocation.ToCompactString(), *Barrel->GetComponentLocation().ToCompactString(), LaunchSpeed / 100, *AimDirection.ToCompactString());

	MoveBarrelTowards(AimingData.HitLocation, AimDirection);
}

std::optional<FVector> UTankAimingComponent::GetAssistedAimDirection(const FAimingData& AimingData, float LaunchSpeed) const
{
	if (!AimingData.bHitResult)
	{
		return std::nullopt;
	}

	const auto FireLocation = GetBarrelLocation();

	// If we are within the threshold distance of target, don't calculate the projectile velocity, just aim directly at intended target
	if (const auto ToFireLocation = (AimingData.HitLocation - FireLocation); ToFireLocation.SizeSquared() <= AssistedAimTargetThresholdDistSq)
	{
		const auto AimDirection = ToFireLocation.GetSafeNormal();

		UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: GetAssistedAimDirection - Skipping projectile calculation as distance=%fm within %fm; aim direction=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			ToFireLocation.Size() / 100, AssistedAimTargetThresholdDistanceMeters, *AimDirection.ToCompactString());

		return AimDirection;
	}

	FVector OutProjectileVelocity{ EForceInit::ForceInitToZero };

	const bool bSolutionFound = UGameplayStatics::SuggestProjectileVelocity(
		this,
		OutProjectileVelocity,
		FireLocation,
		AimingData.HitLocation,
		LaunchSpeed,
		false,
		0.0f,
		0.0f,
		ESuggestProjVelocityTraceOption::DoNotTrace,
		{},
		{},
		false);

	if (bSolutionFound)
	{
		const auto AimDirection = OutProjectileVelocity.GetSafeNormal();

		UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: GetAssistedAimDirection - projectile calculation adjusted aim direction from %s to %s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			*(AimingData.HitLocation - FireLocation).GetSafeNormal().ToCompactString(), *AimDirection.ToCompactString());

		return AimDirection;
	}
	else
	{
		return std::nullopt;
	}
}

FVector UTankAimingComponent::GetBarrelLocation() const
{
	return Barrel->GetSocketLocation(TankSockets::GunFire);
}

void UTankAimingComponent::DirectAimAt(const FAimingData& AimingData)
{
	const FVector FireOriginLocation = GetBarrelLocation();
	const FVector TargetLocation = AimingData.AimingOriginWorldLocation + AimingData.AimingWorldDirection * ZeroingDistance;
	const FVector AimDirection = (TargetLocation - FireOriginLocation).GetSafeNormal();

	MoveBarrelTowards(TargetLocation, AimDirection);
}

void UTankAimingComponent::MoveBarrelTowards(const FVector& Target, const FVector& AimDirection)
{
	check(Barrel && Turret);

	if (IsBarrelAlreadyAtTarget(Target, AimDirection))
	{
		FiringStatus = ETankFiringStatus::Locked;
		return;
	}

	const auto TargetRotation = AimDirection.Rotation();
	const auto BarrelRotator = Barrel->GetForwardVector().Rotation();

	const auto DesiredRotator = TargetRotation - Barrel->GetForwardVector().Rotation();

	bool bMoved = Barrel->Elevate(DesiredRotator.Pitch);
	bMoved |= Turret->Rotate(ClampDeltaYaw(DesiredRotator.Yaw));

	if (!bMoved)
	{
		FiringStatus = ETankFiringStatus::Locked;
	}
	else if(auto ArmedOwner = Cast<IArmedActor>(GetOwner()); ArmedOwner && ArmedOwner->CanFire())
	{
		FiringStatus = ETankFiringStatus::Aiming;
	}
	else
	{
		FiringStatus = ETankFiringStatus::Reloading;
	}
}

bool UTankAimingComponent::IsBarrelAlreadyAtTarget(const FVector& Target, const FVector& AimDirection) const
{
	const float Alignment = AimDirection | Barrel->GetForwardVector();
	bool bAtTarget = Alignment >= AimToleranceCosine;

	if (!bAtTarget)
	{
		// check arc length
		const auto ArcError = FVector::Distance(Target, GetBarrelLocation()) * FMath::Acos(Alignment);
		if (ArcError <= AssistedAimArcLengthErrorThresholdMeters * 100)
		{
			bAtTarget = true;
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: IsBarrelAlreadyAtTarget: %s - Error Angle=%f; ArcError=%fm; AimDirection=%s; BarrelDirection=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		LoggingUtils::GetBoolString(bAtTarget),
		FMath::RadiansToDegrees(FMath::Acos(Alignment)),
		FVector::Distance(Target, GetBarrelLocation()) * FMath::Acos(Alignment) / 100,
		*AimDirection.ToCompactString(), *Barrel->GetForwardVector().ToCompactString()
	);

	return bAtTarget;
}

bool UTankAimingComponent::IsAimingAllowed() const
{
	// Check if owning actor has a debuff to block aiming
	if (TR::GameplayTags::HasExactTag(GetOwner(), TR::GameplayTags::AimBlocked))
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Verbose, TEXT("%s-%s: IsAimingAllowed: FALSE - By GameplayTag=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *TR::GameplayTags::AimBlocked.ToString());

		return false;
	}

	return true;
}

void UTankAimingComponent::SetTankAimingMode(EAimingMode NewAimingMode)
{
	CurrentAimingMode = NewAimingMode;
}

#if ENABLE_VISUAL_LOG

void UTankAimingComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Tank Aiming Component");

	Category.Add(TEXT("Firing Status"), *LoggingUtils::GetName(FiringStatus));
	Snapshot->Status.Add(Category);
}

#endif

namespace
{
	float ClampDeltaYaw(float Yaw)
	{
		auto ClampedAngle = FMath::Fmod(Yaw + 180.0f, 360.0f);
		if (ClampedAngle < 0.0f)
		{
			ClampedAngle += 360.0f;
		}

		ClampedAngle -= 180.0f;

		return ClampedAngle;
	}
}

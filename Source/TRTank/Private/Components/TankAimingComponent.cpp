// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankAimingComponent.h"

#include "TankSockets.h"
#include "Components/TankTurretComponent.h"
#include "Components/TankBarrelComponent.h"

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
	PrimaryComponentTick.bCanEverTick = true;
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


void UTankAimingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UTankAimingComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: InitializeComponent"), *LoggingUtils::GetName(GetOwner()), *GetName());
}

void UTankAimingComponent::AimAt(const FVector& Location, float LaunchSpeed)
{
	check(Barrel && Turret);

	FVector ProjectileVelocity{ EForceInit::ForceInitToZero };

	const auto FireLocation = Barrel->GetSocketLocation(TankSockets::GunFire);

	const bool bSolutionFound = UGameplayStatics::SuggestProjectileVelocity(
		this,
		ProjectileVelocity,
		FireLocation,
		Location,
		LaunchSpeed,
		false,
		0.0f,
		0.0f,
		ESuggestProjVelocityTraceOption::DoNotTrace,
		{},
		{},
		true);

	const auto AimDirection = ProjectileVelocity.GetSafeNormal();

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: AimAt - %s: Location=%s from barrelLocation=%s at LaunchSpeed=%f m/s with AimDirection=%AimDirection"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		LoggingUtils::GetBoolString(bSolutionFound),
		*Location.ToCompactString(), *Barrel->GetComponentLocation().ToCompactString(), LaunchSpeed / 100, *AimDirection.ToCompactString());

	if (bSolutionFound)
	{
		MoveBarrelTowards(AimDirection);
	}
}

void UTankAimingComponent::MoveBarrelTowards(const FVector& AimDirection) const
{
	check(Barrel && Turret);

	const auto TargetRotation = AimDirection.Rotation();
	const auto BarrelRotator = Barrel->GetForwardVector().Rotation();

	const auto DesiredRotator = TargetRotation - Barrel->GetForwardVector().Rotation();

	// TODO: AimDeadZoneDegrees prevents violent shaking - possibly this is a rotation centering issue in model or the socket
	if (FMath::Abs(DesiredRotator.Pitch) > AimDeadZoneDegrees)
	{
		Barrel->Elevate(DesiredRotator.Pitch);
	}

	if (FMath::Abs(DesiredRotator.Yaw) > AimDeadZoneDegrees)
	{
		Turret->Rotate(ClampDeltaYaw(DesiredRotator.Yaw));
	}
}

#if ENABLE_VISUAL_LOG

void UTankAimingComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Tank Aiming Component");

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

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankAimingComponent.h"

#include "TankSockets.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"

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
	// TODO: Move at given rotate and pitch speed

	check(Barrel && Turret);

	const auto BarrelRotationDelta = AimDirection.Rotation() - Barrel->GetForwardVector().Rotation();

	const FRotator BarrelRotation = FRotator{ BarrelRotationDelta.Pitch, 0, 0 };
	Barrel->AddWorldRotation(BarrelRotation);

	const auto TurretRotationDelta = AimDirection.Rotation() - Turret->GetForwardVector().Rotation();

	const FRotator TurretRotation = FRotator{ 0, TurretRotationDelta.Yaw, 0 };
	Turret->AddWorldRotation(TurretRotation);
}

#if ENABLE_VISUAL_LOG

void UTankAimingComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Tank Aiming Component");

	Snapshot->Status.Add(Category);
}

#endif

// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/BaseTankPawn.h"

#include "Components/TankAimingComponent.h"
#include "Components/TankTurretComponent.h"
#include "Components/TankBarrelComponent.h"
#include "Components/TankTrackComponent.h"
#include "Components/TankMovementComponent.h"
#include "Components/HealthComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

#include "TankSockets.h"
#include "Projectile.h"

#include "Subsystems/TankEventsSubsystem.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "AbilitySystem/TRAbilitySystemComponent.h" 
#include "AbilitySystem/TRAttributeSet.h"

namespace
{
	FBox GetBounds(const UStaticMeshComponent& Comp, const FName* SocketName = nullptr);
	FVector GetSocketRelativeLocation(const UStaticMeshComponent& Comp, const FName& SocketName);
}

ABaseTankPawn::ABaseTankPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = TankBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBody"));
	TankBody->SetSimulatePhysics(true);
	TankBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	TankTurret = CreateDefaultSubobject<UTankTurretComponent>(TEXT("TankTurret"));
	TankTurret->SetupAttachment(TankBody, TankSockets::TurretAttach);

	TankBarrel = CreateDefaultSubobject<UTankBarrelComponent>(TEXT("TankBarrel"));
	TankBarrel->SetupAttachment(TankTurret, TankSockets::GunAttach);

	TankTreadRight = CreateDefaultSubobject<UTankTrackComponent>(TEXT("TankTreadRight"));
	TankTreadRight->SetupAttachment(TankBody, TankSockets::TreadRightAttach);

	TankTreadLeft = CreateDefaultSubobject<UTankTrackComponent>(TEXT("TankTreadLeft"));
	TankTreadLeft->SetupAttachment(TankBody, TankSockets::TreadLeftAttach);

	TankTreadSideRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideRight"));
	TankTreadSideRight->SetupAttachment(TankBody, TankSockets::SideRightAttach);

	TankTreadSideLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideLeft"));
	TankTreadSideLeft->SetupAttachment(TankBody, TankSockets::SideLeftAttach);

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(TankBody);

	// Only enable for player controller
	CameraSpringArm->PrimaryComponentTick.bStartWithTickEnabled = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);

	TankAimingComponent = CreateDefaultSubobject<UTankAimingComponent>(TEXT("TankAimingComponent"));
	TankMovementComponent = CreateDefaultSubobject<UTankMovementComponent>(TEXT("TankMovement"));
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	AbilitySystemComponent = CreateDefaultSubobject<UTRAbilitySystemComponent>(TEXT("Ability System"));
	AbilitySystemComponent->SetIsReplicated(true);

	AttributeSet = CreateDefaultSubobject<UTRAttributeSet>(TEXT("Attribute Set"));
}

// Called when the game starts or when spawned
void ABaseTankPawn::BeginPlay()
{
	Super::BeginPlay();

	// Cannot call this in the constructor
	TankBody->SetMassOverrideInKg(NAME_None, 40000);
}

void ABaseTankPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	TankAimingComponent->SetTankComponents(
		{
				.Barrel = TankBarrel,
				.Turret = TankTurret
		});

	TankMovementComponent->Initialize(
		{
			.LeftTrack = TankTreadLeft,
			.RightTrack = TankTreadRight
		});
}

void ABaseTankPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	UpdateSpringArmTickEnabled();
}

void ABaseTankPawn::UpdateSpringArmTickEnabled()
{
	const bool bIsLocalPlayerController = IsPlayerControlled() && IsLocallyControlled();

	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: UpdateSpringArmTickEnabled - bIsLocalPlayerController=%s"), *GetName(),
		LoggingUtils::GetBoolString(bIsLocalPlayerController));

	// keep ticking spring component once possessed by a local player controller
	if (bIsLocalPlayerController)
	{
		check(CameraSpringArm);

		CameraSpringArm->SetComponentTickEnabled(bIsLocalPlayerController);
	}
}

bool ABaseTankPawn::CanFire() const
{
	auto World = GetWorld();
	check(World);

	if (LastFireTimeSeconds < 0)
	{
		return true;
	}

	return World->GetTimeSeconds() - LastFireTimeSeconds > FireCooldownTimeSeconds;
}

float ABaseTankPawn::GetFireCooldownTimeRemaining() const
{
	UWorld* World = GetWorld();
	check(World);
	float CurrentTime = World->GetTimeSeconds();
	float TimeElapsedSinceLastFired = CurrentTime - LastFireTimeSeconds;
	bool bIsFireOnCooldown =  TimeElapsedSinceLastFired < FireCooldownTimeSeconds;
	return bIsFireOnCooldown ? FireCooldownTimeSeconds - TimeElapsedSinceLastFired : 0.f;
}

float ABaseTankPawn::GetFireCooldownProgressPercentage() const
{
	UWorld* World = GetWorld();
	check(World);
	float CurrentTime = World->GetTimeSeconds();
	float TimeElapsedSinceLastFired = CurrentTime - LastFireTimeSeconds;
	bool bIsFireOnCooldown =  TimeElapsedSinceLastFired < FireCooldownTimeSeconds;
	float ProgressPercentage =  TimeElapsedSinceLastFired/FireCooldownTimeSeconds;
	return bIsFireOnCooldown ? ProgressPercentage : 1.f;
}

void ABaseTankPawn::AimAt(const FAimingData& AimingData)
{
	TankAimingComponent->AimAt(AimingData, TankShellSpeed);
}

void ABaseTankPawn::Fire()
{
	check(TankBarrel);

	auto World = GetWorld();
	if (!World)
	{
		UE_VLOG_UELOG(this, LogTRTank, Error, TEXT("%s: Fire: World is NULL"), *GetName());
		return;
	}

	if (!MainGunProjectile)
	{
		UE_VLOG_UELOG(this, LogTRTank, Error, TEXT("%s: Fire: MainGunProjectile is NULL"), *GetName());
		return;
	}

	if (!CanFire())
	{
		return;
	}

	const FVector SpawnLocation = TankBarrel->GetSocketLocation(TankSockets::GunFire);
	const FRotator SpawnRotation = TankBarrel->GetSocketRotation(TankSockets::GunFire);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = this;
	SpawnParameters.Owner = this;

	auto SpawnedProjectile = World->SpawnActor<AProjectile>(MainGunProjectile, SpawnLocation, SpawnRotation, SpawnParameters);

	if (!SpawnedProjectile)
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning, TEXT("%s: Fire: Unable to spawn projectile %s at %s with rotation=%s"), *GetName(), *LoggingUtils::GetName(MainGunProjectile), *SpawnLocation.ToCompactString());
		return;
	}

	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: Fire: %s at %s"), *GetName(), *LoggingUtils::GetName(MainGunProjectile), *SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	check(TankBarrel);

	SpawnedProjectile->Initialize(*TankBarrel, TankSockets::GunFire);
	SpawnedProjectile->Launch(TankShellSpeed);

	LastFireTimeSeconds = World->GetTimeSeconds();
}

void ABaseTankPawn::SetLeftThrottle(float Value)
{
	check(TankTreadLeft);
	TankTreadLeft->SetThrottle(Value);
}

void ABaseTankPawn::SetRightThrottle(float Value)
{
	check(TankTreadRight);
	TankTreadRight->SetThrottle(Value);
}

void ABaseTankPawn::MoveForward(float Throw)
{
	check(TankMovementComponent);
	TankMovementComponent->MoveForward(Throw);
}

void ABaseTankPawn::TurnRight(float Throw)
{
	check(TankMovementComponent);
	TankMovementComponent->TurnRight(Throw);
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void ABaseTankPawn::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	// helpful article about the Visual Logger
	// https://benui.ca/unreal/visual-logger/

	if (!TankBody || !TankTurret || !TankTreadLeft || !TankTreadRight || !TankAimingComponent)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto CurrentTimeSeconds = World->GetTimeSeconds();

	// Get reference to the current category
	const int32 CatIndex = Snapshot->Status.AddZeroed();
	FVisualLogStatusCategory& Category = Snapshot->Status[CatIndex];
	Category.Category = FString::Printf(TEXT("Tank (%s)"), *GetName());

	Category.Add(TEXT("Fire Cooldown Remaining"), FString::Printf(TEXT("%.1f"), FMath::Max(0, LastFireTimeSeconds + FireCooldownTimeSeconds - CurrentTimeSeconds)));

	// TODO: Move to health component visual logger
	Category.Add(TEXT("Health"), FString::Printf(TEXT("%.1f"), HealthComponent->GetHealth()));

	// TODO: Change color based on speed and stopping
	// 
	FColor BoxColor = FColor::Blue;

	// Add oriented bounded box for tank
	const auto OBB = GetBounds(*TankBody) + GetBounds(*TankTurret, &TankSockets::TurretAttach) + GetBounds(*TankTreadLeft, &TankSockets::TreadLeftAttach) + GetBounds(*TankTreadRight, &TankSockets::TreadRightAttach);
	const FVector& BoundsExtent = OBB.GetExtent();
	const FVector ZOffset = OBB.GetCenter();

	const auto& ActorTransform = GetActorTransform();
	const auto TransformMatrix = ActorTransform.ToMatrixNoScale();

	Snapshot->AddElement(OBB, TransformMatrix, LogTRTank.GetCategoryName(), ELogVerbosity::Log, BoxColor);

	// Draw barrel
	const auto& BarrelOBB = GetBounds(*TankBarrel);
	const auto& BarrelExtent = BarrelOBB.GetExtent();
	const auto& GunBarrelLocation = TankTurret->GetSocketLocation(TankSockets::GunAttach) + TankBarrel->GetForwardVector() * BarrelExtent.X - TankBarrel->GetUpVector() * BarrelExtent.Z;
	auto BarrelTransform = TankBarrel->GetComponentTransform();
	BarrelTransform.SetTranslation(GunBarrelLocation);

	FColor BarrelColor;
	switch (TankAimingComponent->GetTankFiringStatus())
	{
		case ETankFiringStatus::Locked: BarrelColor = FColor::Red; break;
		case ETankFiringStatus::Aiming: BarrelColor = FColor::Orange; break;
		case ETankFiringStatus::Reloading: BarrelColor = FColor::Yellow; break;
		default: BarrelColor = FColor::Blue; break;
	}

	Snapshot->AddElement(BarrelOBB, BarrelTransform.ToMatrixNoScale(), LogTRTank.GetCategoryName(), ELogVerbosity::Log, BarrelColor);


	const auto MyController = GetController();
	Snapshot->AddElement(GetActorLocation() + ZOffset, LogTRTank.GetCategoryName(), ELogVerbosity::Log, BoxColor,
		FString::Printf(TEXT("%s\n%s"), *GetName(), *LoggingUtils::GetName(MyController)));

	// Forward vector
	const auto& ForwardVector = GetActorForwardVector();
	const auto FrontWorldLocation = GetActorLocation() + ForwardVector * FMath::Max(BoundsExtent.X, BoundsExtent.Y);

	Snapshot->AddArrow(FrontWorldLocation, FrontWorldLocation + ForwardVector * 100.0f, LogTRTank.GetCategoryName(), ELogVerbosity::Log, FColor::Red, TEXT("F"));

	TankAimingComponent->DescribeSelfToVisLog(Snapshot);
}

#endif

#pragma endregion Visual Logger

namespace
{
	FBox GetBounds(const UStaticMeshComponent& Comp, const FName* SocketName)
	{
		auto Mesh = Comp.GetStaticMesh();
		if (!Mesh)
		{
			return FBox{ EForceInit::ForceInitToZero };
		}

		auto Bounds = Mesh->GetBounds();

		const FVector& BoundsExtent = Bounds.BoxExtent;
		FVector RelativeCenter(0, 0, BoundsExtent.Z);

		if (SocketName)
		{
			RelativeCenter += GetSocketRelativeLocation(Comp, *SocketName);
		}

		return FBox::BuildAABB(RelativeCenter, BoundsExtent);
	}

	FVector GetSocketRelativeLocation(const UStaticMeshComponent& Comp, const FName & SocketName)
	{
		const USceneComponent* CurrentComp = &Comp;
		while (auto ParentComp = CurrentComp->GetAttachParent())
		{
			if (!ParentComp->DoesSocketExist(SocketName))
			{
				CurrentComp = ParentComp;
				continue;
			}

			FTransform SocketTransform = ParentComp->GetSocketTransform(SocketName);
			FTransform RelativeTransform = ParentComp->GetRelativeTransform();
			return RelativeTransform.InverseTransformPosition(SocketTransform.GetLocation());
		}

		return FVector::ZeroVector;
	}
}


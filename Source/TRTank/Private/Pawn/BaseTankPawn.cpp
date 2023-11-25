// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/BaseTankPawn.h"

#include "Components/TankAimingComponent.h"
#include "Components/TankTurretComponent.h"
#include "Components/TankBarrelComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

#include "TankSockets.h"
#include "Projectile.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	FBox GetBounds(const UStaticMeshComponent& Comp, const FName* SocketName = nullptr);
}

ABaseTankPawn::ABaseTankPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = TankBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBody"));
	TankBody->SetSimulatePhysics(true);
	TankBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TankBody->SetMassOverrideInKg(NAME_None, 40000);

	TankTurret = CreateDefaultSubobject<UTankTurretComponent>(TEXT("TankTurret"));
	TankTurret->SetupAttachment(TankBody, TankSockets::TurretAttach);

	TankBarrel = CreateDefaultSubobject<UTankBarrelComponent>(TEXT("TankBarrel"));
	TankBarrel->SetupAttachment(TankTurret, TankSockets::GunAttach);

	TankTreadRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadRight"));
	TankTreadRight->SetupAttachment(TankBody, TankSockets::TreadRightAttach);

	TankTreadLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadLeft"));
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
}

// Called when the game starts or when spawned
void ABaseTankPawn::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseTankPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	TankAimingComponent->SetTankComponents(
		{
				.Barrel = TankBarrel,
				.Turret = TankTurret
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

void ABaseTankPawn::AimAt(const FVector& Location)
{
	TankAimingComponent->AimAt(Location, TankShellSpeed);
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

	const FVector SpawnLocation = TankBarrel->GetSocketLocation(TankSockets::GunFire);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = this;
	SpawnParameters.Owner = this;

	auto SpawnedProjectile = World->SpawnActor<AProjectile>(MainGunProjectile, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);

	if (!SpawnedProjectile)
	{
		UE_VLOG_UELOG(this, LogTRTank, Warning, TEXT("%s: Fire: Unable to spawn projectile %s at %s"), *GetName(), *LoggingUtils::GetName(MainGunProjectile), *SpawnLocation.ToCompactString());
		return;
	}

	UE_VLOG_UELOG(this, LogTRTank, Log, TEXT("%s: Fire: %s at %s"), *GetName(), *LoggingUtils::GetName(MainGunProjectile), *SpawnLocation.ToCompactString());
}

#if ENABLE_VISUAL_LOG

void ABaseTankPawn::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	// helpful article about the Visual Logger
	// https://benui.ca/unreal/visual-logger/

	if (!TankBody || !TankTurret || !TankTreadLeft || !TankTreadRight)
	{
		return;
	}

	// Get reference to the current category
	const int32 CatIndex = Snapshot->Status.AddZeroed();
	FVisualLogStatusCategory& Category = Snapshot->Status[CatIndex];
	Category.Category = FString::Printf(TEXT("Tank (%s)"), *GetName());

	// TODO: Change color based on speed and stopping
	// 
	FColor BoxColor = FColor::Blue;

	// Add oriented bounded box for tank
	const auto OBB = GetBounds(*TankBody) + GetBounds(*TankTurret, &TankSockets::TurretAttach) + GetBounds(*TankTreadLeft, &TankSockets::TreadLeftAttach) + GetBounds(*TankTreadRight, &TankSockets::TreadRightAttach);
	const FVector& BoundsExtent = OBB.GetExtent();
	const FVector ZOffset = OBB.GetCenter();

	const auto TransformMatrix = GetActorTransform().ToMatrixNoScale();

	Snapshot->AddElement(OBB, TransformMatrix, LogTRTank.GetCategoryName(), ELogVerbosity::Log, BoxColor);

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
			if (auto ParentComp = Comp.GetAttachParent(); ParentComp)
			{
				FTransform SocketTransform = ParentComp->GetSocketTransform(*SocketName);
				FTransform RelativeTransform = ParentComp->GetRelativeTransform();
				FVector RelativeOffset = RelativeTransform.InverseTransformPosition(SocketTransform.GetLocation());

				RelativeCenter += RelativeOffset;
			}
		}

		return FBox::BuildAABB(RelativeCenter, BoundsExtent);
	}
}


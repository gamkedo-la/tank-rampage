// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/BaseTankPawn.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	namespace Sockets
	{
		const FName TankTurret = "Turret";
		const FName TankGun = "Barrel";
		const FName TankTreadRight = "Tread_RT";
		const FName TankSideRight = "Side_RT";
		const FName TankTreadLeft = "Tread_LT";
		const FName TankSideLeft = "Side_LT";
	}
}

ABaseTankPawn::ABaseTankPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = TankBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBody"));
	TankBody->SetSimulatePhysics(true);
	TankBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TankBody->SetMassOverrideInKg(NAME_None, 40000);

	TankTurret = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTurret"));
	TankTurret->SetupAttachment(TankBody, Sockets::TankTurret);

	TankBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBarrel"));
	TankBarrel->SetupAttachment(TankTurret, Sockets::TankGun);

	TankTreadRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadRight"));
	TankTreadRight->SetupAttachment(TankBody, Sockets::TankTreadRight);

	TankTreadLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadLeft"));
	TankTreadLeft->SetupAttachment(TankBody, Sockets::TankTreadLeft);

	TankTreadSideRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideRight"));
	TankTreadSideRight->SetupAttachment(TankBody, Sockets::TankSideRight);

	TankTreadSideLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideLeft"));
	TankTreadSideLeft->SetupAttachment(TankBody, Sockets::TankSideLeft);

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(TankBody);

	// Only enable for player controller
	CameraSpringArm->PrimaryComponentTick.bStartWithTickEnabled = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);
}

// Called when the game starts or when spawned
void ABaseTankPawn::BeginPlay()
{
	Super::BeginPlay();
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
	UE_VLOG_UELOG(this, LogTRTank, VeryVerbose, TEXT("%s: AimAt - Location=%s"), *GetName(), *Location.ToCompactString());


}


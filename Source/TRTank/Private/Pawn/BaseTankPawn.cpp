// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/BaseTankPawn.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

ABaseTankPawn::ABaseTankPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = TankBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBody"));

	TankTurret = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTurret"));
	TankTurret->SetupAttachment(TankBody);

	TankBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBarrel"));
	TankBarrel->SetupAttachment(TankTurret);

	TankTreadRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadRight"));
	TankTreadRight->SetupAttachment(TankBody);

	TankTreadLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadLeft"));
	TankTreadLeft->SetupAttachment(TankBody);

	TankTreadSideRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideRight"));
	TankTreadSideRight->SetupAttachment(TankBody);

	TankTreadSideLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTreadSideLeft"));
	TankTreadSideLeft->SetupAttachment(TankBody);

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


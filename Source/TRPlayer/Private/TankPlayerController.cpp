// Fill out your copyright notice in the Description page of Project Settings.


#include "TankPlayerController.h"

#include "Logging/LoggingUtils.h"
#include "TRPlayerLogging.h"
#include "Pawn/BaseTankPawn.h"

#include "InputActionValue.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/TankAimingComponent.h"

ATankPlayerController::ATankPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATankPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeCamera();
	InitializeInputMappingContext();
}

void ATankPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// CastChecked will terminate the game if it is null
	auto EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::OnLook);
	}
	else
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: SetupPlayerInputComponent - LookAction not bound"), *GetName());
	}

	if (FireAction)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ThisClass::OnFire);
	}
	else
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: SetupPlayerInputComponent - FireAction not bound"), *GetName());
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::OnMove);
	}
	else
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: SetupPlayerInputComponent - MoveAction not bound"), *GetName());
	}
}

void ATankPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimTowardCrosshair();
}

ABaseTankPawn* ATankPlayerController::GetControlledTank() const
{
	return Cast<ABaseTankPawn>(GetPawn());
}

void ATankPlayerController::InitializeInputMappingContext()
{
	if (!InputMappingContext)
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: BeginPlay - InputMappingContext not defined - no input mappings can be bound!"), *GetName());
		return;
	}

	auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (!Subsystem)
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: BeginPlay - Cannot get UEnhancedInputLocalPlayerSubsystem from PlayerController=%s"), *GetName(), *GetName());
		return;
	}

	Subsystem->AddMappingContext(InputMappingContext, 0);
}

void ATankPlayerController::InitializeCamera()
{
	check(PlayerCameraManager);

	// Invert the values
	PlayerCameraManager->ViewPitchMin = -MaxPitch;
	PlayerCameraManager->ViewPitchMax = -MinPitch;
}

void ATankPlayerController::AimTowardCrosshair()
{
	auto ControlledTank = GetControlledTank();
	if (!ControlledTank)
	{
		return;
	}
	
	FAimingData AimingData;
	GetAimingData(AimingData);
	
	ControlledTank->AimAt(AimingData);
}

void ATankPlayerController::OnFire()
{
	auto ControlledTank = GetControlledTank();
	if (!ControlledTank)
	{
		return;
	}

	ControlledTank->Fire();
}

void ATankPlayerController::OnMove(const FInputActionValue& Value)
{
	auto ControlledTank = GetControlledTank();
	if (!ControlledTank)
	{
		return;
	}

	const auto MoveAxisValue = Value.Get<FVector2D>();

	ControlledTank->MoveForward(MoveAxisValue.Y);
	ControlledTank->TurnRight(MoveAxisValue.X);
}

void ATankPlayerController::OnLook(const FInputActionValue& Value)
{
	const auto LookAxisValue = Value.Get<FVector2D>();

	// AddYawInput and AddPitchInput requires Camera Control Rotation to be checked on the camera spring arm on the pawn
	if (!FMath::IsNearlyZero(LookAxisValue.X))
	{
		AddYawInput(LookAxisValue.X);
	}

	if (!FMath::IsNearlyZero(LookAxisValue.Y))
	{
		AddPitchInput(LookAxisValue.Y);
	}
}

void ATankPlayerController::GetAimingData(FAimingData& AimingData) const
{
	const auto CrosshairScreenLocation = GetCrosshairScreenspaceLocation();
	
	DeprojectScreenPositionToWorld(
		CrosshairScreenLocation.X,
		CrosshairScreenLocation.Y,
		AimingData.AimingOriginWorldLocation,
		AimingData.AimingWorldDirection
	);

	//Setup Ray Trace
	UWorld* World = GetWorld();
	check(World);
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetPawn());
	const auto& TraceStartLocation = AimingData.AimingOriginWorldLocation;
	const auto TraceEndLocation = TraceStartLocation
												+ MaxAimDistanceMeters * 100 //conversion to meters
												* AimingData.AimingWorldDirection;
	
	AimingData.bHitResult = World->LineTraceSingleByChannel(
		HitResult,
		TraceStartLocation,
		TraceEndLocation,
		ECollisionChannel::ECC_Visibility,
		Params);
	
	AimingData.HitLocation = HitResult.Location;
}

FVector2D ATankPlayerController::GetCrosshairScreenspaceLocation() const
{
	int32 ViewportSizeX, ViewportSizeY;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	// Origin is at top left of screen
	return FVector2D(ViewportSizeX, ViewportSizeY) * CrosshairPositionFraction;
}

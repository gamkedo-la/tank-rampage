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

	// TODO: Start the tank moving the barrel so that a shot would hit where the crosshair intersects the world
	// Get world location with linetrace through crosshair
	// If hit the landscape then tell the controlled tank to aim at the crosshair
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

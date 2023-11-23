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

	const auto MaybeHitLocation = GetRaySightHitLocation();

	if (!MaybeHitLocation)
	{
		return;
	}

	const auto& HitLocation = *MaybeHitLocation;

	ControlledTank->AimAt(HitLocation);
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

std::optional<FVector> ATankPlayerController::GetRaySightHitLocation() const
{
	// Find the crosshair position
	const auto ScreenspaceLocation = GetCrosshairScreenspaceLocation();

	// De-project the screen position of the crosshair to a world direction
	const auto LookDirection = GetCrosshairWorldDirection(ScreenspaceLocation);

	// Line-trace along that direction (look direction) and see what we hit (up to max range)
	return GetLookVectorHitLocation(LookDirection);
}

FVector2D ATankPlayerController::GetCrosshairScreenspaceLocation() const
{
	int32 ViewportSizeX, ViewportSizeY;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	// Origin is at top left of screen
	return FVector2D(ViewportSizeX, ViewportSizeY) * CrosshairPositionFraction;
}

FVector ATankPlayerController::GetCrosshairWorldDirection(const FVector2D& ScreenLocation) const
{
	FVector CameraWorldLocation, WorldDirection;

	// CameraWorldLocation here is just the camera world location and isn't useful in this situation
	DeprojectScreenPositionToWorld(
		ScreenLocation.X,
		ScreenLocation.Y,
		CameraWorldLocation,
		WorldDirection
	);

	return WorldDirection;
}

std::optional<FVector> ATankPlayerController::GetLookVectorHitLocation(const FVector& LookDirection) const
{
	auto World = GetWorld();

	if (!World)
	{
		return std::nullopt;
	}

	check(PlayerCameraManager);

	const auto& TraceStartLocation = PlayerCameraManager->GetCameraLocation();
	const auto TraceEndLocation = TraceStartLocation + MaxAimDistanceMeters * 100 * LookDirection;

	FHitResult HitResult;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetPawn());

	if (!World->LineTraceSingleByChannel(
		HitResult,
		TraceStartLocation,
		TraceEndLocation,
		ECollisionChannel::ECC_Visibility,
		Params))
	{
		return std::nullopt;
	}

	return HitResult.Location;
}

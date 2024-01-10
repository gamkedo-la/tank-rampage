// Fill out your copyright notice in the Description page of Project Settings.


#include "TankPlayerController.h"

#include "Logging/LoggingUtils.h"
#include "TRPlayerLogging.h"
#include "Pawn/BaseTankPawn.h"
#include "Components/HealthComponent.h"
#include "Item/ItemInventory.h"
#include "Subsystems/TankEventsSubsystem.h"

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

void ATankPlayerController::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	UE_VLOG_UELOG(this, LogTRPlayer, Display,
		TEXT("%s: GameHasEnded: %s"), *GetName(), bIsWinner ? TEXT("Won") : TEXT("Lost"));

	Super::GameHasEnded(EndGameFocus, bIsWinner);

	// TODO: This is placeholder logic - for now just pause for three seconds and then restart the level
	// We will want a message to show on the HUD and transition to end game stats

	FTimerHandle _;
	GetWorldTimerManager().SetTimer(_, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		this->RestartLevel();
	}), 5.0f, false);
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

	BindWeaponSelectActions(*EnhancedInputComponent);
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

void ATankPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto Tank = GetControlledTank();
	if (!ensure(Tank))
	{
		return;
	}

	Tank->GetHealthComponent()->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
}

bool ATankPlayerController::IsControlledTankAlive() const
{
	auto Tank = GetControlledTank();
	if (!Tank)
	{
		return false;
	}

	return Tank && Tank->GetHealthComponent()->IsAlive();
}

void ATankPlayerController::InitializeInputMappingContext()
{
	if (!InputMappingContext)
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: BeginPlay - InputMappingContext not defined - no input mappings can be bound!"), *GetName());
		return;
	}

	auto LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		// on dedicated server
		UE_LOG(LogTRPlayer, Display, TEXT("%s: BeginPlay - LocalPlayer is NULL - on dedicated server"), *GetName(), *GetName());
		return;

	}
	auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

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

void ATankPlayerController::BindWeaponSelectActions(UEnhancedInputComponent& EnhancedInputComponent)
{
	for (int32 i = 0; i < WeaponSelectActions.Num(); ++i)
	{
		auto WeaponSelectAction = WeaponSelectActions[i];
		if (!ensureAlwaysMsgf(WeaponSelectAction, TEXT("WeaponSelectAction[%d] is NULL"), i))
		{
			continue;
		}

		// Cannot bind a lambda so no way to pass in the index
		EnhancedInputComponent.BindAction(WeaponSelectAction, ETriggerEvent::Triggered, this, &ThisClass::OnSelectWeapon);
	}
}

#pragma region Controls

void ATankPlayerController::AimTowardCrosshair()
{
	auto ControlledTank = GetControlledTank();
	if (!ControlledTank || !IsControlledTankAlive())
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
	if (!ControlledTank || !IsControlledTankAlive())
	{
		return;
	}

	ControlledTank->Fire();
}

void ATankPlayerController::OnMove(const FInputActionValue& Value)
{
	auto ControlledTank = GetControlledTank();
	if (!ControlledTank || !IsControlledTankAlive())
	{
		return;
	}

	const auto MoveAxisValue = Value.Get<FVector2D>();

	ControlledTank->MoveForward(MoveAxisValue.Y);
	ControlledTank->TurnRight(MoveAxisValue.X);
}

void ATankPlayerController::OnSelectWeapon(const FInputActionInstance& InputActionInstance)
{
	const int32 FoundIndex = WeaponSelectActions.Find(InputActionInstance.GetSourceAction());
	if (FoundIndex != INDEX_NONE)
	{
		SelectWeapon(FoundIndex);
	}
	else
	{
		UE_LOG(LogTRPlayer, Error, TEXT("%s: OnSelectWeapon - Cannot find bound weapon index for source input action=%s"),
			*GetName(), *LoggingUtils::GetName(InputActionInstance.GetSourceAction()));
	}
}

void ATankPlayerController::SelectWeapon(int32 WeaponIndex) const
{
	auto ControlledTank = GetControlledTank();
	if (!ControlledTank || !IsControlledTankAlive())
	{
		return;
	}

	auto ItemInventory = ControlledTank->GetItemInventory();
	check(ItemInventory);

	// TODO: Switch to CanWeaponBeActivatedByIndex if we don't want player to switch to a weapon still in cooldown
	if (ItemInventory->IsWeaponAvailableByIndex(WeaponIndex))
	{
		UE_VLOG_UELOG(this, LogTRPlayer, Log,
			TEXT("%s: SelectWeapon: %d"), *GetName(), WeaponIndex);

		ItemInventory->SetActiveWeaponByIndex(WeaponIndex);
	}
	else
	{
		UE_VLOG_UELOG(this, LogTRPlayer, Log,
			TEXT("%s: SelectWeapon: %d - Cannot be selected as that weapon slot hasn't been filled yet"), *GetName(), WeaponIndex);
	}
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

#pragma endregion Controls

void ATankPlayerController::OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, AController* EventInstigator, AActor* ChangeCauser)
{
	check(HealthComponent);

	if (auto Tank = GetControlledTank(); Tank && HealthComponent->IsDead())
	{
		auto World = GetWorld();
		check(World);

		auto TankEventsSubsystem = World->GetSubsystem<UTankEventsSubsystem>();
		if (ensure(TankEventsSubsystem))
		{
			TankEventsSubsystem->OnTankDestroyed.Broadcast(Tank, EventInstigator, ChangeCauser);
		}
	}
}

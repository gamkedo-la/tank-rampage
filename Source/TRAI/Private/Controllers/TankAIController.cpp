// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/TankAIController.h"

#include "Pawn/BaseTankPawn.h"
#include "Components/TankAimingComponent.h"
#include "Components/HealthComponent.h"

#include "Kismet/GameplayStatics.h" 
#include "TRAILogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Subsystems/TankEventsSubsystem.h"
#include "Item/ItemInventory.h"
#include "Item/ItemNames.h"

#include "Curves/CurveFloat.h"


ATankAIController::ATankAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

ABaseTankPawn* ATankAIController::GetControlledTank() const
{
	return Cast<ABaseTankPawn>(GetPawn());
}

void ATankAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto World = GetWorld();
	const auto NowSeconds = World->GetTimeSeconds();

	if (!World || NowSeconds < StartDelayTime)
	{
		return;
	}

	auto PlayerTank = GetPlayerTank();
	auto ControlledTank = GetControlledTank();

	if (!PlayerTank || !ControlledTank)
	{
		return;
	}

	FTankAIContext AIContext
	{
		.MyTank = *ControlledTank,
		.PlayerTank = *PlayerTank,
		.bAllowFiringWhileAiming = false
	};

	if (!IsPlayerInRange(AIContext))
	{
		FirstInRangeTime = TargetingErrorLastTime = -1;
		return;
	}
	else if (FirstInRangeTime < 0)
	{
		FirstInRangeTime = NowSeconds;
	}

	if (NowSeconds - FirstInRangeTime < ReactionTime)
	{
		return;
	}

	if (NowSeconds - TargetingErrorLastTime >= TargetingErrorResetTime)
	{
		InitTargetingError(AIContext);
		TargetingErrorLastTime = NowSeconds;
	}

	// AI Tanks have trouble getting an aim lock while close, so allow firing while "aiming" when close up
	if (!MoveTowardPlayer(AIContext))
	{
		AIContext.bAllowFiringWhileAiming = true;
	}

	AimAtPlayerTank(AIContext);
	Fire(AIContext);
}

void ATankAIController::BeginPlay()
{
	Super::BeginPlay();

	// Do alternate checks for line of sight
	bLOSflag = true;
	bSkipExtraLOSChecks = false;
}

ABaseTankPawn* ATankAIController::GetPlayerTank() const
{
	return Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void ATankAIController::Fire(const FTankAIContext& AIContext)
{
	auto& MyTank = AIContext.MyTank;

	auto TankAimingComponent = MyTank.GetTankAimingComponent();

	const auto FiringStatus = TankAimingComponent->GetTankFiringStatus();
	if (FiringStatus == ETankFiringStatus::Locked || (AIContext.bAllowFiringWhileAiming && FiringStatus == ETankFiringStatus::Aiming))
	{
		MyTank.Fire();
	}
}

void ATankAIController::AimAtPlayerTank(const FTankAIContext& AIContext)
{
	const auto& PlayerTank = AIContext.PlayerTank;

	auto PlayerVelocity = PlayerTank.GetVelocity();

	FVector PredictiveOffset{ EForceInit::ForceInitToZero };
	if (PlayerVelocity.SizeSquared() > FMath::Square(PlayerVelocityPredictiveThreshold))
	{
		// Assuming only the "X" direction ignoring gravity
		const float PlayerDistanceMeters = AIContext.MyTank.GetDistanceTo(&PlayerTank);
		const float InterceptTime = PlayerDistanceMeters / AIContext.MyTank.GetCurrentWeaponExitSpeed();

		PredictiveOffset = PlayerVelocity * InterceptTime;
	}

	const auto PredictedPosition = AIContext.PlayerTank.GetActorLocation() + PredictiveOffset;
	const auto& AimTarget = PredictedPosition + TargetingError;

	FAimingData AimingData;
	AimingData.bHitResult = true;
	AimingData.HitLocation = AimTarget;
	
	AIContext.MyTank.AimAt(AimingData);
}

bool ATankAIController::MoveTowardPlayer(const FTankAIContext& AIContext)
{
	const auto MinMoveDistance = MinMoveDistanceMeters * 100;

	const auto& PlayerTank = AIContext.PlayerTank;
	const auto& AITank = AIContext.MyTank;

	const auto& TargetLocation = PlayerTank.GetActorLocation();

	const bool bShouldMove = FVector::DistSquared(AITank.GetActorLocation(), TargetLocation) > FMath::Square(MinMoveDistance);

	if (!bShouldMove)
	{
		return false;
	}

	MoveToLocation(TargetLocation, MinMoveDistanceMeters * 100);

	return true;
}

bool ATankAIController::IsPlayerInRange(const FTankAIContext& AIContext) const
{
	const bool bInRangeByDistance = AIContext.MyTank.GetSquaredDistanceTo(&AIContext.PlayerTank) <= FMath::Square(MaxAggroDistanceMeters * 100);
	if (!bInRangeByDistance)
	{
		return false;
	}

	// Do alternate checks so multiple line trace points are used for reference
	return LineOfSightTo(&AIContext.PlayerTank, FVector::ZeroVector, true);
}

void ATankAIController::InitTargetingError(const FTankAIContext& AIContext)
{
	if (!TargetingErrorByDistanceMeters)
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s-%s: InitTargetingError: No TargetingErrorByDistanceMeters curve set!"),
			*GetName(), *AIContext.MyTank.GetName());
		return;
	}

	const float PlayerDistanceMeters = AIContext.MyTank.GetDistanceTo(&AIContext.PlayerTank) / 100;
	const float TargetingErrorMagnitude = TargetingErrorByDistanceMeters->FloatCurve.Eval(PlayerDistanceMeters);

	TargetingError = FMath::RandRange(-TargetingErrorMagnitude, TargetingErrorMagnitude) * FMath::VRand();
}

void ATankAIController::OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, AController* EventInstigator, AActor* ChangeCauser)
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

		Tank->DetachFromControllerPendingDestroy();
		Tank->Destroy();
	}
}

void ATankAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto Tank = GetControlledTank();
	if (!ensure(Tank))
	{
		return;
	}

	// Give AI a basic item
	Tank->GetItemInventory()->AddItemByName(ItemNames::MainGunName);

	Tank->GetHealthComponent()->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
}

void ATankAIController::OnUnPossess()
{
	auto Tank = GetControlledTank();

	Super::OnUnPossess();

	if (Tank)
	{
		// Clear inventory when possessed by player as player start pawn is first possessed by AI and then possessed by player
		Tank->GetItemInventory()->Clear();
		Tank->GetHealthComponent()->OnHealthChanged.RemoveDynamic(this, &ThisClass::OnHealthChanged);
	}
}

#if ENABLE_VISUAL_LOG
void ATankAIController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	auto& Category = Snapshot->Status[0];
}

#endif

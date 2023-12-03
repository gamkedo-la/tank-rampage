// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/TankAIController.h"

#include "Pawn/BaseTankPawn.h"
#include "Components/TankAimingComponent.h"

#include "Kismet/GameplayStatics.h" 
#include "TRAILogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"


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

	auto PlayerTank = GetPlayerTank();
	auto ControlledTank = GetControlledTank();

	if (!PlayerTank || !ControlledTank)
	{
		return;
	}

	FTankAIContext AIContext
	{
		.MyTank = *ControlledTank,
		.PlayerTank = *PlayerTank
	};

	if (!IsPlayerInRange(AIContext))
	{
		return;
	}

	AimAtPlayerTank(AIContext);
	Fire(AIContext);

	MoveTowardPlayer(AIContext);
}

ABaseTankPawn* ATankAIController::GetPlayerTank() const
{
	return Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void ATankAIController::Fire(const FTankAIContext& AIContext)
{
	auto& MyTank = AIContext.MyTank;

	auto TankAimingComponent = MyTank.GetTankAimingComponent();

	if (TankAimingComponent->GetTankFiringStatus() == ETankFiringStatus::Locked)
	{
		AIContext.MyTank.Fire();
	}
}

void ATankAIController::AimAtPlayerTank(const FTankAIContext& AIContext)
{
	AIContext.MyTank.AimAt(AIContext.PlayerTank.GetActorLocation());
}

void ATankAIController::MoveTowardPlayer(const FTankAIContext& AIContext)
{
	const auto MinMoveDistance = MinMoveDistanceMeters * 100;

	const auto& PlayerTank = AIContext.PlayerTank;
	const auto& AITank = AIContext.MyTank;

	const auto& TargetLocation = PlayerTank.GetActorLocation();

	const bool bShouldMove = FVector::DistSquared(AITank.GetActorLocation(), TargetLocation) > FMath::Square(MinMoveDistance);

	if (!bShouldMove)
	{
		return;
	}

	MoveToLocation(TargetLocation, MinMoveDistance);
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

#if ENABLE_VISUAL_LOG
void ATankAIController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	auto& Category = Snapshot->Status[0];
}

#endif

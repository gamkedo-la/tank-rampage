// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/TankAIController.h"

#include "Pawn/BaseTankPawn.h"

#include "Kismet/GameplayStatics.h" 

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
	AIContext.MyTank.Fire();
}

void ATankAIController::AimAtPlayerTank(const FTankAIContext& AIContext)
{
	AIContext.MyTank.AimAt(AIContext.PlayerTank.GetActorLocation());
}

void ATankAIController::MoveTowardPlayer(const FTankAIContext& AIContext)
{
	MoveToActor(&AIContext.PlayerTank, MovementAcceptanceRadius);
}

bool ATankAIController::IsPlayerInRange(const FTankAIContext& AIContext) const
{
	return AIContext.MyTank.GetSquaredDistanceTo(&AIContext.PlayerTank) <= FMath::Square(MaxAggroDistanceMeters * 100);
}

#if ENABLE_VISUAL_LOG
void ATankAIController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	auto& Category = Snapshot->Status[0];
}

#endif

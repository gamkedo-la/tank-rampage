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

	if (!IsPlayerInRange(*ControlledTank, *PlayerTank))
	{
		return;
	}

	AimAtPlayerTank(*ControlledTank, *PlayerTank);
	MoveTowardPlayer(*ControlledTank, *PlayerTank);

}

ABaseTankPawn* ATankAIController::GetPlayerTank() const
{
	return Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void ATankAIController::AimAtPlayerTank(ABaseTankPawn& MyTank, const ABaseTankPawn& PlayerTank)
{
	MyTank.AimAt(PlayerTank.GetActorLocation());
}

void ATankAIController::MoveTowardPlayer(ABaseTankPawn& MyTank, const ABaseTankPawn& PlayerTank)
{
	// TODO:
}

bool ATankAIController::IsPlayerInRange(const ABaseTankPawn& MyTank, const ABaseTankPawn& PlayerTank) const
{
	return MyTank.GetDistanceTo(&PlayerTank) <= MaxAggroDistanceMeters;
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/BaseAIController.h"
#include "Interfaces/TankOwner.h"

#include "TankAIController.generated.h"

/**
 * 
 */
UCLASS()
class TRAI_API ATankAIController : public ABaseAIController, public ITankOwner
{
	GENERATED_BODY()
	
public:

	ATankAIController();

	// Inherited via ITankOwner
	ABaseTankPawn* GetControlledTank() const override;

	virtual void Tick(float DeltaTime) override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

private:

	struct FTankAIContext
	{
		ABaseTankPawn& MyTank;
		ABaseTankPawn& PlayerTank;
	};

	ABaseTankPawn* GetPlayerTank() const;

	void AimAtPlayerTank(const FTankAIContext& AIContext);
	void Fire(const FTankAIContext& AIContext);

	void MoveTowardPlayer(const FTankAIContext& AIContext);
	bool IsPlayerInRange(const FTankAIContext& AIContext) const;

private:
	UPROPERTY(EditDefaultsOnly)
	float MaxAggroDistanceMeters{ 100.0f };

	UPROPERTY(EditDefaultsOnly)
	float MinMoveDistanceMeters{ 10.0f };
};

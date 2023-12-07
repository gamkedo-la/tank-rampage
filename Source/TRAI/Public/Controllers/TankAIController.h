// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/BaseAIController.h"
#include "Interfaces/TankOwner.h"

#include "TankAIController.generated.h"

class UCurveFloat;

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
	virtual void BeginPlay() override;

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

	void InitTargetingError(const FTankAIContext& AIContext);

private:
	UPROPERTY(EditDefaultsOnly)
	float MaxAggroDistanceMeters{ 100.0f };

	UPROPERTY(EditDefaultsOnly)
	float MinMoveDistanceMeters{ 10.0f };

	UPROPERTY(EditAnywhere)
	float PlayerVelocityPredictiveThreshold{ 300.0f };

	UPROPERTY(EditDefaultsOnly)
	float StartDelayTime{ 3.0f };

	UPROPERTY(EditAnywhere)
	float ReactionTime{ 0.5f };

	UPROPERTY(EditAnywhere)
	float TargetingErrorResetTime{ 2.0f };

	UPROPERTY(EditAnywhere)
	UCurveFloat* TargetingErrorByDistanceMeters{};

	float FirstInRangeTime{ -1.0f };
	float TargetingErrorLastTime{ -1.0f };
	FVector TargetingError{ EForceInit::ForceInitToZero };
};

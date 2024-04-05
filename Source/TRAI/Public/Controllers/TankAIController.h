// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/BaseAIController.h"
#include "Interfaces/TankOwner.h"

#include "TankAIController.generated.h"

class UCurveFloat;
class UHealthComponent;

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

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:

	struct FTankAIContext
	{
		ABaseTankPawn& MyTank;
		ABaseTankPawn& PlayerTank;
	};

	ABaseTankPawn* GetPlayerTank() const;

	void AimAtPlayerTank(const FTankAIContext& AIContext);
	void Fire(const FTankAIContext& AIContext);

	bool MoveTowardPlayer(const FTankAIContext& AIContext);
	bool IsPlayerInRange(const FTankAIContext& AIContext) const;

	void InitTargetingError(const FTankAIContext& AIContext);

	UFUNCTION()
	void OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, float PreviousMaxHealthValue, AController* EventInstigator, AActor* ChangeCauser);

private:
	UPROPERTY(EditDefaultsOnly)
	float MaxAggroDistanceMeters{ 100.0f };

	UPROPERTY(EditDefaultsOnly)
	float MinMoveDistanceMeters{ 10.0f };

	UPROPERTY(EditDefaultsOnly)
	float AcceptanceRadiusMeters{ 5.0f };

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

	UPROPERTY(EditAnywhere)
	UCurveFloat* TargetingErrorMultiplierByShotsFired{};

	float FirstInRangeTime{ -1.0f };
	float TargetingErrorLastTime{ -1.0f };
	int32 ShotsFired{};

	FVector TargetingError{ EForceInit::ForceInitToZero };
};

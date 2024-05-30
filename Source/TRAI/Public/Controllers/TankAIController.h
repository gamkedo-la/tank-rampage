// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Controllers/BaseAIController.h"
#include "Interfaces/TankOwner.h"

#include "TankAIController.generated.h"

class UCurveFloat;
class UHealthComponent;
class UTankAISharedStateSubsystem;

/**
 * 
 */
UCLASS()
class TRAI_API ATankAIController : public ABaseAIController, public ITankOwner
{
	GENERATED_BODY()

public:

	ATankAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
		UTankAISharedStateSubsystem& AISubsystem;
		double NowSeconds;
		float DistSqToPlayer;
	};

	ABaseTankPawn* GetPlayerTank() const;

	TOptional<FTankAIContext> GetAIContext() const;
	bool ShouldMoveTowardReportedPosition(const FTankAIContext& AIContext) const;

	void AimAtPlayerTank(const FTankAIContext& AIContext);
	void Fire(const FTankAIContext& AIContext);

	bool MoveTowardPlayer(const FTankAIContext& AIContext);
	bool IsPlayerInRange(const FTankAIContext& AIContext) const;

	void InitTargetErrorIfApplicable(const FTankAIContext& AIContext);

	void InitTargetingError(const FTankAIContext& AIContext);

	void InitReportedPositionReactTimeIfApplicable(const FTankAIContext& AIContext);
	void ExecuteAI();

	UFUNCTION()
	void OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, float PreviousMaxHealthValue, AController* EventInstigator, AActor* ChangeCauser);

	bool HasLineOfSight(const FTankAIContext& AIContext) const;
	bool IsInInfaredRange(const FTankAIContext& AIContext) const;

	void ResetState();
	void UpdateSharedPerceptionState(const FTankAIContext& AIContext) const;

	bool PassesDirectPerceptionReactionTimeDelay();

	void Wander(const FTankAIContext& AIContext);

	void SeekTowardLocation(const FVector& Location);

	bool ShouldWander() const;

	bool DeltaTimeExceedsThreshold(float Interval, float LastTime) const;

private:
	UPROPERTY(EditDefaultsOnly)
	float MaxAggroDistanceMeters{ 100.0f };

	/* AI can see through obstacles to perceive the player */
	UPROPERTY(EditDefaultsOnly)
	float MaxInfaredDistanceMeters{ 100.0f };

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
	float ReportedPositionForgetTime{ 30.0f };

	UPROPERTY(EditAnywhere)
	float ReportedPositionMinDelayTime{ 2.0f };

	UPROPERTY(EditAnywhere)
	float ReportedPositionMaxDelayTime{ 6.0f };

	UPROPERTY(EditDefaultsOnly)
	float WanderRadius{ 10000.0f };

	UPROPERTY(EditDefaultsOnly)
	float WanderCooldownSeconds{ 3.0f };

	UPROPERTY(EditAnywhere)
	float MoveRequestCooldownTimeSeconds{ 2.0f };

	UPROPERTY(EditAnywhere)
	UCurveFloat* TargetingErrorByDistanceMeters{};

	UPROPERTY(EditAnywhere)
	UCurveFloat* TargetingErrorMultiplierByShotsFired{};

	float FirstInRangeTime{ -1.0f };
	float TargetingErrorLastTime{ -1.0f };
	float ReportedPositionReactTime{ -1.0f };
	float LastWanderTime{ -1.0f };
	float LastMoveTime{ -1.0f };


	int32 ShotsFired{};
	bool bHasLOS{};
	bool bInInfaredRange{};

	FVector TargetingError{ EForceInit::ForceInitToZero };
};

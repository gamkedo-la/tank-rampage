// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TankAimingComponent.generated.h"

UENUM(BlueprintType)
enum class ETankFiringStatus : uint8
{
	Locked UMETA(DisplayName = "Locked"),
	Aiming UMETA(DisplayName = "Aiming"),
	Reloading UMETA(DisplayName = "Reloading"),
	NoTarget UMETA(DisplayName = "No Target"),
	Max UMETA(DisplayName = "MAX"),
};

class UTankBarrelComponent;
class UTankTurretComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTankAimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	struct FTankComponents
	{
		UTankBarrelComponent* Barrel{};
		UTankTurretComponent* Turret{};
	};

	UTankAimingComponent();

	void SetTankComponents(const FTankComponents& TankComponents);

	void AimAt(const FVector& Location, float LaunchSpeed);

	UFUNCTION(BlueprintPure)
	ETankFiringStatus GetTankFiringStatus() const { return FiringStatus; }


#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent();

private:
	void MoveBarrelTowards(const FVector& AimDirection);

	bool IsBarrelAlreadyAtTarget(const FVector& AimDirection) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTankBarrelComponent> Barrel{};

	UPROPERTY(Transient)
	TObjectPtr<UTankTurretComponent> Turret{};

	UPROPERTY(Category = Setup, EditDefaultsOnly)
	float AimTolerance{ 0.01f };

	ETankFiringStatus FiringStatus{ ETankFiringStatus::NoTarget };
	
};

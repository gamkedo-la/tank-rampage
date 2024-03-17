// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include <optional>

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

UENUM(BlueprintType)
enum class EAimingMode : uint8
{
	AssistedAim UMETA(DisplayName = "Assisted Aim"),
	ManualAim UMETA(DisplayName = "Manual Aim"),
};

USTRUCT(BlueprintType)
struct FAimingData
{
	GENERATED_BODY()

	// Bool to indicate if the ray trace hit something.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Aiming Data")
	bool bHitResult = false;

	// Location of the ray trace hit. Valid only if bIsRayTraceHit is true.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Aiming Data")
	FVector HitLocation = FVector::ZeroVector;

	// World location of the aimer's perspective.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Aiming Data")
	FVector AimingOriginWorldLocation = FVector::ZeroVector;

	// Direction the aimer is looking.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Aiming Data")
	FVector AimingWorldDirection = FVector::ZeroVector;
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

	void AimAt(const FAimingData& AimingData, float LaunchSpeed);

	void AimAtWithNoLaunchSpeed(const FAimingData& AimingData);
	
	UFUNCTION(BlueprintPure)
	ETankFiringStatus GetTankFiringStatus() const { return FiringStatus; }

	UFUNCTION(BlueprintPure)
	EAimingMode GetTankAimingMode() const { return CurrentAimingMode; }

	UFUNCTION(BlueprintCallable)
	void SetTankAimingMode(EAimingMode NewAimingMode);

	UFUNCTION(BlueprintPure)
	bool CanFire() const;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent();

private:
	
	void AssistedAimAt(const FAimingData& AimingData, float LaunchSpeed);

	void DirectAimAt(const FAimingData& AimingData);
	
	void MoveBarrelTowards(const FVector& Target, const FVector& AimDirection);

	bool DoMoveBarrelTowards(const FVector& Target, const FVector& AimDirection);
	ETankFiringStatus ComputeFiringStatus(bool bBarrelMoved) const;

	bool IsBarrelAlreadyAtTarget(const FVector& Target, const FVector& AimDirection) const;

	bool IsAimingAllowed() const;

	std::optional<FVector> GetAssistedAimDirection(const FAimingData& AimingData, float LaunchSpeed) const;

	FVector GetBarrelLocation() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTankBarrelComponent> Barrel{};

	UPROPERTY(Transient)
	TObjectPtr<UTankTurretComponent> Turret{};

	UPROPERTY(Category = Setup, EditDefaultsOnly)
	float AimToleranceDegrees{ 2.5f };

	UPROPERTY(Category = Setup, EditDefaultsOnly)
	EAimingMode DefaultAimingMode{ EAimingMode::AssistedAim };

	ETankFiringStatus FiringStatus{ ETankFiringStatus::NoTarget };

	EAimingMode CurrentAimingMode{ DefaultAimingMode };

	UPROPERTY(EditDefaultsOnly, Category="Tank Aiming")
	float ZeroingDistance = 5000.f;

	float AimToleranceCosine{};

	/*
	* Minimum distance needed for auto aim to be used. 
	  If we are close enough to target just fire directly.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Tank Aiming")
	float AssistedAimTargetThresholdDistanceMeters{ 10.0f };
	float AssistedAimTargetThresholdDistSq{};

	UPROPERTY(EditDefaultsOnly, Category = "Tank Aiming")
	float AssistedAimArcLengthErrorThresholdMeters{ 3.0f };
};

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
	
	UFUNCTION(BlueprintPure)
	ETankFiringStatus GetTankFiringStatus() const { return FiringStatus; }

	UFUNCTION(BlueprintPure)
	EAimingMode GetTankAimingMode() const { return CurrentAimingMode; }

	UFUNCTION(BlueprintCallable)
	void SetTankAimingMode(EAimingMode NewAimingMode);

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent();

private:
	
	void AssistedAimAt(const FAimingData& AimingData, float LaunchSpeed);

	void DirectAimAt(const FAimingData& AimingData);
	
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

	EAimingMode CurrentAimingMode{ EAimingMode::AssistedAim };

	UPROPERTY(EditDefaultsOnly, Category="Tank Aiming")
	float ZeroingDistance = 5000.f;
	
};

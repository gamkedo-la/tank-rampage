// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ArmedActor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "BaseTankPawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UTankAimingComponent;
class UTankTurretComponent;
class UTankBarrelComponent;
class UTankTrackComponent;
class UTankMovementComponent;

class AProjectile;

UCLASS()
class TRTANK_API ABaseTankPawn : public APawn, public IVisualLoggerDebugSnapshotInterface, public IArmedActor
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABaseTankPawn();

	USpringArmComponent* GetCameraSpringArm() const;

	UFUNCTION(BlueprintCallable)
	void AimAt(const FVector& Location);

	UFUNCTION(BlueprintCallable)
	void Fire();

	UFUNCTION(BlueprintCallable)
	bool CanFire() const;

	UFUNCTION(BlueprintCallable)
	void SetLeftThrottle(float Value);

	UFUNCTION(BlueprintCallable)
	void SetRightThrottle(float Value);

	UFUNCTION(BlueprintCallable)
	void MoveForward(float Throw);

	UFUNCTION(BlueprintCallable)
	void TurnRight(float Throw);

	UTankAimingComponent* GetTankAimingComponent() const;

	float GetCurrentWeaponExitSpeed() const;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual void NotifyControllerChanged() override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

private:
	void UpdateSpringArmTickEnabled();


protected:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTankAimingComponent> TankAimingComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTankMovementComponent> TankMovementComponent{};

private:
	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankBody{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTankTurretComponent> TankTurret{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTankBarrelComponent> TankBarrel{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTankTrackComponent> TankTreadRight{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTankTrackComponent> TankTreadLeft{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankTreadSideLeft{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankTreadSideRight{};

	UPROPERTY(Category = "Camera", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraSpringArm{};

	UPROPERTY(Category = "Camera", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera{};

	// TODO: Move to a weapon subclass of item
	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float TankShellSpeed{ 100000 };

	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float FireCooldownTimeSeconds{ 3.0f };

	float LastFireTimeSeconds{ -1.0f };

	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TSubclassOf<AProjectile> MainGunProjectile{};

	// TODO: Placeholder - move to health component
	UPROPERTY(Category = "Health", EditDefaultsOnly)
	float MaxHealth{ 100.0f };

	UPROPERTY(Category = "Health", VisibleInstanceOnly)
	float Health{};
};


#pragma region Inline Definitions

inline USpringArmComponent* ABaseTankPawn::GetCameraSpringArm() const
{
	check(CameraSpringArm);
	return CameraSpringArm;
}

inline UTankAimingComponent* ABaseTankPawn::GetTankAimingComponent() const
{
	check(TankAimingComponent);
	return TankAimingComponent;
}

inline float ABaseTankPawn::GetCurrentWeaponExitSpeed() const
{
	return TankShellSpeed;
}

#pragma endregion Inline Definitions

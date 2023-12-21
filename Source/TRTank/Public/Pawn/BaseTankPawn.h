// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ArmedActor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "AbilitySystemInterface.h"

#include "BaseTankPawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UTankAimingComponent;
class UTankTurretComponent;
class UTankBarrelComponent;
class UTankTrackComponent;
class UTankMovementComponent;
class UHealthComponent;

class AProjectile;
class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class TRTANK_API ABaseTankPawn : public APawn, public IVisualLoggerDebugSnapshotInterface, public IArmedActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABaseTankPawn();

	USpringArmComponent* GetCameraSpringArm() const;

	UFUNCTION(BlueprintCallable)
	void AimAt(const FAimingData& AimingData);

	UFUNCTION(BlueprintCallable)
	void Fire();

	UFUNCTION(BlueprintCallable)
	bool CanFire() const;

	UFUNCTION(BlueprintCallable)
	float GetFireCooldownTimeRemaining() const;

	UFUNCTION(BlueprintCallable)
	float GetFireCooldownProgressPercentage() const;

	UFUNCTION(BlueprintCallable)
	void SetLeftThrottle(float Value);

	UFUNCTION(BlueprintCallable)
	void SetRightThrottle(float Value);

	UFUNCTION(BlueprintCallable)
	void MoveForward(float Throw);

	UFUNCTION(BlueprintCallable)
	void TurnRight(float Throw);

	UTankAimingComponent* GetTankAimingComponent() const;
	UHealthComponent* GetHealthComponent() const;

	float GetCurrentWeaponExitSpeed() const;

	// Inherited via IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UAttributeSet* GetAttributeSet() const;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual void NotifyControllerChanged() override;

private:
	void UpdateSpringArmTickEnabled();

protected:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTankAimingComponent> TankAimingComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTankMovementComponent> TankMovementComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UHealthComponent> HealthComponent{};

	UPROPERTY(Category = "GAS", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent{};

	UPROPERTY(Category = "GAS", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAttributeSet> AttributeSet{};

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

inline UHealthComponent* ABaseTankPawn::GetHealthComponent() const
{
	check(HealthComponent);
	return HealthComponent;
}

inline float ABaseTankPawn::GetCurrentWeaponExitSpeed() const
{
	return TankShellSpeed;
}

inline UAbilitySystemComponent* ABaseTankPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

inline UAttributeSet* ABaseTankPawn::GetAttributeSet() const
{
	return AttributeSet;
}

#pragma endregion Inline Definitions

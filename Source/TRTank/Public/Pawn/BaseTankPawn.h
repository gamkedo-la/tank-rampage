// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ArmedActor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/Damageable.h"

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
class UItemInventory;

class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayEffect;


UCLASS()
class TRTANK_API ABaseTankPawn : public APawn, public IVisualLoggerDebugSnapshotInterface, public IArmedActor, public IAbilitySystemInterface, public IDamageable
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
	virtual UHealthComponent* GetHealthComponent() const override;

	float GetCurrentWeaponExitSpeed() const;

	// Inherited via IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UAttributeSet* GetAttributeSet() const;

	UItemInventory* GetItemInventory() const;


#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual void NotifyControllerChanged() override;

private:
	void UpdateSpringArmTickEnabled();

	void UpdateGameplayAbilitySystemAfterPossession(AController* NewController);

	void InitializeAttributes();

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

	UPROPERTY(Category = "GAS", EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DefaultAttributes{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UItemInventory> ItemInventoryComponent{};

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

inline UAbilitySystemComponent* ABaseTankPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

inline UAttributeSet* ABaseTankPawn::GetAttributeSet() const
{
	return AttributeSet;
}

inline UItemInventory* ABaseTankPawn::GetItemInventory() const
{
	return ItemInventoryComponent;
}

#pragma endregion Inline Definitions

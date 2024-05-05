// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ArmedActor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/Damageable.h"
#include "Damage/DamageAdjustmentOwner.h"

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
class UFlippedOverCorrectionComponent;
class UTankEffectsComponent;
class UTankCollisionDetectionComponent;

class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayEffect;


UCLASS()
class TRTANK_API ABaseTankPawn : public APawn, public IVisualLoggerDebugSnapshotInterface, public IArmedActor, public IAbilitySystemInterface, public IDamageable, public IDamageAdjustmentOwner
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABaseTankPawn();

	USpringArmComponent* GetCameraSpringArm() const;

	UFUNCTION(BlueprintCallable)
	void AimAt(const FAimingData& AimingData);

	UFUNCTION(BlueprintCallable)
	bool Fire();

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

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	UTankAimingComponent* GetTankAimingComponent() const;
	virtual UHealthComponent* GetHealthComponent() const override;

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

	virtual float InternalTakePointDamage(float Damage, FPointDamageEvent const& PointDamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual float InternalTakeRadialDamage(float Damage, struct FRadialDamageEvent const& RadialDamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

private:
	void UpdateSpringArmTickEnabled();

	void UpdateGameplayAbilitySystemAfterPossession(AController* NewController);

	void InitializeAttributes();

	float AdjustDamage(float Damage, AController* EventInstigator, AActor* DamageCauser) const;

#if WITH_EDITOR
	UFUNCTION(CallInEditor)
#endif
	void Kill();

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

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UFlippedOverCorrectionComponent> FlippedOverCorrectionComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UTankEffectsComponent> TankEffectsComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UTankCollisionDetectionComponent> TankCollisionDetectionComponent{};
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

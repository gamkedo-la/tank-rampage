// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "Item/WeaponConfig.h"

#include "Weapon.generated.h"

class AProjectile;


/**
 * An item that can deal damage to enemies.
 */
UCLASS()
class TRITEM_API UWeapon : public UItem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	float GetLaunchSpeed() const;

	virtual bool CanBeActivated() const override;

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) override;
	virtual void BeginDestroy() override;

private:
	void LaunchProjectile(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName);
	void ClearProjectileTimer();
	void UpdateHomingTargets(AProjectile& Projectile);

	TSubclassOf<AProjectile> ChooseProjectileClass() const;

	UFUNCTION()
	void OnProjectileDestroyed(AActor* Actor);

	void OnHomingTargetSelected(AProjectile* Projectile, AActor* Target);

protected:
	UPROPERTY(Category = "Firing", EditDefaultsOnly, BlueprintReadWrite)
	float ProjectileLaunchSpeed{ 100000 };

	UPROPERTY(Category = "Firing", EditDefaultsOnly, BlueprintReadWrite)
	int32 ProjectileCount{ 1 };

	UPROPERTY(Category = "Damage | Type", EditDefaultsOnly, BlueprintReadOnly)
	EWeaponDamageType WeaponDamageType{ EWeaponDamageType::Point };

	/*
	* Max damage that can be dealt.  
	  For point damage weapons this is just the damage amount, but for Radial damage weapons this is the max damage when within the inner radius.
	*/
	UPROPERTY(Category = "Damage", EditDefaultsOnly, BlueprintReadWrite)
	float DamageAmount{ 100.0f };

	/*
	* Min damage that can be dealt within the outer radius (only applicable for WeaponDamageType of "Radial").
	*/
	UPROPERTY(Category = "Damage", EditDefaultsOnly, BlueprintReadWrite)
	float MinDamageAmount{ 10.0f };

	/*
	* Inner radius of max damage (only applicable for WeaponDamageType of "Radial"). 
	*/
	UPROPERTY(Category = "Damage | Radial", EditDefaultsOnly, BlueprintReadWrite)
	float DamageInnerRadius{ 250.0f };

	/*
	* Outer radius of min damage (only applicable for WeaponDamageType of "Radial").
	*/
	UPROPERTY(Category = "Damage | Radial", EditDefaultsOnly, BlueprintReadWrite)
	float DamageOuterRadius{ 1000.0f };

	/*
	* Falloff amount between min and max radius (only applicable for WeaponDamageType of "Radial").
	*/
	UPROPERTY(Category = "Damage | Radial", EditDefaultsOnly, BlueprintReadWrite)
	float DamageFalloff{ 1.0f };


	UPROPERTY(Category = "Homing", EditDefaultsOnly, BlueprintReadWrite)
	bool bIsHoming{};

	UPROPERTY(Category = "Homing", EditDefaultsOnly, BlueprintReadWrite)
	float MaxSpeedMultiplier{ 1.5f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly, BlueprintReadWrite)
	float HomingAcceleration{ 5000.0f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float HomingTargetRefreshInterval{ 0.5f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly, meta = (MustImplement = "/Script/TRTank.Damageable"))
	TArray<TSubclassOf<AActor>> HomingTargetClasses{ };

private:
	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TSubclassOf<AProjectile> WeaponProjectileClass{};

	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TSubclassOf<AProjectile> WeaponHomingProjectileClass{};

	/*
	* Controls the delay time firing additional projectiles if <code>ProjectileCount > 1 </code>.
	*/
	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float ProjectileLaunchPeriod{ 0.2f };

	/*
	* Prevent concurrent firing if multiple projectiles in process of being launched
	*/
	bool bIsFiring{};

	FTimerHandle LaunchDelayTimerHandle{};

	UPROPERTY(Transient)
	TSet<AActor*> AvailableHomingTargets{};
	
	/* Maps projectiles to tracked target actors */
	UPROPERTY(Transient)
	TMap<AProjectile*, AActor*> ProjectileTargetMap{};

	float HomingTargetLastUpdateTime{ -1.0f };
};

#pragma region Inline Definitions

inline float UWeapon::GetLaunchSpeed() const
{
	return ProjectileLaunchSpeed;
}

inline bool UWeapon::CanBeActivated() const
{
	return Super::CanBeActivated() && !bIsFiring;
}

#pragma endregion Inline Definitions


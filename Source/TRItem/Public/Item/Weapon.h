// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "Weapon.generated.h"

class AProjectile;

/**
 * 
 */
UCLASS()
class TRITEM_API UWeapon : public UItem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	float GetLaunchSpeed() const;

	virtual bool CanBeActivated() const;

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) override;
	virtual void BeginDestroy() override;

private:
	void LaunchProjectile(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) const;
	void ClearProjectileTimer();

private:
	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TSubclassOf<AProjectile> WeaponProjectileClass{};

	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float ProjectileLaunchSpeed{ 100000 };

	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	int32 ProjectileCount{ 1 };

	/*
	* Controls the delay time firing additional projectiles if <code>ProjectileCount > 1 </code>.
	*/
	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float ProjectileLaunchPeriod{ 0.2f };

	UPROPERTY(Category = "Damage", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float DamageAmount{ 100.0f };

	/*
	* Prevent concurrent firing if multiple projectiles in process of being launched
	*/
	bool bIsFiring{};

	FTimerHandle LaunchDelayTimerHandle{};
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


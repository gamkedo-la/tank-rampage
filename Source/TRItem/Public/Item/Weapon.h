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

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) override;

private:
	UPROPERTY(Category = "Weapon", EditDefaultsOnly)
	TSubclassOf<AProjectile> WeaponProjectileClass{};

	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float ProjectileLaunchSpeed{ 100000 };

	UPROPERTY(Category = "Damage", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float DamageAmount{ 100.0f };
};

#pragma region Inline Definitions

inline float UWeapon::GetLaunchSpeed() const
{
	return ProjectileLaunchSpeed;
}

#pragma endregion Inline Definitions


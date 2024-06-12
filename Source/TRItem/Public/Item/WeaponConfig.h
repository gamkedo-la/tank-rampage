#pragma once

#include "CoreMinimal.h"

#include "WeaponConfig.generated.h"

UENUM(BlueprintType)
enum class EWeaponDamageType : uint8
{
	Point,
	Radial
};

USTRUCT(BlueprintType)
struct FProjectileDamageParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "Damage", BlueprintReadWrite)
	EWeaponDamageType WeaponDamageType{};

	UPROPERTY(Category = "Damage | Radial", BlueprintReadWrite)
	float MinDamageAmount{};

	UPROPERTY(Category = "Damage", BlueprintReadWrite)
	float MaxDamageAmount{};

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

	/*
	* Multiplier on base impact impulse amount.
	*/
	UPROPERTY(Category = "Impulse", EditDefaultsOnly, BlueprintReadWrite, meta = (clampMin="0"))
	float ImpactImpulseAmountMultiplier{ 1.0f };
};

USTRUCT(BlueprintType)
struct FProjectileHomingParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "Homing", BlueprintReadWrite)
	float MaxSpeedMultiplier{ 0.0f };

	UPROPERTY(Category = "Homing", BlueprintReadWrite)
	float HomingAcceleration{ 0.0f };

	UPROPERTY(Category = "Homing", BlueprintReadWrite)
	float HomingTargetRefreshInterval{ 0.5f };

	UPROPERTY(Transient)
	TSet<AActor*> Targets{};

	/* Other projectile's targets */
	UPROPERTY(Transient)
	TSet<AActor*> UsedTargets{};
};
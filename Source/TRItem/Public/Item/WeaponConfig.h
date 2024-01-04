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
};
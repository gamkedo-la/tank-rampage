// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "Item/WeaponConfig.h"

#include "Weapon.generated.h"


/**
 * An item that can deal damage to enemies.
 */
UCLASS(Abstract)
class TRITEM_API UWeapon : public UItem
{
	GENERATED_BODY()

public:

	UWeapon();

	UFUNCTION(BlueprintPure)
	virtual float GetLaunchSpeed() const { return 0.0f; }

	UFUNCTION(BlueprintPure)
	virtual bool IsLaunchable() const { return false; }

};

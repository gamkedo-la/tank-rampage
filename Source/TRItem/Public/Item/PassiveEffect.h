// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "PassiveEffect.generated.h"

/**
 * An item that is a passive effect such as health, armor, and shields.  
 * Some passive effects may have cooldowns like shields,
 * and others like health and armor are just applied once per level.
 * Passive effects are not activated, but rather persist after initialization.
 */
UCLASS(Abstract)
class TRITEM_API UPassiveEffect : public UItem
{
	GENERATED_BODY()

public:
	virtual bool CanBeActivated() const override { return false; }

	UFUNCTION(BlueprintPure)
	float GetCurrentValue() { return CurrentValue; }

	UFUNCTION(BlueprintPure)
	float GetMaxValue() { return MaxValue; }

protected:
	UPROPERTY(BlueprintReadWrite)
	float CurrentValue{};

	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite)
	float MaxValue{};
};

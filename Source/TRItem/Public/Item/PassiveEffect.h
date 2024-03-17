// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "Interfaces/Percentage.h"

#include "PassiveEffect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemValueChanged, const class UPassiveEffect*, Item, float, CurrentValue, float, PreviousValue);


/**
 * An item that is a passive effect such as health, armor, and shields.  
 * Some passive effects may have cooldowns like shields,
 * and others like health and armor are just applied once per level.
 * Passive effects are not activated, but rather persist after initialization.
 */
UCLASS(Abstract)
class TRITEM_API UPassiveEffect : public UItem, public IPercentage
{
	GENERATED_BODY()

public:

	UPassiveEffect();

	virtual bool CanBeActivated() const override { return false; }

	virtual float GetCurrentValue() const override { return CurrentValue; }

	virtual float GetMaxValue() const override { return MaxValue; }

public:
	UPROPERTY(Transient, Category = "Notification", BlueprintAssignable)
	FOnItemValueChanged OnItemValueChanged{};

protected:
	class FCurrentValueChangedWatcher
	{
	public:
		explicit FCurrentValueChangedWatcher(const UPassiveEffect& Effect);
		~FCurrentValueChangedWatcher();

		FCurrentValueChangedWatcher(const FCurrentValueChangedWatcher&) = delete;
		FCurrentValueChangedWatcher& operator=(const FCurrentValueChangedWatcher&) = delete;

	private:
		const UPassiveEffect* Effect;
		float SnapshotValue;
	};

protected:
	UPROPERTY(BlueprintReadWrite)
	float CurrentValue{};

	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite)
	float MaxValue{};
};

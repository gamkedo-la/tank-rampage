// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "Interfaces/Percentage.h"

#include "PassiveEffect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnItemValueChanged, const class UPassiveEffect*, Item, float, CurrentValue, float, PreviousValue, float, MaxValue, float, PreviousMaxValue);


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

	UFUNCTION(Category = "Item", BlueprintCallable, meta=(DisplayName = "Update Current Value"))
	bool SetCurrentValue(float Value);

protected:
	virtual void OnValueChanged(float PreviousValue) {}

public:
	UPROPERTY(Transient, Category = "Notification", BlueprintAssignable)
	FOnItemValueChanged OnItemValueChanged{};

protected:

	virtual void BeforeOnLevelChanged(int32 NewLevel, int32 PreviousLevel) override;
	virtual void AfterOnLevelChanged(int32 NewLevel, int32 PreviousLevel) override;

	class FCurrentValueChangedWatcher
	{
	public:
		explicit FCurrentValueChangedWatcher(UPassiveEffect& Effect);
		~FCurrentValueChangedWatcher();

		FCurrentValueChangedWatcher(const FCurrentValueChangedWatcher&) = delete;
		FCurrentValueChangedWatcher& operator=(const FCurrentValueChangedWatcher&) = delete;

		bool IsChanged() const;

	private:
		UPassiveEffect* Effect;
		float SnapshotValue;
	};

protected:
	UPROPERTY(BlueprintReadWrite)
	float CurrentValue{};

	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite)
	float MaxValue{};

private:

	float CurrentValueBeforeLevelChange{};
	float MaxValueBeforeLevelChange{};
};

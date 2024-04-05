// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/PassiveEffect.h"
#include "ArmorItem.generated.h"

/**
 * 
 */
UCLASS()
class TRITEM_API UArmorItem : public UPassiveEffect
{
	GENERATED_BODY()

protected:
	virtual void NativeInitialize(const FItemConfigData& ItemConfigData) override;

	virtual void BeforeOnLevelChanged(int32 NewLevel, int32 PreviousLevel) override;
	virtual void AfterOnLevelChanged(int32 NewLevel, int32 PreviousLevel) override;

private:
	float OnCalculateDamage(float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser);
	
private:

	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float ArmorDecayRateOnDamage{ 0.25f };

	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ArmorDecayZeroThreshold{ 10.0f };

	float CurrentValueBeforeLevelChange{};
	float MaxValueBeforeLevelChange{};
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/PassiveEffect.h"
#include "ShieldItem.generated.h"

/**
 * 
 */
UCLASS()
class TRITEM_API UShieldItem : public UPassiveEffect
{
	GENERATED_BODY()

public:
	virtual bool CanBeActivated() const override;

protected:
	virtual void NativeInitialize(const FItemConfigData& ItemConfigData) override;
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) override;

private:

	float OnCalculateDamage(float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser);

	void Recharge();

private:

	int32 TimesUsed{};
	
};

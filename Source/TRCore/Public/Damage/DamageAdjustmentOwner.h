// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageAdjustmentOwner.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageAdjustment, float&, Damage, const AActor*, DamagedActor, const AController*, InstigatedBy, const AActor*, DamageCauser);

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UDamageAdjustmentOwner : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TRCORE_API IDamageAdjustmentOwner
{
	GENERATED_BODY()

public:

	virtual FOnDamageAdjustment& GetOnDamageAdjustment() = 0;


	static IDamageAdjustmentOwner* GetFromActor(AActor* Actor);
};

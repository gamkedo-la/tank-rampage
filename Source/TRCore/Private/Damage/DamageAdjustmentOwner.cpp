// Fill out your copyright notice in the Description page of Project Settings.


#include "Damage/DamageAdjustmentOwner.h"

IDamageAdjustmentOwner* IDamageAdjustmentOwner::GetFromActor(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	auto DamageAdjustmentOwner = Cast<IDamageAdjustmentOwner>(Actor);
	if(DamageAdjustmentOwner)
	{
		return DamageAdjustmentOwner;
	}

	return Cast<IDamageAdjustmentOwner>(Actor->FindComponentByInterface<UDamageAdjustmentOwner>());
}

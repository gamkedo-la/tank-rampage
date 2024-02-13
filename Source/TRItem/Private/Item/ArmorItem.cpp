// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ArmorItem.h"

#include "Damage/DamageAdjustmentOwner.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

void UArmorItem::NativeInitialize(const FItemConfigData& ItemConfigData)
{
	auto DamageAdjustmentOwner = IDamageAdjustmentOwner::GetFromActor(GetOwner());

	if (!ensureMsgf(DamageAdjustmentOwner, TEXT("%s: unable to get IDamageAdjustmentOwner from Owner=%s"), *GetName(), *LoggingUtils::GetName(GetOwner())))
	{
		return;
	}

	CurrentValue = MaxValue;

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: NativeInitialize: MaxValue=%f"), *GetName(), MaxValue);

	// FIXME: Shield needs to adjust first before armor!  Need some way of doing registration order
	// A simple way to account for this for now is to specify shield first in data table so that it registers before armor
	// and delegates are stored in a list in registration order so they will then fire in correct sequence
	// This doesn't work as the shield is earned but you start out with armor so order will be wrong
	// Need to handle this somehow in the damage adjustment owner and have it reflected in a parameter
	DamageAdjustmentOwner->GetOnDamageAdjustment().AddDynamic(this, &ThisClass::OnCalculateDamage);
}

void UArmorItem::OnCalculateDamage(float& Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Verbose, TEXT("%s: OnCalculateDamage: Damage=%f; DamagedActor=%s; InstigatedBy=%s; DamageCauser=%s"),
		*GetName(), Damage, *LoggingUtils::GetName(DamagedActor), *LoggingUtils::GetName(InstigatedBy), *LoggingUtils::GetName(DamageCauser));

	if (Damage <= 0)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: Skipping zero or negative damage=%f"), *GetName(), Damage);
		return;
	}

	if (FMath::IsNearlyZero(CurrentValue))
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: Skipping since armor is depleted"), *GetName(), Damage);
		return;
	}

	const float AbsorbAmount = FMath::Min(CurrentValue, Damage);
	float DecayAmount = AbsorbAmount * ArmorDecayRateOnDamage;

	// If armor decays to below 1 HP then just clamp to 0
	if (CurrentValue - DecayAmount < 1.0f)
	{
		DecayAmount = CurrentValue;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: DamageReduction=%f/%f; Armor=%f/%f"),
		*GetName(), AbsorbAmount, Damage, CurrentValue - DecayAmount, MaxValue);

	Damage -= AbsorbAmount;
	CurrentValue -= DecayAmount;
}

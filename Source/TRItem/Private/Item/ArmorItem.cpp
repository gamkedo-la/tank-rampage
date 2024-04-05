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

	DamageAdjustmentOwner->RegisterDamageAdjustment(this, [this](float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser)
	{
		return OnCalculateDamage(Damage, DamagedActor, InstigatedBy, DamageCauser);
	}, 100);
}

void UArmorItem::BeforeOnLevelChanged(int32 NewLevel, int32 PreviousLevel)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: BeforeOnLevelChanged: NewLevel=%d; CurrentValue=%f/%f"), *GetName(), NewLevel, CurrentValue, MaxValue);

	CurrentValueBeforeLevelChange = CurrentValue;
	MaxValueBeforeLevelChange = MaxValue;
}

void UArmorItem::AfterOnLevelChanged(int32 NewLevel, int32 PreviousLevel)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: AfterOnLevelChanged: NewLevel=%d; CurrentValue=%f/%f"), *GetName(), NewLevel, CurrentValue, MaxValue);

	// fire off event if value changed

	if (FMath::IsNearlyEqual(CurrentValueBeforeLevelChange, CurrentValue) && FMath::IsNearlyEqual(MaxValueBeforeLevelChange, MaxValue))
	{
		return;
	}

	OnItemValueChanged.Broadcast(this, CurrentValue, CurrentValueBeforeLevelChange, MaxValue, MaxValueBeforeLevelChange);
}

float UArmorItem::OnCalculateDamage(float Damage, const AActor* DamagedActor, const AController* InstigatedBy, const AActor* DamageCauser)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Verbose, TEXT("%s: OnCalculateDamage: Damage=%f; DamagedActor=%s; InstigatedBy=%s; DamageCauser=%s"),
		*GetName(), Damage, *LoggingUtils::GetName(DamagedActor), *LoggingUtils::GetName(InstigatedBy), *LoggingUtils::GetName(DamageCauser));

	if (Damage <= 0)
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: Skipping zero or negative damage=%f"), *GetName(), Damage);
		return Damage;
	}

	if (FMath::IsNearlyZero(CurrentValue))
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: Skipping since armor is depleted"), *GetName());
		return Damage;
	}

	FCurrentValueChangedWatcher ChangeWatcher(*this);

	const float AbsorbAmount = FMath::Min(CurrentValue, Damage);
	float DecayAmount = AbsorbAmount * ArmorDecayRateOnDamage;

	// If armor decays to below the configured HP then just clamp to 0
	if (CurrentValue - DecayAmount < ArmorDecayZeroThreshold)
	{
		DecayAmount = CurrentValue;
	}

	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: OnCalculateDamage: DamageReduction=%f/%f; Armor=%f/%f"),
		*GetName(), AbsorbAmount, Damage, CurrentValue - DecayAmount, MaxValue);

	Damage -= AbsorbAmount;
	CurrentValue -= DecayAmount;

	return Damage;
}

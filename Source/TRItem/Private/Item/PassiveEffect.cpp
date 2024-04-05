// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PassiveEffect.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

UPassiveEffect::UPassiveEffect()
{
	ItemType = EItemType::PassiveEffect;
}

void UPassiveEffect::BeforeOnLevelChanged(int32 NewLevel, int32 PreviousLevel)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: BeforeOnLevelChanged: NewLevel=%d; CurrentValue=%f/%f"), *GetName(), NewLevel, CurrentValue, MaxValue);

	CurrentValueBeforeLevelChange = CurrentValue;
	MaxValueBeforeLevelChange = MaxValue;
}

void UPassiveEffect::AfterOnLevelChanged(int32 NewLevel, int32 PreviousLevel)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: AfterOnLevelChanged: NewLevel=%d; CurrentValue=%f/%f"), *GetName(), NewLevel, CurrentValue, MaxValue);

	// fire off event if value changed

	if (FMath::IsNearlyEqual(CurrentValueBeforeLevelChange, CurrentValue) && FMath::IsNearlyEqual(MaxValueBeforeLevelChange, MaxValue))
	{
		return;
	}

	OnItemValueChanged.Broadcast(this, CurrentValue, CurrentValueBeforeLevelChange, MaxValue, MaxValueBeforeLevelChange);
}

UPassiveEffect::FCurrentValueChangedWatcher::FCurrentValueChangedWatcher(const UPassiveEffect& Effect) : 
	Effect(&Effect), SnapshotValue(Effect.CurrentValue)
{
}

UPassiveEffect::FCurrentValueChangedWatcher::~FCurrentValueChangedWatcher()
{
	if (FMath::IsNearlyEqual(Effect->CurrentValue, SnapshotValue))
	{
		return;
	}

	UE_VLOG_UELOG(Effect->GetOuter(), LogTRItem, Log, TEXT("%s: Broadcast OnValueChanged event %f -> %f"),
		*Effect->GetName(), SnapshotValue, Effect->CurrentValue);

	const auto EffectMaxValue = Effect->GetMaxValue();

	Effect->OnItemValueChanged.Broadcast(Effect, Effect->CurrentValue, SnapshotValue, EffectMaxValue, EffectMaxValue);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PassiveEffect.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

UPassiveEffect::UPassiveEffect()
{
	ItemType = EItemType::PassiveEffect;
}

bool UPassiveEffect::SetCurrentValue(float Value)
{
	UE_VLOG_UELOG(GetOuter(), LogTRItem, Log, TEXT("%s: SetCurrentValue: %f -> %f (Max=%f)"),
		*GetName(), CurrentValue, Value, MaxValue);

	const auto ClampedValue = FMath::Clamp(Value, 0, MaxValue);

	if (!FMath::IsNearlyEqual(Value, ClampedValue))
	{
		UE_VLOG_UELOG(GetOuter(), LogTRItem, Warning, TEXT("%s: SetCurrentValue: Value=%f was outside [0,%f] and clamped to %f"),
			*GetName(), Value, MaxValue, ClampedValue);
	}

	FCurrentValueChangedWatcher ChangeWatcher(*this);

	CurrentValue = ClampedValue;

	return ChangeWatcher.IsChanged();
}

UPassiveEffect::FCurrentValueChangedWatcher::FCurrentValueChangedWatcher(UPassiveEffect& Effect) : 
	Effect(&Effect), SnapshotValue(Effect.CurrentValue)
{
}

UPassiveEffect::FCurrentValueChangedWatcher::~FCurrentValueChangedWatcher()
{
	if (!IsChanged())
	{
		return;
	}

	Effect->OnValueChanged(SnapshotValue);

	UE_VLOG_UELOG(Effect->GetOuter(), LogTRItem, Log, TEXT("%s: Broadcast OnValueChanged event %f -> %f"),
		*Effect->GetName(), SnapshotValue, Effect->CurrentValue);

	Effect->OnItemValueChanged.Broadcast(Effect, Effect->CurrentValue, SnapshotValue);
}

bool UPassiveEffect::FCurrentValueChangedWatcher::IsChanged() const
{
	return !FMath::IsNearlyEqual(Effect->CurrentValue, SnapshotValue);
}

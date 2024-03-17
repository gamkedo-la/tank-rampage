// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PassiveEffect.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

UPassiveEffect::UPassiveEffect()
{
	ItemType = EItemType::PassiveEffect;
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

	Effect->OnItemValueChanged.Broadcast(Effect, Effect->CurrentValue, SnapshotValue);
}

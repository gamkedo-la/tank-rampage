// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PassiveEffect.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PassiveEffect)

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

void UPassiveEffect::NativeToString(FString& Str) const
{
	Super::NativeToString(Str);

	Str.Appendf(TEXT(";CurrentValue=%.1f;MaxValue=%.1f"), CurrentValue, MaxValue);
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

	const auto EffectMaxValue = Effect->GetMaxValue();

	Effect->OnItemValueChanged.Broadcast(Effect, Effect->CurrentValue, SnapshotValue, EffectMaxValue, EffectMaxValue);
}

bool UPassiveEffect::FCurrentValueChangedWatcher::IsChanged() const
{
	return !FMath::IsNearlyEqual(Effect->CurrentValue, SnapshotValue);
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

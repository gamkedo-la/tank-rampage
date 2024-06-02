// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ActivatableEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ActivatableEffect)

UActivatableEffect::UActivatableEffect()
{
	ItemType = EItemType::ActivatableEffect;
}

float UActivatableEffect::GetActivationTimeRemaining() const
{
	auto World = GetWorld();
	check(World);

	const auto LastActivation = GetLastActivationTimeSeconds();

	if (LastActivation < 0)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, DurationSeconds - (World->GetTimeSeconds() - LastActivation));
}

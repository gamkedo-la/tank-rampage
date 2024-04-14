// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "ActivatableEffect.generated.h"

/**
 * An effect that has to be activated such as a turbo boost.
 */
UCLASS(Abstract)
class TRITEM_API UActivatableEffect : public UItem
{
	GENERATED_BODY()

public:
	UActivatableEffect();

	UFUNCTION(BlueprintPure)
	bool IsActive() const;

	UFUNCTION(BlueprintPure)
	float GetActivationTimeRemaining() const;

	UFUNCTION(BlueprintPure)
	float GetActivationPercentRemaining() const;

protected:
	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite)
	float DurationSeconds{ 5.0f };
	
};

#pragma region Inline Definitions

inline bool UActivatableEffect::IsActive() const
{
	return GetActivationTimeRemaining() > 0;
}

inline float UActivatableEffect::GetActivationPercentRemaining() const
{
	const auto TimeRemaining = GetActivationTimeRemaining();
	return TimeRemaining > 0.0f ? TimeRemaining / DurationSeconds : 0.0f;
}

#pragma endregion Inline Definitions

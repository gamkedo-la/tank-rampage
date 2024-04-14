// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/ActivatableEffect.h"
#include "GameplayTagContainer.h"

#include "AttributeModifierEffect.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class TRITEM_API UAttributeModifierEffect : public UActivatableEffect
{
	GENERATED_BODY()

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) override;
	virtual void BeginDestroy() override;

private:
	void ScheduleEffectRemoval();
	void RemoveEffect();

	bool ApplyEffectToSelf(const FGameplayTagContainer& TagsContainer);

protected:

	/*
	* Multiplier on attribute specified by buff tags.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly, BlueprintReadWrite)
	float AttributeMultiplier{ 1.0f };

private:

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<FGameplayTag> BuffTags;

	FTimerHandle TagExpirationHandle{};
	int32 LastTagCount{};
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Weapon.h"
#include "GameplayTagContainer.h"

#include "EMPWeapon.generated.h"

class UAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class TRITEM_API UEMPWeapon : public UWeapon
{
	GENERATED_BODY()

protected:
	virtual bool DoActivation(USceneComponent& ActivationReferenceComponent, const FName& ActivationSocketName) override;
	virtual void BeginDestroy() override;

	TArray<APawn*> SweepForAffectedEnemies() const;

private:
	void CheckRemoveStunTag();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float InfluenceRadius{ 5000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float EffectDuration{ 5.0f };

	UPROPERTY(Transient)
	TMap<UAbilitySystemComponent*, float> AffectedActors;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<FGameplayTag> DebuffTags;

	FTimerHandle TagExpirationHandle;
};

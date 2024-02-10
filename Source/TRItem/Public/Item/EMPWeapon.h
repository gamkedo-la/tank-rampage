// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Weapon.h"
#include "GameplayTagContainer.h"

#include "EMPWeapon.generated.h"

class UAbilitySystemComponent;
class UNiagaraSystem;
class UNiagaraComponent;


USTRUCT()
struct FEMPAffectedActorData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Transient)
	UNiagaraComponent* Vfx{};

	float EndGameTimeSeconds{};
};

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
	void ScheduleStunRemoval(float DeltaTime);

	void PlayActivationVfx();

	void ApplyEffectToEnemy(AActor* Enemy, float EffectEndGameTimeSeconds, const FGameplayTagContainer& DebuffTagsContainer);

	UNiagaraComponent* PlayAffectedEnemyVfx(AActor* Enemy);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Effect")
	float InfluenceRadius{ 5000.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Effect")
	float EffectDuration{ 5.0f };

private:

	UPROPERTY(Transient)
	TMap<UAbilitySystemComponent*, FEMPAffectedActorData> AffectedActors;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<FGameplayTag> DebuffTags;

	FTimerHandle TagExpirationHandle;

	UPROPERTY(Category = "Effects | Activation", EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> ActivationVfx{};

	UPROPERTY(Category = "Effects | Activation", EditDefaultsOnly)
	FName EffectRadiusName{ TEXT("Radius") };

	UPROPERTY(Category = "Effects | Activation", EditDefaultsOnly)
	FName OwnerRelativeVelocityName{ TEXT("Relative Velocity") };

	UPROPERTY(Category = "Effects | Enemy", EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> AffectedEnemyVfx{};

	UPROPERTY(Category = "Effects | Enemy", EditDefaultsOnly)
	FName EnemyRadiusName{ TEXT("Radius") };
};

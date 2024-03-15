// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TankEffectsComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class ABaseTankPawn;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTankEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTankEffectsComponent();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent)
	void SetNiagaraDeathEffectParameters(UNiagaraComponent* NiagaraComponent);

private:
	void PlayDeathVfx();

	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	FVector GetExplosionLocation() const;

private:
	UPROPERTY(Category = "Death", EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> DeathVfx{};

	UPROPERTY(Category = "Death", EditDefaultsOnly)
	FName DeathVfxScaleParameterName{};

	UPROPERTY(Category = "Death", EditDefaultsOnly)
	float DeathVfxScaleParameterValue{ 1.0f };

	UPROPERTY(Category = "Death", EditDefaultsOnly)
	float ExplosionZOffset{ 0.0f };
};

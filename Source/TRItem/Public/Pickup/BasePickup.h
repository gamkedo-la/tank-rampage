// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasePickup.generated.h"

class UGameplayEffect;
class USoundBase;

UCLASS(Abstract)
class TRITEM_API ABasePickup : public AActor
{
	GENERATED_BODY()
	
public:	
	ABasePickup();

	UFUNCTION(BlueprintPure)
	float GetSpawnOffsetZ() const;

	UFUNCTION(BlueprintPure)
	bool IsMarkedForDestroy() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	virtual void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	UFUNCTION(BlueprintCallable)
	void RegisterOverlapEvent(UPrimitiveComponent* OverlapCheckComponent);

	/*
	* Handle overlap with player pawn.
	*/
	virtual void OnOverlap(APawn* PlayerPawn) {}

	/*
	* Handle overlap with player pawn.
	*/
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "OnOverlap"))
	void ReceiveOnOverlap(APawn* PlayerPawn);

	/*
	* Indicate that this actor should be destroyed once the current frame's processing is complete.
	*/
	UFUNCTION(BlueprintCallable)
	void MarkForDestroy();

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void PlayPickupSfx();

	void SetLifetimeIfApplicable();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;

private:

	/* How long the pickup persists after spawning.  Set to a value > 0, to limit the lifetime for spawned pickups.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Lifetime")
	float LifetimeAfterSpawn{};

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	float SpawnOffsetZ{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> PickupSfx{};

	bool bMarkedForDestroy{};
};

#pragma region Inline Definitions

inline float ABasePickup::GetSpawnOffsetZ() const
{
	return SpawnOffsetZ;
}

inline bool ABasePickup::IsMarkedForDestroy() const
{
	return bMarkedForDestroy;
}
#pragma endregion Inline Definitions

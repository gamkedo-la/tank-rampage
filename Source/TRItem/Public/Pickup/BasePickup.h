// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasePickup.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class TRITEM_API ABasePickup : public AActor
{
	GENERATED_BODY()
	
public:	
	ABasePickup();

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

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;
};

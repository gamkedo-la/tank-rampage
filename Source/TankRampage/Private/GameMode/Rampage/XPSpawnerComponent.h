// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XPSpawnerComponent.generated.h"

class AXPToken;
class ABaseTankPawn;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UXPSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UXPSpawnerComponent();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);

	void SpawnToken(const FVector& Location, const AController& Owner) const;

	FVector GetSpawnLocation(const FVector& BaseLocation) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "XP")
	TSubclassOf<AXPToken> XPTokenClass{};
};

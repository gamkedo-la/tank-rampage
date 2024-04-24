// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SpawnPoint.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRCORE_API USpawnPoint : public USceneComponent
{
	GENERATED_BODY()

public:	
	USpawnPoint();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Category="Spawn", EditDefaultsOnly)
	TSubclassOf<AActor> ActorToSpawn{};
};

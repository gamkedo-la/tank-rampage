// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemySpawnerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UEnemySpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEnemySpawnerComponent();

protected:
	virtual void BeginPlay() override;
};

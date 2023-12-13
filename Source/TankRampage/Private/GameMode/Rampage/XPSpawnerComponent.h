// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XPSpawnerComponent.generated.h"

class AXPToken;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UXPSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UXPSpawnerComponent();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "XP")
	TSubclassOf<AXPToken> XPTokenClass{};
};

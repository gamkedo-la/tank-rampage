// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "RampageGameMode.generated.h"

class UXPSpawnerComponent;

/**
 * 
 */
UCLASS()
class ARampageGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARampageGameMode();

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UXPSpawnerComponent> XPSpawnerComponent{};
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "RampageGameMode.generated.h"

class UXPSpawnerComponent;
class AXPToken;
class UXPCollectionComponent;

/**
 * 
 */
UCLASS()
class ARampageGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARampageGameMode();

	void OnTokenCollected(const AXPToken& Token);

protected:
	virtual void BeginPlay() override;

	void AddXP(int32 XP);

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UXPSpawnerComponent> XPSpawnerComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UXPCollectionComponent> XPCollectionComponent{};

	UPROPERTY(Category = "XP", EditDefaultsOnly)
	int32 TokenXPAmount{ 1 };

	UPROPERTY(Category = "XP", EditDefaultsOnly)
	TArray<int32> XPLevels{};
};
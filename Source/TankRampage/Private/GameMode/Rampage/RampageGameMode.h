// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "RampageGameMode.generated.h"

class UXPSpawnerComponent;
class AXPToken;
class UXPCollectionComponent;
class ULevelUnlocksComponent;

class ABaseTank;

/**
 * 
 */
UCLASS()
class ARampageGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARampageGameMode();

	void OnTokenCollected(const AXPToken& Token, APawn* PlayerPawn);

protected:
	virtual void BeginPlay() override;

	void AddXP(APawn* PlayerPawn, int32 XP);

private:

	void InitializeGameState();
	void RegisterEvents();

	UFUNCTION()
	void OnTankDestroyed(ABaseTankPawn* DestroyedTank, AController* DestroyedBy, AActor* DestroyedWith);


private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UXPSpawnerComponent> XPSpawnerComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UXPCollectionComponent> XPCollectionComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<ULevelUnlocksComponent> LevelUnlocksComponent{};

	UPROPERTY(Category = "XP", EditDefaultsOnly)
	int32 TokenXPAmount{ 1 };

	UPROPERTY(Category = "XP", EditDefaultsOnly)
	TArray<int32> XPLevels{};
};

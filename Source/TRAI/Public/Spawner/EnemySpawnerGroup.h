// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnerGroup.generated.h"

class AEnemySpawner;

UCLASS()
class TRAI_API AEnemySpawnerGroup : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemySpawnerGroup();

	const TArray<AEnemySpawner*>& GetSpawners() const;

protected:

	UFUNCTION(BlueprintCallable)
	void Initialize(UPrimitiveComponent* InOverlapComponent);

private:

	UPROPERTY(Transient)
	TArray<AEnemySpawner*> Spawners;
};

#pragma region Inline Definitions

inline const TArray<AEnemySpawner*>& AEnemySpawnerGroup::GetSpawners() const
{
	return Spawners;
}

#pragma endregion Inline Definitions

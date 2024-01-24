// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class USpawnLocationComponent;

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanSpawnEnemy, const TSubclassOf<APawn>& /*Enemy Type*/);

UCLASS(Abstract)
class TRAI_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemySpawner();

	int32 Spawn(int32 DesiredCount, TArray<APawn*>& OutSpawned);
	int32 GetMaxSpawnCount() const;
	bool CanSpawnAnyFor(const APawn& PlayerPawn) const;

	float GetLastSpawnGameTime() const;
	float GetTimeSinceLastSpawn() const;

	FCanSpawnEnemy CanSpawnEnemy{};

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void Initialize(UPrimitiveComponent* InOverlapComponent);

private:

	UClass* SelectSpawnClass() const;

	FVector GetSpawnReferenceLocation() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> NoSpawnZoneComponent{};

	UPROPERTY(Transient)
	TArray<USpawnLocationComponent*> SpawnLocations{};

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpawnLocationComponent> FirstSpawnLocation{};

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TSubclassOf<APawn>> SpawningTypes;

	float LastSpawnTime{ -1.f };
	bool bPlayerInSpawnZone{};

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float MinimumDistanceFOV{};

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float MinimumDistanceNotFOV{};

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float MaxDistance{};
};

#pragma region Inline Definitions

inline float AEnemySpawner::GetLastSpawnGameTime() const
{
	return LastSpawnTime;
}

inline int32 AEnemySpawner::GetMaxSpawnCount() const
{
	return SpawnLocations.Num();
}
#pragma endregion Inline Definitions
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include <random>

#include "EnemySpawner.generated.h"

class USpawnLocationComponent;

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanSpawnEnemy, const TSubclassOf<APawn>& /*Enemy Type*/);

UCLASS(Abstract)
class TRAI_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemySpawner();

	int32 Spawn(int32 DesiredCount, const AActor* LookAtActor = nullptr, TArray<APawn*>* OutSpawned = nullptr);
	int32 GetMaxSpawnCount() const;
	bool CanSpawnAnyFor(const APawn& PlayerPawn, float* OutScore = nullptr) const;

	/*
	* First-level quick check based on max distance
	*/
	bool ShouldBeConsideredForSpawning(const APawn& PlayerPawn, float ConsiderationRadiusSq) const;

	bool IsCoolingDown() const;

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

	float CalculateSpawningScore(float DistanceFromPlayer, float MinDistance, bool bInFOV) const;

	void GroundSpawnPoints();
	void GroundSpawnPoint(USpawnLocationComponent& SpawnLocation);


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

	/*
	* Ideal distance from player between min and max for spawning.
	*/
	UPROPERTY(EditAnywhere, Category = "Spawning")
	float IdealSpawnDistanceRatio{ 0.5f };

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float MaxDistance{};

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float CooldownTime{ 10.0f };

	UPROPERTY(EditAnywhere, Category = "Spawning | Score")
	float FOVScoreMultiplier{ 2.0f};

	UPROPERTY(EditAnywhere, Category = "Spawning | Score")
	float DistanceUnitMultiplier{ 0.005f };

	UPROPERTY(EditAnywhere, Category = "Spawning | Grounding")
	float GroundTraceUpOffset{ 5 * 100.0f };

	UPROPERTY(EditAnywhere, Category = "Spawning | Grounding")
	float GroundTraceDownOffset{ 10 * 100.0f };

	UPROPERTY(EditAnywhere, Category = "Spawning | Grounding")
	float GroundEpsilon{ 1e-3f };

	std::default_random_engine Rng;
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

inline bool AEnemySpawner::IsCoolingDown() const
{
	return LastSpawnTime >= 0 && GetTimeSinceLastSpawn() <= CooldownTime;
}

#pragma endregion Inline Definitions
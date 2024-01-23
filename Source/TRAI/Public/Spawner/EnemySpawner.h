// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanSpawnEnemy, const TSubclassOf<APawn>& /*Enemy Type*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnRelevancyChange, AEnemySpawner* /*Spawner*/, APawn* /*PlayerPawn*/, bool /*bIsrelevant*/);

UCLASS(Abstract)
class TRAI_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemySpawner();

	APawn* Spawn();

	FCanSpawnEnemy CanSpawnEnemy{};
	FOnRelevancyChange OnRelevancyChange{};

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void Initialize(UPrimitiveComponent* InOverlapComponent);

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RegisterOverlaps();
	void DeregisterOverlaps();

	UClass* SelectSpawnClass() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> OverlapComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<USceneComponent> SpawnPointComponent{};

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TSubclassOf<APawn>> SpawningTypes;
};

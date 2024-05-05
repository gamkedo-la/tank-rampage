// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TankCollisionDetectionComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRelevantCollision, const FHitResult& /* Hit*/, const FVector& /*NormalImpulse*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTankCollisionDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTankCollisionDetectionComponent();

	FOnRelevantCollision OnRelevantCollision{};

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	bool IsRelevantCollision(const FHitResult& Hit) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	TArray<FString> ActorSubstrings;

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	TArray<FString> ComponentSubstrings;

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypes;
};

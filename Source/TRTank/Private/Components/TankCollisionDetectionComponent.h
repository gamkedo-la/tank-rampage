// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BaseCollisionRelevanceComponent.h"

#include "TankCollisionDetectionComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRelevantCollision, const FHitResult& /* Hit*/, const FVector& /*NormalImpulse*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTankCollisionDetectionComponent : public UBaseCollisionRelevanceComponent
{
	GENERATED_BODY()

public:	
	FOnRelevantCollision OnRelevantCollision{};

protected:
	virtual void RegisterCollisions() override;
	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;
};

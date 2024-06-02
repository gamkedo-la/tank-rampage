// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankCollisionDetectionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TankCollisionDetectionComponent)

void UTankCollisionDetectionComponent::RegisterCollisions()
{
	RegisterOwner();
}

void UTankCollisionDetectionComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	OnRelevantCollision.Broadcast(Hit, NormalImpulse);
}

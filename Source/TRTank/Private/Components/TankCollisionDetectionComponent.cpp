// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankCollisionDetectionComponent.h"


void UTankCollisionDetectionComponent::RegisterCollisions()
{
	RegisterOwner();
}

void UTankCollisionDetectionComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	OnRelevantCollision.Broadcast(Hit, NormalImpulse);
}

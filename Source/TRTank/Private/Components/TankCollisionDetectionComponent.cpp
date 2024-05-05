// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TankCollisionDetectionComponent.h"

#include "Utils/CollisionUtils.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "VisualLogger/VisualLogger.h"
#include "Debug/TRDebugUtils.h"


namespace
{
	bool MatchesByName(const TArray<FString>& Array, const FString& InputName);
}

UTankCollisionDetectionComponent::UTankCollisionDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTankCollisionDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	auto MyOwner = GetOwner();
	check(MyOwner);

	// subscribe to all owner collision events
	// This will fire for all primitive components on the tank
	MyOwner->OnActorHit.AddUniqueDynamic(this, &ThisClass::OnHit);
}

void UTankCollisionDetectionComponent::OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsRelevantCollision(Hit))
	{
		OnRelevantCollision.Broadcast(Hit, NormalImpulse);
	}
}

bool UTankCollisionDetectionComponent::IsRelevantCollision(const FHitResult& Hit) const
{
	const auto OtherComponent = Hit.GetComponent();
	const auto Actor = Hit.GetActor();

	if (OtherComponent && ObjectTypes.Contains(OtherComponent->GetCollisionObjectType()))
	{
		return true;
	}

	if (OtherComponent && MatchesByName(ComponentSubstrings, OtherComponent->GetName()))
	{
		return true;
	}

	if (Actor && MatchesByName(ActorSubstrings, Actor->GetName()))
	{
		return true;
	}

	return false;
}

namespace
{
	inline bool MatchesByName(const TArray<FString>& Array, const FString& InputName)
	{
		return Array.ContainsByPredicate([&InputName](const auto& NameSubstring)
		{
			return InputName.Contains(NameSubstring);
		});
	}
}

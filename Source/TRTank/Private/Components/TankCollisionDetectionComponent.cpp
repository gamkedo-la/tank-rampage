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

	// subscribe to all owner collision events
	auto MyOwner = GetOwner();
	check(MyOwner);

	//// Need the component on other end of hit so need to subscribe to all component-level events
	//TArray<UPrimitiveComponent*> Comps;
	//MyOwner->GetComponents<UPrimitiveComponent>(Comps);
	// Subscribe to OnComponentHit for each and pass the context to OnRelevantCollision
	// 	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	MyOwner->OnActorHit.AddUniqueDynamic(this, &ThisClass::OnHit);
}

void UTankCollisionDetectionComponent::OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	//const auto OtherComponent = Hit.GetComponent();

	//if (auto Pawn = Cast<APawn>(GetOwner()); OtherComponent && Pawn && Pawn->IsPlayerControlled())
	//{
	//	UE_LOG(LogTRTank, Warning, TEXT("%s: NormalImpulse=%s; Hit Comp=%s;Actor=%s; ObjectType=%s"), *GetName(),
	//		*NormalImpulse.ToCompactString(),
	//		*OtherComponent->GetName(), *LoggingUtils::GetName(OtherActor),
	//		*LoggingUtils::GetName(OtherComponent->GetCollisionObjectType()));
	//}

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

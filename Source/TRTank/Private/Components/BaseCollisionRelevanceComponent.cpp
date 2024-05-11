// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BaseCollisionRelevanceComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRTankLogging.h"

#include "VisualLogger/VisualLogger.h"

namespace
{
	bool MatchesByName(const TArray<FString>& Array, const FString& InputName);
}

// Sets default values for this component's properties
UBaseCollisionRelevanceComponent::UBaseCollisionRelevanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBaseCollisionRelevanceComponent::BeginPlay()
{
	Super::BeginPlay();

	bMatchTrueCondition = !bNotifyAllButSpecified;

	RegisterCollisions();
}

void UBaseCollisionRelevanceComponent::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	const bool bIsRelevant = IsRelevantCollision(Hit);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: OnActorHit - bIsRelevant=%s; OtherActor=%s; NormalImpulse=%s; Hit=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), LoggingUtils::GetBoolString(bIsRelevant),
		*LoggingUtils::GetName(OtherActor), *NormalImpulse.ToCompactString(), *Hit.ToString());

	if (bIsRelevant)
	{
		OnNotifyRelevantCollision(nullptr, Hit, NormalImpulse);
	}
}

void UBaseCollisionRelevanceComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	const bool bIsRelevant = IsRelevantCollision(Hit);

	UE_VLOG_UELOG(GetOwner(), LogTRTank, VeryVerbose, TEXT("%s-%s: OnComponentHit - bIsRelevant=%s; HitComponent=%s; OtherActor=%s; OtherComponent=%s; NormalImpulse=%s; Hit=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), LoggingUtils::GetBoolString(bIsRelevant),
		*LoggingUtils::GetName(HitComponent), *LoggingUtils::GetName(OtherActor), *LoggingUtils::GetName(OtherComponent),
		*NormalImpulse.ToCompactString(), *Hit.ToString());

	if (bIsRelevant)
	{
		OnNotifyRelevantCollision(HitComponent, Hit, NormalImpulse);
	}
}

void UBaseCollisionRelevanceComponent::RegisterOwner()
{
	auto MyOwner = GetOwner();
	check(MyOwner);

	// subscribe to all owner collision events
	// This will fire for all primitive components on the actor
	MyOwner->OnActorHit.AddUniqueDynamic(this, &ThisClass::OnActorHit);
}

void UBaseCollisionRelevanceComponent::RegisterComponent(UPrimitiveComponent* Component)
{
	if (!ensure(Component))
	{
		return;
	}

	Component->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnComponentHit);
}

bool UBaseCollisionRelevanceComponent::IsRelevantCollision(const FHitResult& Hit) const
{
	const auto OtherComponent = Hit.GetComponent();
	const auto Actor = Hit.GetActor();

	if (OtherComponent && ObjectTypes.Contains(OtherComponent->GetCollisionObjectType()))
	{
		return bMatchTrueCondition;
	}

	if (OtherComponent && MatchesByName(ComponentSubstrings, OtherComponent->GetName()))
	{
		return bMatchTrueCondition;
	}

	if (Actor && MatchesByName(ActorSubstrings, Actor->GetName()))
	{
		return bMatchTrueCondition;
	}

	return !bMatchTrueCondition;
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/DynamicChildActorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DynamicChildActorComponent)

void UDynamicChildActorComponent::SpawnActor()
{
	if (ShouldSpawnActor())
	{
		CreateChildActor();
	}
}

void UDynamicChildActorComponent::DestroyActor()
{
	DestroyChildActor();
}

void UDynamicChildActorComponent::CreateChildActor(TFunction<void(AActor*)> CustomizerFunc)
{
	if (ShouldCreateActor())
	{
		Super::CreateChildActor(CustomizerFunc);
	}
}

void UDynamicChildActorComponent::OnRegister()
{
	Super::OnRegister();

	// destroy editor only created objects since these get persisted into the map
	if (auto World = GetWorld(); bSpawnInEditor && !bSpawnOnRegister && GetChildActor() && World->IsGameWorld() && GetChildActor()->IsNetStartupActor())
	{
		DestroyActor();
	}
}

bool UDynamicChildActorComponent::ShouldCreateActor() const
{
	if (GetChildActor() && GetChildActorClass())
	{
		// Child Actor already exists - need to execute default behavior
		return true;
	}
	auto World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Handle editor PIE world case
	if(!World->IsGameWorld())
	{
		return bSpawnInEditor;
	}
	else if(bSpawnOnRegister)
	{
		return true;
	}

	// only spawn if begun play
	// Actor components begin play before actors so check if we have begun play for the case to not spawn on register
	return HasBegunPlay();
}

bool UDynamicChildActorComponent::ShouldSpawnActor() const
{
	// only spawn if not already created and we have a valid class
	return !GetChildActor() && GetChildActorClass();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SpawnPoint.h"

#include "TRCoreLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Kismet/GameplayStatics.h"

USpawnPoint::USpawnPoint()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (!ensure(ActorToSpawn))
	{
		return;
	}

	auto World = GetWorld();
	check(World);

	const auto& SpawnTransform = GetComponentTransform();

	auto Actor = World->SpawnActorDeferred<AActor>(ActorToSpawn, SpawnTransform);

	if (!Actor)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRCore, Warning, TEXT("%s-%s: Unable to spawn actor %s with transform %s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *GetComponentTransform().ToHumanReadableString());
		return;
	}

	Actor->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);

	UGameplayStatics::FinishSpawningActor(Actor, SpawnTransform);
}

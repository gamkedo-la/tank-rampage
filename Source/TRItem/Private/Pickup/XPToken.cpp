// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/XPToken.h"

#include "XPSubsystem.h"

#include "Logging/LoggingUtils.h"
#include "Utils/CollisionUtils.h"

#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Components/SphereComponent.h" 
#include "Components/StaticMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(XPToken)

AXPToken::AXPToken()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CollisionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionVolume"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	CollisionVolume->SetCollisionProfileName(TR::CollisionProfile::DynamicOverlapOnlyTank);
	CollisionVolume->SetGenerateOverlapEvents(true);

	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AXPToken::BeginPlay()
{
	Super::BeginPlay();

	RegisterOverlapEvent(CollisionVolume);

	UE_VLOG_LOCATION(this, LogTRItem, Log, GetActorLocation(), CollisionVolume->GetLocalBounds().SphereRadius, FColor::Blue, TEXT("XP Token"));
}

void AXPToken::OnOverlap(APawn* PlayerPawn)
{
	auto World = GetWorld();
	if (!ensureMsgf(World, TEXT("World is NULL")))
	{
		return;
	}

	auto XPSubsystem = World->GetSubsystem<UXPSubsystem>();
	check(XPSubsystem);

	UE_VLOG_LOCATION(this, LogTRItem, Log, GetActorLocation(), CollisionVolume->GetLocalBounds().SphereRadius, FColor::Emerald, TEXT("XP Token Collected"));

	XPSubsystem->OnXPTokenOverlap.Broadcast(this, PlayerPawn);

	MarkForDestroy();
}

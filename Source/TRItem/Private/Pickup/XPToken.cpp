// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/XPToken.h"

#include "XPSubsystem.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Components/SphereComponent.h" 
#include "Components/StaticMeshComponent.h"

AXPToken::AXPToken()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CollisionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionVolume"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	CollisionVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CollisionVolume->SetGenerateOverlapEvents(true);

	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AXPToken::BeginPlay()
{
	Super::BeginPlay();

	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);

	UE_VLOG_LOCATION(this, LogTRItem, Log, GetActorLocation(), CollisionVolume->GetLocalBounds().SphereRadius, FColor::Blue, TEXT("XP Token"));
}

void AXPToken::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	auto World = GetWorld();
	if (!ensureMsgf(World, TEXT("World is NULL")))
	{
		return;
	}

	if (auto Controller = Pawn->GetController(); Controller && Controller->IsPlayerController())
	{
		auto XPSubsystem = World->GetSubsystem<UXPSubsystem>();
		check(XPSubsystem);

		UE_VLOG_LOCATION(this, LogTRItem, Log, GetActorLocation(), CollisionVolume->GetLocalBounds().SphereRadius, FColor::Emerald, TEXT("XP Token Collected"));

		XPSubsystem->OnXPTokenOverlap.Broadcast(this, Pawn);

		ApplyEffectToTarget(Pawn, InstantGameplayEffectClass);

		Destroy();
	}
}

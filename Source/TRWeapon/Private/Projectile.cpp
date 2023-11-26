// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRWeaponLogging.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->bAutoRegister = true;
	ProjectileMovementComponent->bAutoActivate = false;
	// This is needed for the projectile to actually move
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectile::Launch(float Speed)
{
	UE_VLOG_UELOG(this, LogTRWeapon, Log, TEXT("%s: Launch at %f cm/s"), *GetName(), Speed);

	ProjectileMovementComponent->SetVelocityInLocalSpace(FVector::ForwardVector * Speed);
	ProjectileMovementComponent->Activate();
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

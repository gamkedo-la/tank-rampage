// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"

#include "Logging/LoggingUtils.h"
#include "TRWeaponLogging.h"
#include "VisualLogger/VisualLogger.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->bAutoRegister = true;
	ProjectileMovementComponent->bAutoActivate = false;
	// This is needed for the projectile to actually move
	ProjectileMovementComponent->bShouldBounce = true;

	ExplosionForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("ExplosionForce"));
	ExplosionForce->bAutoActivate = false;
	ExplosionForce->PrimaryComponentTick.bStartWithTickEnabled = false;
	ExplosionForce->SetupAttachment(RootComponent);
}

void AProjectile::Launch(float Speed)
{
	UE_VLOG_UELOG(this, LogTRWeapon, Log, TEXT("%s: Launch at %f cm/s"), *GetName(), Speed);

	ProjectileMovementComponent->SetVelocityInLocalSpace(FVector::ForwardVector * Speed);
	ProjectileMovementComponent->Activate();

	SetLifeSpan(MaxLifetime);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	
	InitDebugDraw();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DestroyDebugDraw();
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	ExplosionForce->FireImpulse();

	UE_VLOG_LOCATION(this, LogTRWeapon, Display, GetActorLocation(), ExplosionForce->Radius, FColor::Red, TEXT("Explosion"));

	Destroy();
}

#if ENABLE_VISUAL_LOG

void AProjectile::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	if (!ProjectileMesh)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto CurrentTimeSeconds = World->GetTimeSeconds();

	// Get reference to the current category
	const int32 CatIndex = Snapshot->Status.AddZeroed();
	FVisualLogStatusCategory& Category = Snapshot->Status[CatIndex];
	Category.Category = FString::Printf(TEXT("Projectile (%s)"), *GetName());

	FVector Min, Max;
	ProjectileMesh->GetLocalBounds(Min, Max);
	FBox Bounds{ Min, Max };

	Snapshot->AddElement(Bounds, GetActorTransform().ToMatrixNoScale(),
		LogTRWeapon.GetCategoryName(), ELogVerbosity::Log, FColor::Red,
		*GetName());
}

void AProjectile::InitDebugDraw()
{
	// Ensure that state logged regularly so we see the updates in the visual logger
	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateLambda([this]()
		{
			UE_VLOG(this, LogTRWeapon, Log, TEXT("Get Projectile State"));
		});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);
}


void AProjectile::DestroyDebugDraw()
{
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
}

#else
void AProjectile::InitDebugDraw() {}
void AProjectile::DestroyDebugDraw() {}


#endif
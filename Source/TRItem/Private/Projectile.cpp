// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/StaticMeshComponent.h"
#include "FiredWeaponMovementComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Engine/DamageEvents.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	
	ProjectileMovementComponent = CreateDefaultSubobject<UFiredWeaponMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovementComponent->bAutoRegister = true;
	ProjectileMovementComponent->bAutoActivate = false;
	// Doesn't need to bounce as we destroy on hit
	ProjectileMovementComponent->bShouldBounce = false;

	ExplosionForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("ExplosionForce"));
	ExplosionForce->bAutoActivate = false;
	ExplosionForce->PrimaryComponentTick.bStartWithTickEnabled = false;
	ExplosionForce->SetupAttachment(RootComponent);
}

void AProjectile::Launch(float Speed)
{
	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Launch at %f cm/s"), *GetName(), Speed);

	ProjectileMovementComponent->SetVelocityInLocalSpace(FVector::ForwardVector * Speed);
	ProjectileMovementComponent->Activate();

	InitialDirection = GetActorRotation().Vector();

	SetLifeSpan(MaxLifetime);

	PlayFiringVfx();
}

void AProjectile::Initialize(USceneComponent& IncidentComponent, const FName& IncidentSocketName)
{
	AttachComponent = &IncidentComponent;
	AttachSocketName = IncidentSocketName;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ensure(ProjectileMesh))
	{
		ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
	
	InitDebugDraw();
}

void AProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DestroyDebugDraw();
}

void AProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(ProjectileMovementComponent);

	ProjectileMovementComponent->SetCanDamageOwner(bCanDamageInstigator);

	// Avoid self-hit unless configured to allow
	if (ensure(ProjectileMesh) && !bCanDamageInstigator)
	{
		ProjectileMesh->IgnoreActorWhenMoving(GetOwner(), true);
	}
}

void AProjectile::PlayFiringVfx()
{
	if (!FiringVfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: FiringVfx is not set"), *GetName());
		return;
	}

	const auto& Direction = GetActorRotation().Vector(); // Muzzle socket rotation on tank barrel

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: FiringVfx: %s playing at %s"), *GetName(), *FiringVfx.GetName(), *GetActorLocation().ToCompactString());

	check(AttachComponent);

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(FiringVfx, AttachComponent, AttachSocketName, FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::Type::KeepRelativeOffset, true);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: FiringVfx: %s playing NiagaraComponent=%s"), *GetName(), *FiringVfx.GetName(), *LoggingUtils::GetName(NiagaraComp));

	if (!NiagaraComp)
	{
		return;
	}

	NiagaraComp->SetVectorParameter(DirectionParameter, Direction);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	bool bDestroy = true;

	if (OtherActor)
	{
		// TODO: Should switch between FRadialDamageEvent and FPointDamageEvent based on type of weapon
		FPointDamageEvent DamageEvent;
		// TODO: Make configurable
		DamageEvent.Damage = 100;
		DamageEvent.HitInfo = Hit;
		DamageEvent.ShotDirection = InitialDirection;

		auto Pawn = GetInstigator();

		// Avoid damaging self unless configured to do so
		if (Pawn != OtherActor || CanDamageInstigator())
		{
			OtherActor->TakeDamage(DamageEvent.Damage, DamageEvent, Pawn ? Pawn->GetController() : nullptr, this);
		}
		else
		{
			bDestroy = false;
		}
	}

	if (bDestroy)
	{
		UE_VLOG_LOCATION(this, LogTRItem, Display, GetActorLocation(), ExplosionForce->Radius, FColor::Red, TEXT("Explosion"));
		ExplosionForce->FireImpulse();
		Destroy();
	}
	else if(OtherActor)
	{	
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Ignoring self hit with instigator=%s"), *GetName(), *OtherActor->GetName());
	}
}

#pragma region Visual Logger

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

	Snapshot->AddElement(Bounds, GetActorTransform().ToMatrixWithScale(),
		LogTRItem.GetCategoryName(), ELogVerbosity::Log, FColor::Red,
		*GetName());
}

void AProjectile::InitDebugDraw()
{
	// Ensure that state logged regularly so we see the updates in the visual logger
	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateLambda([this]()
		{
			UE_VLOG(this, LogTRItem, Log, TEXT("Get Projectile State"));
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

#pragma endregion Visual Logger
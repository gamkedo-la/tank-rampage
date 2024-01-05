// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/StaticMeshComponent.h"
#include "FiredWeaponMovementComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Engine/DamageEvents.h"
#include "Item/WeaponConfig.h"

#include "TRTags.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include <limits>

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

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

	SetLifeSpan(MaxLifetime);

	ProjectileMovementComponent->MaxSpeed = Speed * ProjectileHomingParams.MaxSpeedMultiplier;
	ProjectileMovementComponent->HomingAccelerationMagnitude = ProjectileHomingParams.HomingAcceleration;

	ProjectileMovementComponent->SetVelocityInLocalSpace(FVector::ForwardVector * Speed);
	ProjectileMovementComponent->Activate();

	InitialDirection = GetActorRotation().Vector();

	PlayFiringEffects();
}

void AProjectile::Initialize(USceneComponent& IncidentComponent, const FName& IncidentSocketName, const FProjectileDamageParams& InProjectileDamageParams, 
	const std::optional<FProjectileHomingParams>& InOptHomingParams)
{
	AttachComponent = &IncidentComponent;
	AttachSocketName = IncidentSocketName;
	ProjectileDamageParams = InProjectileDamageParams;

	if (InOptHomingParams)
	{
		InitHomingInfo(*InOptHomingParams);
	}
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

	GetWorldTimerManager().ClearTimer(HomingTargetTimerHandle);

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

void AProjectile::PlayFiringEffects()
{
	PlayFiringVfx();
	PlayFiringSfx();
}

void AProjectile::PlayFiringVfx()
{
	if (!FiringVfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: FiringVfx is not set"), *GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: FiringVfx: %s playing at %s"), *GetName(), *FiringVfx.GetName(), *GetActorLocation().ToCompactString());

	check(AttachComponent);

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(FiringVfx, AttachComponent, AttachSocketName, FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::Type::KeepRelativeOffset, true);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: FiringVfx: %s playing NiagaraComponent=%s"), *GetName(), *FiringVfx.GetName(), *LoggingUtils::GetName(NiagaraComp));

	if (!NiagaraComp)
	{
		return;
	}

	SetNiagaraFireEffectParameters(NiagaraComp);
}

void AProjectile::SetNiagaraFireEffectParameters_Implementation(UNiagaraComponent* NiagaraComponent)
{
	check(NiagaraComponent);

	if (!DirectionParameter.IsNone())
	{
		const auto& Direction = GetActorRotation().Vector(); // Muzzle socket rotation on tank barrel
		NiagaraComponent->SetVectorParameter(DirectionParameter, Direction);
	}
}

void AProjectile::PlayFiringSfx()
{
	if (!FiringSfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: PlayFiringSfx is not set"), *GetName());
		return;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	// We want to limit the number of times this plays when firing multiple shells so pass in the owner of the projectile
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(GetOwner(), FiringSfx, GetActorLocation(), GetActorRotation());

	if (!SpawnedAudioComponent)
	{
		UE_VLOG_UELOG(this, LogTRItem, Error,
			TEXT("%s-%s: PlayFiringSfx - Unable to spawn audio component for sfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *FiringSfx->GetName());
		return;
	}

	SpawnedAudioComponent->bAutoDestroy = true;
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	bool bDestroy = true;

	if (OtherActor)
	{
		auto Pawn = GetInstigator();

		// Avoid damaging self unless configured to do so
		if (Pawn != OtherActor || CanDamageInstigator())
		{
			ApplyDamageTo(OtherActor, Hit, Pawn);
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

void AProjectile::ApplyDamageTo(AActor* OtherActor, const FHitResult& Hit, APawn* InstigatingPawn)
{
	check(OtherActor);

	AController* const InstigatorController = InstigatingPawn ? InstigatingPawn->GetController() : nullptr;

	if (ProjectileDamageParams.WeaponDamageType == EWeaponDamageType::Radial)
	{
		TArray<AActor*> IgnoreActors;
		if (InstigatingPawn && !CanDamageInstigator())
		{
			IgnoreActors.Add(InstigatingPawn);
		}

		const auto ActualDamage = UGameplayStatics::ApplyRadialDamageWithFalloff(
			this, ProjectileDamageParams.MaxDamageAmount, ProjectileDamageParams.MinDamageAmount,
			Hit.ImpactPoint, ProjectileDamageParams.DamageInnerRadius, ProjectileDamageParams.DamageOuterRadius,
			ProjectileDamageParams.DamageFalloff, nullptr, IgnoreActors, InstigatingPawn, InstigatorController);

		UE_VLOG_LOCATION(this, LogTRItem, Log, Hit.ImpactPoint, ProjectileDamageParams.DamageInnerRadius, FColor::Red, TEXT("%s (%s -> %s): %f -> %f"),
			*GetName(), *LoggingUtils::GetName(InstigatingPawn), *LoggingUtils::GetName(OtherActor), ProjectileDamageParams.MaxDamageAmount, ActualDamage);

		UE_VLOG_LOCATION(this, LogTRItem, Log, Hit.ImpactPoint, ProjectileDamageParams.DamageOuterRadius, FColor::Orange, TEXT("%s (%s -> %s): %f -> %f"),
			*GetName(), *LoggingUtils::GetName(InstigatingPawn), *LoggingUtils::GetName(OtherActor), ProjectileDamageParams.MinDamageAmount, ActualDamage);
	}
	else
	{
		const auto ActualDamage = UGameplayStatics::ApplyPointDamage(OtherActor, ProjectileDamageParams.MaxDamageAmount, InitialDirection, Hit, InstigatorController, InstigatingPawn, nullptr);

		UE_VLOG_LOCATION(this, LogTRItem, Log, Hit.ImpactPoint, 50.0f, FColor::Red, TEXT("%s (%s): %f"),
			*GetName(), *LoggingUtils::GetName(InstigatingPawn), ActualDamage);
	}
}

#pragma region Homing

void AProjectile::InitHomingInfo(const FProjectileHomingParams& InProjectileHomingParams)
{
	ProjectileHomingParams = InProjectileHomingParams;

	GetWorldTimerManager().SetTimer(HomingTargetTimerHandle, this, &AProjectile::RefreshHomingTarget,
		ProjectileHomingParams.HomingTargetRefreshInterval, true, ProjectileHomingParams.HomingTargetRefreshInterval);
}

void AProjectile::RefreshHomingTarget()
{
	std::pair<AActor*, float> BestTarget{ nullptr, std::numeric_limits<float>::max() };
	int32 ViableCount{};

	const auto& CurrentLocation = GetActorLocation();

	for (auto PotentialTarget : ProjectileHomingParams.Targets)
	{
		// TODO: Use gameplay tags
		if (!IsValid(PotentialTarget) || PotentialTarget->ActorHasTag(TR::Tags::Dead))
		{
			continue;
		}

		++ViableCount;
		const float DistSq = FVector::DistSquared(CurrentLocation, PotentialTarget->GetActorLocation());
		if (DistSq < BestTarget.second)
		{
			BestTarget = { PotentialTarget, DistSq };
		}
	}

	ProjectileMovementComponent->HomingTargetComponent = GetHomingSceneComponent(BestTarget.first);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: RefreshHomingTarget: Selected %s out of %d viable"), *GetName(), *LoggingUtils::GetName(BestTarget.first), ViableCount);
}

USceneComponent* AProjectile::GetHomingSceneComponent(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->GetRootComponent();
}

#pragma endregion Homing

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

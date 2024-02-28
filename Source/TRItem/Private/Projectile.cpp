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

	ProjectileMesh->SetMobility(EComponentMobility::Movable);
	ProjectileMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	// Pawns like the tank need to overlap so they are not launched unrealistically in response to the collision
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
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

	check(ExplosionForce);

	if (ProjectileDamageParams.WeaponDamageType == EWeaponDamageType::Radial)
	{
		ExplosionForce->Falloff = ERadialImpulseFalloff::RIF_Linear;
		ExplosionForce->Radius = ProjectileDamageParams.DamageOuterRadius;
	}

	ExplosionForce->ImpulseStrength *= ProjectileDamageParams.ImpactImpulseAmountMultiplier;
	HitVfxScaleParameterValue *= ProjectileDamageParams.ImpactImpulseAmountMultiplier;

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
		ProjectileMesh->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnOverlapBegin);
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
	PlaySfxAtActorLocation(FiringSfx);
}

void AProjectile::PlayFiringVfx()
{
	if (!FiringVfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: PlayFiringVfx - FiringVfx is not set"), *GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: PlayFiringVfx: %s playing at %s"), *GetName(), *FiringVfx.GetName(), *GetActorLocation().ToCompactString());

	check(AttachComponent);

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(FiringVfx, AttachComponent, AttachSocketName, FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::Type::KeepRelativeOffset, true);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: PlayFiringVfx: %s playing NiagaraComponent=%s"), *GetName(), *FiringVfx.GetName(), *LoggingUtils::GetName(NiagaraComp));

	if (!NiagaraComp)
	{
		return;
	}

	SetNiagaraFireEffectParameters(NiagaraComp);
}

void AProjectile::PlayHitVfx()
{
	if (!HitVfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Warning, TEXT("%s: PlayHitVfx - HitVfx is not set"), *GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: PlayHitVfx: %s playing at %s"), *GetName(), *HitVfx.GetName(), *GetActorLocation().ToCompactString());

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitVfx, GetActorLocation());

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: PlayHitVfx: %s playing NiagaraComponent=%s"), *GetName(), *HitVfx.GetName(), *LoggingUtils::GetName(NiagaraComp));

	if (!NiagaraComp)
	{
		return;
	}

	SetNiagaraHitEffectParameters(NiagaraComp);
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

void AProjectile::SetNiagaraHitEffectParameters_Implementation(UNiagaraComponent* NiagaraComponent)
{
	check(NiagaraComponent);

	if (!HitVfxScaleParameterName.IsNone())
	{
		NiagaraComponent->SetFloatParameter(HitVfxScaleParameterName, HitVfxScaleParameterValue);
	}
}

void AProjectile::PlaySfxAtActorLocation(USoundBase* Sound) const
{
	if (!ensure(Sound))
	{
		return;
	}

	// The owner of the audio component is derived from the world context object and this will control the sound concurrency
	// We want to limit the number of times this plays when firing multiple shells so pass in the owner of the projectile
	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAtLocation(GetOwner(), Sound, GetActorLocation(), GetActorRotation());

	if (!SpawnedAudioComponent)
	{
		// This is not an error condition as the component may not spawn if the sound is not audible, for example it attenuates below a threshold based on distance
		UE_VLOG_UELOG(this, LogTRItem, Log,
			TEXT("%s-%s: PlaySfxAtActorLocation - Unable to spawn audio component for sfx=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());
		return;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log,
		TEXT("%s-%s: PlaySfxAtActorLocation - Playing sfx=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	OnCollision(HitComponent, OtherActor, OtherComponent, Hit);
}

void AProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OnCollision(OverlappedComponent, OtherActor, OtherComponent, SweepResult);
}

void AProjectile::OnCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, const FHitResult& Hit)
{
	bool bDestroy = true;

	if (OtherActor)
	{
		bDestroy = ApplyDamageTo(OtherActor, Hit, GetInstigator());
	}

	if (bDestroy)
	{
		UE_VLOG_LOCATION(this, LogTRItem, Display, GetActorLocation(), ExplosionForce->Radius, FColor::Red, TEXT("Explosion"));
		ExplosionForce->FireImpulse();

		PlaySfxAtActorLocation(ExplosionSfx);
		PlayHitSfx(OtherActor);
		PlayHitVfx();

		MarkForDestroy();
	}
	else if (OtherActor)
	{
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Ignoring self hit with instigator=%s"), *GetName(), *OtherActor->GetName());
	}
}

void AProjectile::MarkForDestroy()
{
	// Allow frame to complete

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		Destroy();
	}));
}

void AProjectile::PlayHitSfx(AActor* HitActor) const
{
	if (!HitActor)
	{
		return;
	}

	if (HitActor->ActorHasTag(TR::Tags::Tank))
	{
		PlaySfxAtActorLocation(TankHitSfx);
	}
}

bool AProjectile::ApplyDamageTo(AActor* OtherActor, const FHitResult& Hit, APawn* InstigatingPawn)
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

		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this, ProjectileDamageParams.MaxDamageAmount, ProjectileDamageParams.MinDamageAmount,
			Hit.ImpactPoint, ProjectileDamageParams.DamageInnerRadius, ProjectileDamageParams.DamageOuterRadius,
			ProjectileDamageParams.DamageFalloff, nullptr, IgnoreActors, this, InstigatorController);

		UE_VLOG_LOCATION(this, LogTRItem, Log, Hit.ImpactPoint, ProjectileDamageParams.DamageInnerRadius, FColor::Red, TEXT(""),
			*GetName(), *LoggingUtils::GetName(InstigatingPawn), *LoggingUtils::GetName(OtherActor));

		UE_VLOG_LOCATION(this, LogTRItem, Log, Hit.ImpactPoint, ProjectileDamageParams.DamageOuterRadius, FColor::Orange, TEXT("%s (%s -> %s): (%f,%f)"),
			*GetName(), *LoggingUtils::GetName(InstigatingPawn), *LoggingUtils::GetName(OtherActor), ProjectileDamageParams.MinDamageAmount, ProjectileDamageParams.MaxDamageAmount);
	}
	else
	{
		// Avoid damaging self unless configured to do so
		if (InstigatingPawn != OtherActor || CanDamageInstigator())
		{
			const auto ActualDamage = UGameplayStatics::ApplyPointDamage(OtherActor, ProjectileDamageParams.MaxDamageAmount, InitialDirection, Hit, InstigatorController, this, nullptr);

			UE_VLOG_LOCATION(this, LogTRItem, Log, Hit.ImpactPoint, 50.0f, FColor::Red, TEXT("%s (%s): %f"),
				*GetName(), *LoggingUtils::GetName(InstigatingPawn), ActualDamage);
		}
		else
		{
			return false;
		}
	}

	return true;
}


#pragma region Homing

void AProjectile::InitHomingInfo(const FProjectileHomingParams& InProjectileHomingParams)
{
	ProjectileHomingParams = InProjectileHomingParams;

	GetWorldTimerManager().SetTimer(HomingTargetTimerHandle, this, &AProjectile::RefreshHomingTarget,
		ProjectileHomingParams.HomingTargetRefreshInterval, true);

	// Run also immediately
	RefreshHomingTarget();
}

void AProjectile::RefreshHomingTarget()
{
	std::pair<AActor*, float> BestTarget{ nullptr, std::numeric_limits<float>::max() };
	int32 ViableCount{};

	const auto& CurrentLocation = GetActorLocation();
	const auto& VelocityDirection = GetVelocity().GetSafeNormal();

	const FVector GroundLocation = GetGroundLocation();
	const FVector ToGroundDirection = (GroundLocation - CurrentLocation).GetSafeNormal();

	for (auto PotentialTarget : ProjectileHomingParams.Targets)
	{
		// TODO: Use gameplay tags
		if (!IsValid(PotentialTarget) || PotentialTarget->ActorHasTag(TR::Tags::Dead))
		{
			continue;
		}

		++ViableCount;

		// consider both distance and alignment
		const auto& TargetLocation = PotentialTarget->GetActorLocation();
		const auto ToTarget = TargetLocation - CurrentLocation;
		const auto Dist = FMath::Max(0.01, ToTarget.Size());
		const auto ToTargetDirection = ToTarget / Dist;

		if (FMath::Abs(CurrentLocation.Z - TargetLocation.Z) <= MaxZDifference && (
			TargetLocation.Z >= GroundLocation.Z || FMath::Abs(ToTargetDirection | ToGroundDirection) <= HomingGroundAngleCosineThreshold))
		{
			// ensure alignment always positive and > 0
			const auto Alignment = (ToTargetDirection | VelocityDirection) + 1.01;
			const auto Score = Dist / FMath::Cube(Alignment);

			if (Score < BestTarget.second)
			{
				BestTarget = { PotentialTarget, Score };
			}
		}
	}

	auto NewHomingTarget = BestTarget.first;
	auto PreviousHomingTarget = GetCurrentHomingTargetActor();

	ProjectileMovementComponent->HomingTargetComponent = GetHomingSceneComponent(NewHomingTarget);
	ProjectileMovementComponent->bIsHomingProjectile = true;

	// Not finding a target is a valid result and should be broadcast
	if (PreviousHomingTarget != NewHomingTarget)
	{
		OnHomingTargetSelected.Broadcast(this, NewHomingTarget);
	}

	if (NewHomingTarget)
	{
		UE_VLOG_LOCATION(this, LogTRItem, Log, NewHomingTarget->GetActorLocation(), 50.0f, FColor::Yellow, TEXT("%s: Homing Target (%s)"), *GetName(), *NewHomingTarget->GetActorLocation().ToCompactString());
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: RefreshHomingTarget: Selected %s out of %d viable"), *GetName(), *LoggingUtils::GetName(NewHomingTarget), ViableCount);

}

FVector AProjectile::GetGroundLocation() const
{
	const FVector& CurrentLocation = GetActorLocation();
	auto World = GetWorld();

	if (World)
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (World->LineTraceSingleByChannel(Hit, CurrentLocation, CurrentLocation - FVector(0, 0, 10000),
			ECollisionChannel::ECC_Visibility, Params))
		{
			return Hit.Location;
		}

	}

	return CurrentLocation - FVector(0, 0, 500);
}

void AProjectile::AddAvailableHomingTarget(AActor* Actor)
{
	if (!ProjectileMovementComponent->bIsHomingProjectile)
	{
		return;
	}

	ProjectileHomingParams.Targets.Add(Actor);
}

void AProjectile::RemoveAvailableHomingTarget(AActor* Actor)
{
	if (!ProjectileMovementComponent->bIsHomingProjectile)
	{
		return;
	}

	ProjectileHomingParams.Targets.Remove(Actor);
}

AActor* AProjectile::GetCurrentHomingTargetActor() const
{
	if (!ProjectileMovementComponent->bIsHomingProjectile)
	{
		return nullptr;
	}

	auto SceneComponent = ProjectileMovementComponent->HomingTargetComponent.Get();
	if (!SceneComponent)
	{
		return nullptr;
	}

	return SceneComponent->GetOwner();
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

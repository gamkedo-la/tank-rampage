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

#include "Utils/CollisionUtils.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include <limits>

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Projectile)

namespace
{
	constexpr ECollisionChannel HomingLOSTraceChannel = TR::CollisionChannel::MissileHomingTargetTraceType;
}

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));

	ProjectileMesh->SetMobility(EComponentMobility::Movable);
	ProjectileMesh->SetCollisionProfileName(TR::CollisionProfile::Projectile);
	
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

	FiringAudioComponent = PlaySfxAtActorLocation(GetFiringSound());
}

USoundBase* AProjectile::GetFiringSound() const
{
	if (IsHoming() && FiringSfxHoming)
	{
		return FiringSfxHoming;
	}

	if (IsPlayer(GetOwner()) && PlayerFiringSfx)
	{
		return PlayerFiringSfx;
	}

	return FiringSfx;
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

void AProjectile::StopFiringSfx()
{
	if (!IsValid(FiringAudioComponent))
	{
		return;
	}

	if (!FiringAudioComponent->IsPlaying())
	{
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: StopFiringSfx: %s is not playing - nothing to stop"), *GetName(), *FiringAudioComponent->GetName());
		return;
	}

	FiringAudioComponent->FadeOut(FiringStopFadeOutTime, 0.0f);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: StopFiringSfx: %s - fade out to 0 in %fs"), *GetName(), *FiringAudioComponent->GetName(), FiringStopFadeOutTime);
}

void AProjectile::PlayExplosionSfxIfSet()
{
	if (!ExplosionSfx)
	{
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: PlayExplosionSfxIfSet: No specific explosionSFX set"), *GetName());
		return;
	}

	PlaySfxAtActorLocation(ExplosionSfx);
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

UAudioComponent* AProjectile::PlaySfxAtActorLocation(USoundBase* Sound) const
{
	if (!ensure(Sound))
	{
		return nullptr;
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
		return nullptr;
	}

	UE_VLOG_UELOG(this, LogTRItem, Log,
		TEXT("%s-%s: PlaySfxAtActorLocation - Playing sfx=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Sound->GetName());

	SpawnedAudioComponent->bAutoDestroy = true;
	// Comment is misleading. bReverb == true does NOT mean to exclude it
	SpawnedAudioComponent->bReverb = true;

	return SpawnedAudioComponent;
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
		UE_VLOG_UELOG(this, LogTRItem, Verbose, TEXT("%s: Hit %s on %s"), *GetName(), *LoggingUtils::GetName(OtherComponent), *LoggingUtils::GetName(OtherActor));

		UE_VLOG_LOCATION(this, LogTRItem, Display, GetActorLocation(), ExplosionForce->Radius, FColor::Red, TEXT("Explosion"));
		ExplosionForce->FireImpulse();

		StopFiringSfx();

		PlayExplosionSfxIfSet();
		PlayHitSfx(OtherActor, OtherComponent, Hit);

		PlayHitVfx();
		ApplyPostHitEffects(Hit, ProjectileDamageParams);

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

void AProjectile::PlayHitSfx(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit) const
{
	if (!HitActor)
	{
		return;
	}

	const auto Sound = GetHitSound(HitActor, HitComponent, Hit);

	if (!Sound)
	{
		UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: PlayHitSfx: No HitSFX set"), *GetName());

		return;
	}

	PlaySfxAtActorLocation(Sound);
}

USoundBase* AProjectile::GetHitSound(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit) const
{
	if (!HitActor)
	{
		return nullptr;
	}

	if (TankHitPlayerSfx && IsPlayer(HitActor))
	{
		return TankHitPlayerSfx;
	}
	else if (HitActor->ActorHasTag(TR::Tags::Tank))
	{
		return TankHitSfx;
	}

	const auto PhysicalMaterial = Hit.PhysMaterial.Get();

	if (PhysicalMaterial)
	{
		auto MatchedPhysicalMaterialSfx = PhysicalMaterialHitToSfx.Find(PhysicalMaterial);
		if (MatchedPhysicalMaterialSfx)
		{
			const auto HitSurfaceSound = *MatchedPhysicalMaterialSfx;
			UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: GetHitSound: Matched PhysicalMaterial=%s to HitSFX=%s"),
				*GetName(), *PhysicalMaterial->GetName(), *LoggingUtils::GetName(HitSurfaceSound));

			return HitSurfaceSound;
		}
	}

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: GetHitSound: Using No match to PhysicalMaterial=%s; using DefaultHitSfx=%s"),
			*GetName(), *LoggingUtils::GetName(PhysicalMaterial), *LoggingUtils::GetName(DefaultHitSfx));

	return DefaultHitSfx;
}

bool AProjectile::IsPlayer(AActor* Actor) const
{
	auto Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
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
			ProjectileDamageParams.DamageFalloff, nullptr, IgnoreActors, this, InstigatorController, TR::CollisionChannel::ExplosionDamageTraceType);

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

	ViabilityTraceBendFactor = FMath::Sin(FMath::DegreesToRadians(ViabilityTraceBendAngleDegrees) * 0.5f);

	// Run also immediately
	RefreshHomingTarget();
}

void AProjectile::RefreshHomingTarget()
{
	std::pair<AActor*, double> BestTarget{ nullptr, std::numeric_limits<double>::max() };
	int32 ViableCount{};

	const auto& CurrentLocation = GetActorLocation();
	const auto& VelocityDirection = GetVelocity().GetSafeNormal();

	const FVector GroundLocation = GetGroundLocation();
	const FVector ToGroundDirection = (GroundLocation - CurrentLocation).GetSafeNormal();

	const FVector& ProjectileForwardVector = GetActorForwardVector();

	for (auto PotentialTarget : ProjectileHomingParams.Targets)
	{
		// TODO: Use gameplay tags
		if (!IsValid(PotentialTarget) || PotentialTarget->ActorHasTag(TR::Tags::Dead))
		{
			continue;
		}

		// consider both distance and alignment
		const auto& TargetLocation = PotentialTarget->GetActorLocation();
		const auto ToTarget = TargetLocation - CurrentLocation;
		const auto Dist = FMath::Max(0.01, ToTarget.Size());

		if (Dist > MaxHomingDistance)
		{
			continue;
		}

		const auto ToTargetDirection = ToTarget / Dist;
		const auto ToTargetDot = ToTargetDirection | ProjectileForwardVector;

		if (ToTargetDot < HomingTargetCosineThreshold)
		{
			continue;
		}

		if (FMath::Abs(CurrentLocation.Z - TargetLocation.Z) <= MaxZDifference && (
			TargetLocation.Z >= GroundLocation.Z || FMath::Abs(ToTargetDirection | ToGroundDirection) <= HomingGroundAngleCosineThreshold))
		{
			++ViableCount;

			// ensure alignment always positive and > 0
			const auto Alignment = (ToTargetDirection | VelocityDirection) + 1.01;
			auto Score = Dist / FMath::Cube(Alignment);

			if (Score < BestTarget.second)
			{
				// Make sure truly viable by line of sight tests
				if (!HasLineOfSightToTarget(CurrentLocation, *PotentialTarget, Dist))
				{
					Score = FMath::Square(Score);
				}

				if (Score < BestTarget.second)
				{
					BestTarget = { PotentialTarget, Score };
				}
			}

			UE_VLOG_LOCATION(this, LogTRItem, Verbose, TargetLocation + FVector(0,0,100), 15.0f, FColor::Cyan, TEXT("MissileTargetScore: %.1f"), Score);
			UE_VLOG_UELOG(this, LogTRItem, Verbose, TEXT("%s: RefreshHomingTarget: Target=%s; Score=%.1f"), *GetName(), *LoggingUtils::GetName(PotentialTarget), Score);
		}
	}

	auto NewHomingTarget = BestTarget.first;
	auto PreviousHomingTarget = GetCurrentHomingTargetActor();

	ProjectileMovementComponent->HomingTargetComponent = GetHomingSceneComponent(NewHomingTarget);
	ProjectileMovementComponent->bIsHomingProjectile = ProjectileMovementComponent->HomingTargetComponent != nullptr;

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

bool AProjectile::HasLineOfSightToTarget(const FVector& StartLocation, const AActor& Target, float TargetDistance) const
{
	auto World = GetWorld();
	if (!World)
	{
		return false;
	}

	auto MyOwner = GetOwner();

	if (!IsValid(MyOwner))
	{
		return false;
	}

	const auto TargetBounds = TR::CollisionUtils::GetAABB(Target);
	FVector TargetCenter, TargetExtent;
	TargetBounds.GetCenterAndExtents(TargetCenter, TargetExtent);
	const auto TargetLocation = TargetCenter + FVector::ZAxisVector * TargetExtent.Z;

	const auto TraceZOffset = FMath::Min(ViabilityLineTraceMaxZOffset * TargetDistance / ViabilityLineTraceDistScaling, ViabilityLineTraceMaxZOffset);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyOwner);

	const auto StartLocationZOffset = StartLocation + FVector::ZAxisVector * TraceZOffset;

	bool bLOS = !World->LineTraceTestByChannel(StartLocationZOffset, TargetLocation, HomingLOSTraceChannel, Params);

	UE_VLOG_ARROW(this, LogTRItem, Verbose, StartLocationZOffset, TargetLocation, bLOS ? FColor::Green : FColor::Red, TEXT("MissileTrace(D): %s"), *Target.GetName());

	if(bLOS)
	{
		return true;
	}

	// Do a line trace offset from left and right of owner to see if we can bend around
	// Determine radius from lateral distance of angle bisector of central angle with the distance as the radius of the circle
	const auto LateralOffsetDistance = ViabilityTraceBendFactor * TargetDistance;
	const auto RightLateralOffset = MyOwner->GetActorRightVector() * LateralOffsetDistance;

	bLOS = !World->LineTraceTestByChannel(StartLocationZOffset, TargetLocation + RightLateralOffset, HomingLOSTraceChannel, Params);
	UE_VLOG_ARROW(this, LogTRItem, Verbose, StartLocationZOffset, TargetLocation + RightLateralOffset, bLOS ? FColor::Green : FColor::Red, TEXT("MissileTargetTrace(R): %s"), *Target.GetName());

	if (bLOS)
	{
		return true;
	}
	
	bLOS = !World->LineTraceTestByChannel(StartLocationZOffset, TargetLocation - RightLateralOffset, HomingLOSTraceChannel, Params);
	UE_VLOG_ARROW(this, LogTRItem, Verbose, StartLocationZOffset, TargetLocation - RightLateralOffset, bLOS ? FColor::Green : FColor::Red, TEXT("MissileTargetTrace(L): %s"), *Target.GetName());

	if (bLOS)
	{
		return true;
	}

	return false;
}

bool AProjectile::IsHoming() const
{
	return ProjectileMovementComponent->bIsHomingProjectile;
}


FVector AProjectile::GetGroundLocation() const
{
	const FVector& CurrentLocation = GetActorLocation();
	auto World = GetWorld();

	if (World)
	{
		FHitResult Hit;

		if (World->LineTraceSingleByObjectType(Hit, CurrentLocation, CurrentLocation - FVector(0, 0, 10000),
			TR::CollisionChannel::GroundObjectType))
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

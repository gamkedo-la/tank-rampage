// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Item/WeaponConfig.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include <optional>

#include "Projectile.generated.h"

class UStaticMeshComponent;
class UFiredWeaponMovementComponent;
class URadialForceComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UAudioComponent;
class UPhysicalMaterial;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHomingTargetSelected, AProjectile* /* Projectile*/, AActor* /*Target*/);

UCLASS()
class TRITEM_API AProjectile : public AActor, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	UFUNCTION(BlueprintCallable)
	virtual void Launch(float Speed);

	virtual void Initialize(USceneComponent& IncidentComponent, const FName& IncidentSocketName, const FProjectileDamageParams& InProjectileDamageParams,
		const std::optional<FProjectileHomingParams>& InOptHomingParams = std::nullopt);

	UFUNCTION(BlueprintPure)
	bool CanDamageInstigator() const;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	FOnHomingTargetSelected OnHomingTargetSelected{};

	void AddAvailableHomingTarget(AActor* Actor);
	void RemoveAvailableHomingTarget(AActor* Actor);
	void TargetDestroyed(AActor* Actor);

	AActor* GetCurrentHomingTargetActor() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent)
	void SetNiagaraFireEffectParameters(UNiagaraComponent* NiagaraComponent);

	UFUNCTION(BlueprintNativeEvent)
	void SetNiagaraHitEffectParameters(UNiagaraComponent* NiagaraComponent);

	virtual void ApplyPostHitEffects(const FHitResult& HitInfo, const FProjectileDamageParams& DamageParams) {}

private:
	void InitDebugDraw();
	void DestroyDebugDraw();

	void PlayFiringEffects();
	void PlayFiringVfx();
	void PlayHitVfx();
	void StopFiringSfx();
	void PlayExplosionSfxIfSet();

	UAudioComponent* PlaySfxAtActorLocation(USoundBase* Sound) const;

	void PlayHitSfx(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit) const;

	FVector GetGroundLocation() const;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void OnCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, const FHitResult& Hit);


	bool ApplyDamageTo(AActor* OtherActor, const FHitResult& Hit, APawn* InstigatingPawn);

	UFUNCTION()
	void RefreshHomingTarget();

	void InitHomingInfo(const FProjectileHomingParams& InProjectileHomingParams);
	static USceneComponent* GetHomingSceneComponent(AActor* Actor);

	void MarkForDestroy();

	bool HasLineOfSightToTarget(const FVector& StartLocation, const AActor& Target, float TargetDistance) const;

	bool IsHoming() const;

	USoundBase* GetFiringSound() const;
	USoundBase* GetHitSound(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit) const;
	bool IsPlayer(AActor* Actor) const;

	float NearbyTargetPenaltyScore(const AActor& Target) const;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFiredWeaponMovementComponent> ProjectileMovementComponent{};

private:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ProjectileMesh{};

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<URadialForceComponent> ExplosionForce{};

	UPROPERTY(Category = "Effects | Firing", EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> FiringVfx{};

	UPROPERTY(Category = "Effects | Firing", EditDefaultsOnly)
	FName DirectionParameter{};

	UPROPERTY(Category = "Audio | Firing", EditDefaultsOnly)
	TObjectPtr<USoundBase> FiringSfx{};

	UPROPERTY(Category = "Audio | Firing", EditDefaultsOnly)
	TObjectPtr<USoundBase> PlayerFiringSfx{};

	UPROPERTY(Category = "Audio | Firing", EditDefaultsOnly)
	TObjectPtr<USoundBase> FiringSfxHoming{};

	UPROPERTY(Category = "Audio | Firing", EditDefaultsOnly)
	float FiringStopFadeOutTime{ 0.2f };

	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TObjectPtr<USoundBase> ExplosionSfx{};

	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TObjectPtr<USoundBase> TankHitSfx{};

	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TObjectPtr<USoundBase> TankHitPlayerSfx{};

	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TMap<UPhysicalMaterial*, USoundBase*> PhysicalMaterialHitToSfx{};

	UPROPERTY(Category = "Audio | Hit", EditDefaultsOnly)
	TObjectPtr<USoundBase> DefaultHitSfx{};

	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> HitVfx{};

	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly)
	FName HitVfxScaleParameterName{};

	UPROPERTY(Category = "Effects | Hit", EditDefaultsOnly)
	float HitVfxScaleParameterValue{ 1.0f };

	UPROPERTY(EditDefaultsOnly)
	float MaxLifetime{ 10.0f };

	/*
	* If target is below the ground under the projectile, this is the maximum absolute value of the cosine of angle between projectile to ground and projectile to target. 
	*/
	UPROPERTY(Category = "Homing", EditDefaultsOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float HomingGroundAngleCosineThreshold{ 0.85f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly, meta = (ClampMin = "0"))
	float MaxHomingDistance{ 500 * 100.0f };

	/*
	* Minimum alignment of heading to target to consider as a viable candidate so we don't accidentally shoot ourselves or make targeting too powerful.
	*/
	UPROPERTY(Category = "Homing", EditDefaultsOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float HomingTargetCosineThreshold{ -0.3f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float MaxZDifference{ 5000.0f };

	/* Scaling factor of moving the Z offset of trace up based on distance to target*/
	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float ViabilityLineTraceDistScaling{ 100 * 100.0f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float ViabilityLineTraceMaxZOffset{ 1000.0f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float ViabilityTraceBendAngleDegrees{ 20.0f };

	// Angle bisector : sin (x / 2)
	float ViabilityTraceBendFactor{};

	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float AdjacentTargetPenaltyMultiplier{ 10000.0f };

	FProjectileDamageParams ProjectileDamageParams{};

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> AttachComponent{};

	FName AttachSocketName{};
	FVector InitialDirection{ EForceInit::ForceInitToZero };

#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif

	/*
	* Indicates whether the tank that fired the weapon can be damaged by it.
	*/
	UPROPERTY(Transient)
	FProjectileHomingParams ProjectileHomingParams{};

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> FiringAudioComponent{};

	FTimerHandle HomingTargetTimerHandle{};

	UPROPERTY(Category = "Damage", EditDefaultsOnly)
	bool bCanDamageInstigator{ false };
};

#pragma region Inline Definitions

inline bool AProjectile::CanDamageInstigator() const
{
	return bCanDamageInstigator;
}

#pragma endregion Inline Definitions

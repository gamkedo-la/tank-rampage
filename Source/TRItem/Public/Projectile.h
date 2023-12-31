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

	AActor* GetCurrentHomingTargetActor() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent)
	void SetNiagaraFireEffectParameters(UNiagaraComponent* NiagaraComponent);

private:
	void InitDebugDraw();
	void DestroyDebugDraw();

	void PlayFiringEffects();
	void PlayFiringVfx();
	void PlayFiringSfx();

	FVector GetGroundLocation() const;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	bool ApplyDamageTo(AActor* OtherActor, const FHitResult& Hit, APawn* InstigatingPawn);

	UFUNCTION()
	void RefreshHomingTarget();

	void InitHomingInfo(const FProjectileHomingParams& InProjectileHomingParams);
	static USceneComponent* GetHomingSceneComponent(AActor* Actor);

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

	UPROPERTY(EditDefaultsOnly)
	float MaxLifetime{ 10.0f };

	/*
	* If target is below the ground under the projectile, this is the maximum absolute value of the cosine of angle between projectile to ground and projectile to target. 
	*/
	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float HomingGroundAngleCosineThreshold{ 0.85f };

	UPROPERTY(Category = "Homing", EditDefaultsOnly)
	float MaxZDifference{ 5000.0f };

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

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "Projectile.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class URadialForceComponent;
class UNiagaraSystem;

UCLASS()
class TRITEM_API AProjectile : public AActor, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	UFUNCTION(BlueprintCallable)
	void Launch(float Speed);

	void Initialize(USceneComponent& IncidentComponent, const FName& IncidentSocketName);

	UFUNCTION(BlueprintPure)
	bool CanDamageInstigator() const;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void InitDebugDraw();
	void DestroyDebugDraw();
	void PlayFiringVfx();


private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ProjectileMesh{};

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent{};

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<URadialForceComponent> ExplosionForce{};

	UPROPERTY(Category = "Effects | Firing", EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> FiringVfx{};

	UPROPERTY(Category = "Effects | Firing", EditDefaultsOnly)
	FName DirectionParameter{};

	UPROPERTY(EditDefaultsOnly)
	float MaxLifetime{ 10.0f };

	/*
	* Indicates whether the tank that fired the weapon can be damaged by it.
	*/
	UPROPERTY(EditDefaultsOnly)
	bool bCanDamageInstigator{ false };

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> AttachComponent{};

	FName AttachSocketName{};

#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif
};

#pragma region Inline Definitions

inline bool AProjectile::CanDamageInstigator() const
{
	return bCanDamageInstigator;
}

#pragma endregion Inline Definitions

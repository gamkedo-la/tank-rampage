// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BaseCollisionRelevanceComponent.h"
#include "TankCrashComponent.generated.h"

class USoundBase;

/**
 * 
 */
UCLASS()
class UTankCrashComponent : public UBaseCollisionRelevanceComponent
{
	GENERATED_BODY()

protected:
	virtual void RegisterCollisions() override;
	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;

private:
	float GetAudioVolume(const FVector& NormalImpulse) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> TankCrashSfx{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinAudioVolume{ 0.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxAudioVolume{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio", Meta=( ClampMin= "1.0"))
	float VolumeEaseFactor{ 1.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinVolumeImpulse{ 1e5f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxVolumeImpulse{ 1e7f };
};

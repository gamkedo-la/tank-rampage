// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BaseCollisionRelevanceComponent.h"
#include "HitSfxComponent.generated.h"

class USoundBase;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRCORE_API UHitSfxComponent : public UBaseCollisionRelevanceComponent
{
	GENERATED_BODY()

public:
	UHitSfxComponent();

protected:
	virtual void RegisterCollisions() override;

	UFUNCTION(BlueprintImplementableEvent)
	bool BlueprintRegisterCollisions();

	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse) override;

	USoundBase* GetHitSfx() const { return HitSfx; }
private:
	float GetAudioVolume(const FVector& NormalImpulse) const;

private:

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> HitSfx{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinAudioVolume{ 0.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxAudioVolume{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio", Meta = (ClampMin = "1.0"))
	float VolumeEaseFactor{ 1.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinVolumeImpulse{ 1e5f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxVolumeImpulse{ 1e7f };

	/* Minimum amount of time to wait in between triggering of the sfx */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinPlayInterval{ 0.0f };

	/* Minimum game time elapsed before the sound will start playing */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinPlayTimeSeconds{ 0.0f };

	float LastPlayTimeSeconds{ -1.0f };
};

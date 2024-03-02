// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TankTrackComponent.generated.h"

/**
 * Tank track is used to set maximum driving force, and to apply forces to the tank.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UTankTrackComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UTankTrackComponent();

	UFUNCTION(BlueprintCallable, Category = Input)
	void SetThrottle(float InThrottle);

	UFUNCTION(BlueprintPure)
	bool IsGrounded() const;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

	void ApplySidewaysForce(float DeltaTime);
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);

private:

	void DriveTrack(float Throttle);

	float GetAdjustedMaxDrivingForce() const;

private:

	UPROPERTY(EditDefaultsOnly, Category = Throttle)
	float TrackMaxDrivingForce{ 4.0e5f };

	float CurrentThrottle{};
};

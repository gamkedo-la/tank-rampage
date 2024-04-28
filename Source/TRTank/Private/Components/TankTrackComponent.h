// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "Containers/TimedCircularBuffer.h"

#include "TankTrackComponent.generated.h"

class ASpringWheel;

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
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	float GetAdjustedMaxDrivingForce() const;

	void InitWheels();

	void DriveTrackNoSuspension(float Throttle);
	void DriveTrackWithSuspension( float Throttle);

	bool HasSuspension() const;

	TArray<ASpringWheel*> GetWheels() const;

	void CheckStuck();
	bool IsStuck() const;

private:

	UPROPERTY(EditDefaultsOnly, Category = Throttle)
	float TrackMaxDrivingForce{ 4.0e5f };

	UPROPERTY(EditDefaultsOnly, Category = Throttle)
	float GroundTraceExtent{ 20 };

	float CurrentThrottle{};

	UPROPERTY(Transient)
	TArray<ASpringWheel*> Wheels;

	using FVectorBuffer = TR::TTimedCircularBuffer <
		FVector,
		decltype([]() { return FVector{ ForceInitToZero }; }),
		decltype([](const FVector& V) { return FMath::Max3(FMath::Abs(V.X), FMath::Abs(V.Y), FMath::Abs(V.Z)); })
	> ;
	FVectorBuffer PositionBuffer;
	TR::TTimedCircularBuffer<float> ThrottleBuffer;

	UPROPERTY(EditDefaultsOnly, Category = Stuck)
	float ThrottleSampleTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = Stuck)
	float ThrottleBoostMultiplier{ 6.0f };

	UPROPERTY(EditDefaultsOnly, Category = Stuck)
	float ThrottleStuckDetectionThreshold{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = Stuck)
	float StuckDisplacementThreshold{ 100.0f };

	bool bStuckBoostActive{};
};

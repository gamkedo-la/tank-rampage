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

	UFUNCTION(BlueprintPure)
	bool IsAirborne() const;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

	void NotifyRelevantTankCollision(const FHitResult& Hit, const FVector& NormalImpulse);

	UFUNCTION(BlueprintPure)
	float GetThrottle() const;

protected:
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

	void ApplySidewaysForce(float DeltaTime);
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	float GetAdjustedMaxDrivingForce() const;

	void InitWheels();
	void InitTrackWheels();

	void DriveTrackNoSuspension(float Throttle);
	void DriveTrackWithSuspension( float Throttle);

	bool HasSuspension() const;

	TArray<ASpringWheel*> GetWheels() const;

	// Stuck Detection
	void InitStuckDetection();
	bool ShouldCheckForBeingStuck() const;
	void CalculateStuck();
	void SampleStuckBuffers();
	bool IsStuck() const;
	bool StuckBeyondResetThreshold() const;
	void ResetTankTransform();
	void ResetStuckBuffers();

	bool ShouldRecalculateGrounded() const;
	void CalculateGrounded();
	bool IsGroundedFallback() const;

	bool IsGroundedLocation(const FVector& WorldLocation, const FVector& WorldUpVector) const;

	void DriveTrackNoSuspension(float Throttle, const FName& ForceSocket, UPrimitiveComponent& PrimitiveComponent, float ForceMultiplier);

	void RecordThrottle(float Value);
	void ClearThrottle();

private:

	struct FTrackWheel
	{
		FName SocketName{};
		bool bGrounded{};

		#if ENABLE_VISUAL_LOG
			void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const;
		#endif
	};

	TArray<FTrackWheel> TrackWheels{};

	UPROPERTY(EditDefaultsOnly, Category = Throttle)
	float TrackMaxDrivingForce{ 4.0e5f };

	UPROPERTY(EditDefaultsOnly, Category = Throttle)
	float GroundTraceExtent{ 20.0f };

	UPROPERTY(EditDefaultsOnly, Category = Throttle)
	float GroundTraceInterval{ 0.1f };

	UPROPERTY(Transient)
	TArray<ASpringWheel*> Wheels;

	using FVectorBuffer = TR::TTimedCircularBuffer <
		FVector,
		decltype([]() { return FVector{ ForceInitToZero }; }),
		decltype([](const FVector& V) { return FMath::Max3(FMath::Abs(V.X), FMath::Abs(V.Y), FMath::Abs(V.Z)); })
	>;

	FVectorBuffer PositionBuffer;
	TR::TTimedCircularBuffer<float> ThrottleBuffer;

	/* Tick rate for stuck detection. Set to 0 to use the component tick and < 0 to disable. */
	UPROPERTY(EditDefaultsOnly, Category = "Stuck")
	float StuckCheckInterval{ 0.1f };
	
	/* Amount of time to sample for stuck detection. */
	UPROPERTY(EditDefaultsOnly, Category = "Stuck", meta = (ClampMin = "1.0"))
	float ThrottleSampleTime{ 3.0f };

	/* Multiplier on current throttle to give tank a boost when detected as stuck */
	UPROPERTY(EditDefaultsOnly, Category = "Stuck", meta = (ClampMin = "2.0"))
	float ThrottleBoostMultiplier{ 6.0f };

	/* Minimum average throttle over sampling interval in order for stuck detection to be active. */
	UPROPERTY(EditDefaultsOnly, Category = "Stuck", meta = (ClampMin = "0.1"))
	float ThrottleStuckDetectionThreshold{ 0.5f };

	/* Minimum displacement when above throttle threshold to detect not being stuck. */
	UPROPERTY(EditDefaultsOnly, Category = "Stuck", meta = (ClampMin = "0.0"))
	float StuckDisplacementThreshold{ 100.0f };

	/* Amount of time that elapses after being stuck before tank is reset. 
	*  To disable this behavior set to <= 0.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Stuck")
	float StuckResetThresholdTime{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Road Hack")
	float RoadAlignmentCosineThreshold{ 0.75f };

	UPROPERTY(EditDefaultsOnly, Category = "Road Hack")
	float CounterMagnitudeMaxValue{ 5e6 };

	UPROPERTY(EditDefaultsOnly, Category = "Road Hack")
	float CounterMangitudeThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Road Hack")
	float CounterMangitudeMinInterval{ 0.2f };

	UPROPERTY(EditDefaultsOnly, Category = "Road Hack")
	float CounterAirborneCooldownTime{ 5.0f };

	float LastCounterTime{};
	float LastAirborneTime{};

	float CalculatedStuckCheckInterval{};
	float LastStuckTime{ -1.0f };
	float LastStuckCheckTime{ -1.0f };
	float LastGroundTraceTime{ -1.0f };

	float CurrentThrottle{};
	float LastThrottle{};

	bool bStuckCheckingEnabled{};
	bool bStuckBoostActive{};
};

#pragma region Inline Definitions

FORCEINLINE bool UTankTrackComponent::IsAirborne() const
{
	return !IsGrounded();
}

FORCEINLINE float UTankTrackComponent::GetThrottle() const
{
	return LastThrottle;
}

#pragma endregion Inline Definitions
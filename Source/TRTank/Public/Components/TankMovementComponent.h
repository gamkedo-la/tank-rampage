// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/NavMovementComponent.h"
#include "TankMovementComponent.generated.h"

class UTankTrackComponent;

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TRTANK_API UTankMovementComponent : public UNavMovementComponent
{
	GENERATED_BODY()
	
public:

	UTankMovementComponent();

	struct FInitParams
	{
		UTankTrackComponent* LeftTrack{};
		UTankTrackComponent* RightTrack{};

		FString ToString() const;
	};

	void Initialize(const FInitParams& InitParams);

	UFUNCTION(BlueprintCallable)
	void MoveForward(float Throw);

	UFUNCTION(BlueprintCallable)
	void TurnRight(float Throw);

	virtual float GetMaxSpeed() const override;

	FVector GetComponentVelocity() const;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	virtual void RequestPathMove(const FVector& MoveInput) override;

private:
	bool IsMovementAllowed() const;

	void MoveTo(const FVector& MoveDirectionStrength);

#if ENABLE_VISUAL_LOG
	bool DidMoveThisFrame() const;
#endif

private:

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MaxSpeed{ 2000.0f };

	UPROPERTY(Transient)
	TObjectPtr<UTankTrackComponent> LeftTrack{};

	UPROPERTY(Transient)
	TObjectPtr<UTankTrackComponent> RightTrack{};

#if ENABLE_VISUAL_LOG
	FVector LastMovementVector{ EForceInit::ForceInitToZero };
	float LastMovementTime{ -1.0f };
#endif
};

#pragma region Inline Definitions

FORCEINLINE float UTankMovementComponent::GetMaxSpeed() const
{
	return MaxSpeed;
}

FORCEINLINE FVector UTankMovementComponent::GetComponentVelocity() const
{
	return UpdatedComponent ? UpdatedComponent->GetComponentVelocity() : Velocity;
}
#pragma endregion Inline Definitions
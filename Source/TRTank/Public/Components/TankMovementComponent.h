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

protected:
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTankTrackComponent> LeftTrack{};

	UPROPERTY(Transient)
	TObjectPtr<UTankTrackComponent> RightTrack{};
};

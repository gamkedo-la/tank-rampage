// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlippedOverCorrectionComponent.generated.h"

namespace TR::CollisionUtils
{
	struct FGroundData;
}

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRTANK_API UFlippedOverCorrectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFlippedOverCorrectionComponent();

	UFUNCTION(BlueprintPure)
	bool IsCurrentlyFlippedOver() const;

	UFUNCTION(BlueprintPure)
	float GetFlippedOverStartTime() const;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	bool IsActorFlippedOver(TR::CollisionUtils::FGroundData& GroundData) const;
	bool IsActorAboveSpeedThreshold() const;

	void ResetActorToGround(const TR::CollisionUtils::FGroundData& GroundData);

private:
	/**
	* Absolute value of the pitch angle where we detect the actor is probably flipped over.
	*/
	UPROPERTY(Category = "Movement", EditDefaultsOnly)
	float MinFlippedOverPitchDetectionAngle{ 80.0f };

	/*
	 * Minimum speed while angle considers actor flipped over before that we will disregard due to car still have forward speed.
	 * It's possible actor could collide with something to "right it".
	 * This value is in cm/s. 44.7 cm/s in 1mph.
	 */
	UPROPERTY(Category = "Movement", EditDefaultsOnly)
	float MaxFlippedOverSpeed{ 100.0f };

	/**
	* Time in flipped over state before we consider the actor permanently flipped over.
	*/
	UPROPERTY(Category = "Movement", EditDefaultsOnly)
	float FlippedOverTriggerDurationTime{ 3.0 };

	UPROPERTY(Category = "Movement", VisibleInstanceOnly)
	bool bIsFlippedOver{ };

	UPROPERTY(Category = "Movement", VisibleInstanceOnly)
	float FlippedOverStartGameTimeSeconds{};
		
};

#pragma region Inline Definitions

inline bool UFlippedOverCorrectionComponent::IsCurrentlyFlippedOver() const
{
	return bIsFlippedOver;
}

inline float UFlippedOverCorrectionComponent::GetFlippedOverStartTime() const
{
	return bIsFlippedOver ? FlippedOverStartGameTimeSeconds : -1;
}

#pragma endregion Inline Definitions

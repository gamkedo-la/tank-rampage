// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TankEngineSoundsComponent.generated.h"

class USoundBase;
class ABaseTankPawn;
class UAudioComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTankEngineSoundsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTankEngineSoundsComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	struct FEngineAudioParameters
	{
		float Throttle{};
		float Speed{};
		bool bAirborne{};
	};

	struct FEngineAudioValues
	{
		float IdleVolume{};
		float MovementVolume{};
		float MovementPitchShift{};
	};

	ABaseTankPawn* GetOwnerAsTank();
	const ABaseTankPawn* GetOwnerAsTank() const;

	TOptional<FEngineAudioParameters> GetEngineAudioParameters() const;

	UAudioComponent* CreateEngineAudioComponent() const;

	FEngineAudioValues NextAudioValues(const FEngineAudioParameters& Parameters) const;

	FEngineAudioValues DefaultAudioValues() const;

	void InterpAudioValues(FEngineAudioValues& NewAudioValues, float DeltaTime) const;

	bool CheckEngineAudioComponent();

	void InitAudio();

	void UpdateAudioComponent();

	float InterpValue(float Previous, float Current, float DeltaTime) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> EngineSfx{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	FName IdleVolumeParameterName{ "Idle Volume" };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	FName MovementVolumeParameterName{ "Movement Volume" };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	FName MovementPitchOffsetParameterName{ "Movement Pitch Offset" };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinIdleVolume{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxIdleVolume{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinMovementVolume{};

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxMovementVolume{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MinMovementPitchShift{ 0.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MaxMovementPitchShift{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float AudioInterpTime{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float MovementAudioMaxSpeed{ 15 * 100.0f };

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> EngineAudioComponent{};

	FEngineAudioValues CurrentEngineAudioValues{};
};

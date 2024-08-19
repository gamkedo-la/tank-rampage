// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "InputCharacteristics.h"

#include "TRGameInstance.generated.h"

class USoundMix;
class USoundClass;

/**
 * 
 */
UCLASS()
class UTRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintPure, Category = "Controls")
	bool IsGamepadAvailable() const;

private:
	void InitLoadingScreen();

	void InitSoundVolumes();

	void ApplyMixToSoundClass(USoundClass* SoundClass, float Volume);

	UFUNCTION()
	void BeginLoadingScreen(const FString& MapName);

	UFUNCTION()
	void EndLoadingScreen(UWorld* InLoadedWorld);

	void DoLoadingScreen();

	void InitGamepadAvailable();
	void HandleControllerConnectionChange(EInputDeviceConnectionState InputDeviceConnectionState, FPlatformUserId UserId, FInputDeviceId ControllerId);
	void HandleControllerPairingChanged(FInputDeviceId ControllerId, FPlatformUserId NewUserId, FPlatformUserId OldUserId);

#if WITH_EDITOR
	virtual FGameInstancePIEResult StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer, const FGameInstancePIEParameters& Params) override;
#endif

private:
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundMix> VolumeChangeMix{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> MasterSoundClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> SfxSoundClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundClass> MusicSoundClass{};
};


#pragma region Inline Definitions

FORCEINLINE bool UTRGameInstance::IsGamepadAvailable() const
{
	return TR::FInputCharacteristics::IsGamepadAvailable();
}

#pragma endregion Inline Definitions

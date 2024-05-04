// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
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

private:
	void InitLoadingScreen();

	void InitSoundVolumes();
	void DoInitSoundVolumes();

	void ApplyMixToSoundClass(USoundClass* SoundClass, float Volume);

	UFUNCTION()
	void BeginLoadingScreen(const FString& MapName);

	UFUNCTION()
	void EndLoadingScreen(UWorld* InLoadedWorld);

	void DoLoadingScreen();

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

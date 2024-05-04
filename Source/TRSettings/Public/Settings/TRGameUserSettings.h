// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "TRGameUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameUserSettingsUpdated);

/**
 * 
 */
UCLASS(BlueprintType, config = GameUserSettings, configdonotcheckdefaults)
class TRSETTINGS_API UTRGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, DisplayName = "Get TR GameUserSettings", Category = "GameUserSettings")
	static UTRGameUserSettings* GetInstance();

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetMasterVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetSfxVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetSfxVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetMusicVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetMusicVolume() const;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnGameUserSettingsUpdated OnGameUserSettingsUpdated {};

private:
	UPROPERTY(Config)
	float MasterVolume{ 1.0f };

	UPROPERTY(Config)
	float SfxVolume{ 1.0f };

	UPROPERTY(Config)
	float MusicVolume{ 1.0f };
};

#pragma region Inline Definitions

FORCEINLINE UTRGameUserSettings* UTRGameUserSettings::GetInstance()
{
	return CastChecked<UTRGameUserSettings>(GetGameUserSettings());
}

FORCEINLINE void UTRGameUserSettings::SetMasterVolume(float Value)
{
	MasterVolume = Value;
}

FORCEINLINE float UTRGameUserSettings::GetMasterVolume() const
{
	return MasterVolume;
}

FORCEINLINE void UTRGameUserSettings::SetSfxVolume(float Value)
{
	SfxVolume = Value;
}

FORCEINLINE float UTRGameUserSettings::GetSfxVolume() const
{
	return SfxVolume;
}

FORCEINLINE void UTRGameUserSettings::SetMusicVolume(float Value)
{
	MusicVolume = Value;
}

FORCEINLINE float UTRGameUserSettings::GetMusicVolume() const
{
	return MusicVolume;
}

#pragma endregion Inline Definitions

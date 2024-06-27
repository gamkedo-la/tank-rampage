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

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	static float GetMaxVolume();

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetMainVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetMainVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetSfxVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetSfxVolume() const;

	UFUNCTION(BlueprintCallable, Category = "GameUserSettings")
	void SetMusicVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "GameUserSettings")
	float GetMusicVolume() const;

	virtual void SaveSettings() override;

	virtual void LoadSettings(bool bForceReload = false) override;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnGameUserSettingsUpdated OnGameUserSettingsUpdated {};

private:

	static constexpr float MaxVolume = 2.0f;

	UPROPERTY(Config)
	float MainVolume{ 1.0f };

	UPROPERTY(Config)
	float SfxVolume{ 1.0f };

	UPROPERTY(Config)
	float MusicVolume{ 1.0f };

	bool bRequiresConfigCleanup{};
};

#pragma region Inline Definitions

FORCEINLINE float UTRGameUserSettings::GetMaxVolume()
{
	return MaxVolume;
}

FORCEINLINE UTRGameUserSettings* UTRGameUserSettings::GetInstance()
{
	return CastChecked<UTRGameUserSettings>(GetGameUserSettings());
}

FORCEINLINE void UTRGameUserSettings::SetMainVolume(float Value)
{
	MainVolume = Value;
}

FORCEINLINE float UTRGameUserSettings::GetMainVolume() const
{
	return MainVolume;
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

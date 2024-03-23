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
	UFUNCTION(BlueprintCallable, DisplayName = "Get TR GameUserSettings", Category = GameUserSettings)
	static UTRGameUserSettings* GetInstance();

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnGameUserSettingsUpdated OnGameUserSettingsUpdated {};
};

#pragma region Inline Definitions

inline UTRGameUserSettings* UTRGameUserSettings::GetInstance()
{
	return CastChecked<UTRGameUserSettings>(GetGameUserSettings());
}

#pragma endregion Inline Definitions

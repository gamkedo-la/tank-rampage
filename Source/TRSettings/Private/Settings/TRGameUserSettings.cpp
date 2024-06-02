// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/TRGameUserSettings.h"

#include "TRSettingsLogging.h"
#include "Logging/LoggingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TRGameUserSettings)

void UTRGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	UE_LOG(LogTRSettings, Display, TEXT("%s: ApplySettings: bCheckForCommandLineOverrides=%s"), *GetName(), LoggingUtils::GetBoolString(bCheckForCommandLineOverrides));

	Super::ApplySettings(bCheckForCommandLineOverrides);

	OnGameUserSettingsUpdated.Broadcast();
}
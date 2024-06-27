// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/TRGameUserSettings.h"

#include "TRSettingsLogging.h"
#include "Logging/LoggingUtils.h"
#include "Logging/StructuredLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TRGameUserSettings)

namespace
{
	 constexpr const TCHAR* TRGameUserSettingsSectionName = TEXT("/Script/TRSettings.TRGameUserSettings");
	 constexpr const TCHAR* OldMainVolumeKeyName = TEXT("MasterVolume");
}

void UTRGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	UE_LOGFMT(LogTRSettings, Display, "{Name}: ApplySettings: bCheckForCommandLineOverrides={HasOverrides}",
		*GetName(), LoggingUtils::GetBoolString(bCheckForCommandLineOverrides));

	Super::ApplySettings(bCheckForCommandLineOverrides);

	OnGameUserSettingsUpdated.Broadcast();
}

void UTRGameUserSettings::SaveSettings()
{
	check(GConfig);

	// Remove the previous value key so we don't overwrite the new values on the next load and also clean up the file
	if (bRequiresConfigCleanup)
	{
		GConfig->RemoveKey(TRGameUserSettingsSectionName, OldMainVolumeKeyName, *GGameUserSettingsIni);
		bRequiresConfigCleanup = false;
	}

	Super::SaveSettings();
}

void UTRGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	check(GConfig);

	// Port over any renamed settings that still exist in the ini file
	// Note that property redirectors DO NOT work in this case, and we have to do it manually
	if (GConfig->DoesSectionExist(TRGameUserSettingsSectionName, *GGameUserSettingsIni))
	{
		float OldMainVolumeValue;
		if (GConfig->GetFloat(TRGameUserSettingsSectionName, OldMainVolumeKeyName, OldMainVolumeValue, *GGameUserSettingsIni))
		{
			UE_LOGFMT(LogTRSettings, Display, "Found MasterVolume in ini file, replacing default MainVolume value with loaded value {Value}", OldMainVolumeValue);
			MainVolume = OldMainVolumeValue;
			bRequiresConfigCleanup = true;
		}
	}
}

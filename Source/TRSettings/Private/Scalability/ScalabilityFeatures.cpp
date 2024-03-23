// Fill out your copyright notice in the Description page of Project Settings.


#include "Scalability/ScalabilityFeatures.h"
#include "Settings/TRGameUserSettings.h"
#include "TRSettingsLogging.h"
#include "Logging/LoggingUtils.h"

namespace
{

	enum class FScalabilityQualityLevel : int8
	{
		Custom = -1,
		Low = 0,
		Medium = 1,
		High = 2,
		Epic = 3,
		Cinematic = 4
	};

	FScalabilityQualityLevel GetQualityLevel(const UGameUserSettings& GameSettings);
	bool DoesScalabilitySettingMeetThreshold(FScalabilityQualityLevel ThresholdLevel);

	// TODO: May want to move these to config ini file entries in DefaultGameUserSetting.ini: [/Script/TRSettings.TRGameUserSettings]
	constexpr FScalabilityQualityLevel MinDestructionLevel = FScalabilityQualityLevel::High;

}


bool TR::ScalabilityFeatures::IsDestructionAvailable()
{
	 const bool bAvailable = DoesScalabilitySettingMeetThreshold(MinDestructionLevel);

	 UE_LOG(LogTRSettings, Log, TEXT("IsDestructionAvailable: %s"), LoggingUtils::GetBoolString(bAvailable));
	 return bAvailable;
}

namespace
{
	FScalabilityQualityLevel GetQualityLevel(const UGameUserSettings& GameSettings)
	{
		return static_cast<FScalabilityQualityLevel>(GameSettings.GetOverallScalabilityLevel());
	}

	bool DoesScalabilitySettingMeetThreshold(FScalabilityQualityLevel ThresholdLevel)
	{
		const auto Settings = UTRGameUserSettings::GetInstance();
		return GetQualityLevel(*Settings) >= ThresholdLevel;
	}
}

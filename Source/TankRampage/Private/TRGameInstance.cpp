// Fill out your copyright notice in the Description page of Project Settings.


#include "TRGameInstance.h"

#include "TankRampageLogging.h"
#include "Logging/LoggingUtils.h"

#include "Settings/TRGameUserSettings.h"

#include "MoviePlayer.h"

#include "Sound/SoundMix.h"

#include "Sound/SoundClass.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TRGameInstance)

void UTRGameInstance::Init()
{
	UE_LOG(LogTankRampage, Display, TEXT("%s: Init"), *GetName());

	Super::Init();

	InitLoadingScreen();
}

void UTRGameInstance::InitLoadingScreen()
{
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::EndLoadingScreen);
}

void UTRGameInstance::InitSoundVolumes()
{
	const auto Settings = UTRGameUserSettings::GetInstance();
	if (!ensureMsgf(Settings, TEXT("TRGameUserSettings was NULL")))
	{
		return;
	}

	if (SfxSoundClass)
	{
		ApplyMixToSoundClass(SfxSoundClass, Settings->GetSfxVolume());
	}
	else
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: No SfxSoundClass set!"), *GetName());
	}

	if (MusicSoundClass)
	{
		ApplyMixToSoundClass(MusicSoundClass, Settings->GetMusicVolume());
	}
	else
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: No MusicSoundClass set!"), *GetName());
	}

	if (MasterSoundClass)
	{
		ApplyMixToSoundClass(MasterSoundClass, Settings->GetMasterVolume());
	}
	else
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: No MasterSoundClass set!"), *GetName());
	}
}

void UTRGameInstance::ApplyMixToSoundClass(USoundClass* SoundClass, float Volume)
{
	if (!VolumeChangeMix)
	{
		UE_LOG(LogTankRampage, Error, TEXT("%s: No VolumeChangeMix set!"), *GetName());
		return;
	}

	if (!SoundClass)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(GetWorld(), VolumeChangeMix, SoundClass, Volume, 1.0f, 0.0f, true);
	UGameplayStatics::PushSoundMixModifier(GetWorld(), VolumeChangeMix);

	UE_LOG(LogTankRampage, Log, TEXT("%s: Changed volume of %s to %f"), *GetName(), *SoundClass->GetName(), Volume);
}

void UTRGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (IsRunningDedicatedServer())
	{
		UE_LOG(LogTankRampage, Log, TEXT("BeginLoadingScreen: %s - Skip since on dedicated server"), *MapName);
		return;
	}

	UE_LOG(LogTankRampage, Display, TEXT("BeginLoadingScreen: %s"), *MapName);

	DoLoadingScreen();
}

void UTRGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	InitSoundVolumes();

	UE_LOG(LogTankRampage, Display, TEXT("EndLoadingScreen: %s"), *LoggingUtils::GetName(InLoadedWorld));
}

void UTRGameInstance::DoLoadingScreen()
{
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.bAllowInEarlyStartup = true;
	LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

#if WITH_EDITOR
FGameInstancePIEResult UTRGameInstance::StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer, const FGameInstancePIEParameters& Params)
{
	auto Result = Super::StartPlayInEditorGameInstance(LocalPlayer, Params);

	InitSoundVolumes();

	return Result;
}
#endif


// Fill out your copyright notice in the Description page of Project Settings.


#include "TRGameInstance.h"

#include "TankRampageLogging.h"
#include "Logging/LoggingUtils.h"

#include "Settings/TRGameUserSettings.h"

#include "InputCharacteristics.h"

#include "MoviePlayer.h"

#include "Sound/SoundMix.h"

#include "Sound/SoundClass.h"

#include "Kismet/GameplayStatics.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GenericPlatform/GenericApplication.h" 
#include "SlateBasics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TRGameInstance)

void UTRGameInstance::Init()
{
	UE_LOG(LogTankRampage, Display, TEXT("%s: Init"), *GetName());

	Super::Init();

	InitGamepadAvailable();
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
		ApplyMixToSoundClass(MasterSoundClass, Settings->GetMainVolume());
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

#pragma region Loading Screen

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

#pragma endregion Loading Screen

#pragma region Gamepad Support

void UTRGameInstance::InitGamepadAvailable()
{
	/* See:
	https://couchlearn.com/how-to-use-the-game-instance-in-unreal-engine-4/
	https://github.com/Noesis/UE4-ShooterGame/blob/master/Source/ShooterGame/Public/ShooterGameInstance.h
	https://github.com/Noesis/UE4-ShooterGame/blob/master/Source/ShooterGame/Private/ShooterGameInstance.cpp
	*/

	auto& PlatformInputDeviceMapper = IPlatformInputDeviceMapper::Get();
	PlatformInputDeviceMapper.GetOnInputDeviceConnectionChange().AddUObject(this, &ThisClass::HandleControllerConnectionChange);
	PlatformInputDeviceMapper.GetOnInputDevicePairingChange().AddUObject(this, &ThisClass::HandleControllerPairingChanged);

	// See https://answers.unrealengine.com/questions/142358/question-is-there-a-way-to-detect-a-gamepad.html?childToView=706040#answer-706040
	// for a solution as ControllerId will be 0 for player 1 regardless if the "controller" is a gamepad and the connection change only fires if connecting after game starts

	// See also https://answers.unrealengine.com/questions/463722/how-do-you-detect-a-second-gamepad-for-splitscreen.html
	// also https://answers.unrealengine.com/questions/291285/index.html

	// See  https://answers.unrealengine.com/questions/142358/view.html for below

	auto genericApplication = FSlateApplication::Get().GetPlatformApplication();
	bool bGamepadAvailable = genericApplication.IsValid() && genericApplication->IsGamepadAttached();

	TR::FInputCharacteristics::SetGamepadAvailable(bGamepadAvailable);

	UE_LOG(LogTankRampage, Display, TEXT("%s: InitGamepadAvailable - controller gamepad available=%s"), *GetName(), LoggingUtils::GetBoolString(bGamepadAvailable));
}

void UTRGameInstance::HandleControllerConnectionChange(EInputDeviceConnectionState InputDeviceConnectionState, FPlatformUserId UserId, FInputDeviceId ControllerId)
{
	const bool bConnected = InputDeviceConnectionState == EInputDeviceConnectionState::Connected;

	UE_LOG(LogTankRampage, Display, TEXT("%s: HandleControllerConnectionChange - bConnected=%s;InputDeviceConnectionState=%d;UserId=%d;ControllerId=%d"),
		*GetName(), LoggingUtils::GetBoolString(bConnected), InputDeviceConnectionState, UserId.GetInternalId(), ControllerId.GetId());

	TR::FInputCharacteristics::SetGamepadAvailable(bConnected);
}

void UTRGameInstance::HandleControllerPairingChanged(FInputDeviceId ControllerId, FPlatformUserId NewUserId, FPlatformUserId OldUserId)
{
	UE_LOG(LogTankRampage, Display, TEXT("%s: HandleControllerPairingChanged - bConnected=TRUE; ControllerIndex=%d;NewUserId=%d;OldUserId=%d"),
		*GetName(), ControllerId.GetId(), NewUserId.GetInternalId(), OldUserId.GetInternalId());

	TR::FInputCharacteristics::SetGamepadAvailable(true);
}

#pragma endregion Gamepad Support

#if WITH_EDITOR
FGameInstancePIEResult UTRGameInstance::StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer, const FGameInstancePIEParameters& Params)
{
	auto Result = Super::StartPlayInEditorGameInstance(LocalPlayer, Params);

	InitSoundVolumes();

	return Result;
}
#endif

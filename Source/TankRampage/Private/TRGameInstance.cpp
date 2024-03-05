// Fill out your copyright notice in the Description page of Project Settings.


#include "TRGameInstance.h"

#include "TankRampageLogging.h"
#include "Logging/LoggingUtils.h"

#include "MoviePlayer.h"

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


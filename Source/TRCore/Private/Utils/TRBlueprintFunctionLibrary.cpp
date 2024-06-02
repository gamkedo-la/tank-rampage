// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/TRBlueprintFunctionLibrary.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/ConfigCacheIni.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TRBlueprintFunctionLibrary)

bool UTRBlueprintFunctionLibrary::IsRunningInEditor(const UObject* WorldContextObject)
{
	if(!WorldContextObject)
	{
		return false;
	}

	auto World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	return !World->IsGameWorld();
}

FString UTRBlueprintFunctionLibrary::GetProjectVersion()
{
	FString GameVersion;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		GameVersion,
		GGameIni
	);

	return FString::Printf(TEXT("v %s"), *GameVersion);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/TRBlueprintFunctionLibrary.h"

#include "Components/InstancedStaticMeshComponent.h"

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

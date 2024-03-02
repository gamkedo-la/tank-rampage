// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TRBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TRCORE_API UTRBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Tank Rampage|Utils", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsRunningInEditor(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Tank Rampage|Utils")
	static FString GetProjectVersion();
};

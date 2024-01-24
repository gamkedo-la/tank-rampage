// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BillboardComponent.h"
#include "SpawnLocationComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class USpawnLocationComponent : public UBillboardComponent
{
	GENERATED_BODY()

public:	
	USpawnLocationComponent();
};

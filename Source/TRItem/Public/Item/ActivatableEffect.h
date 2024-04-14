// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/Item.h"
#include "ActivatableEffect.generated.h"

/**
 * An effect that has to be activated such as a turbo boost.
 */
UCLASS(Abstract)
class TRITEM_API UActivatableEffect : public UItem
{
	GENERATED_BODY()

public:
	UActivatableEffect();
	
};

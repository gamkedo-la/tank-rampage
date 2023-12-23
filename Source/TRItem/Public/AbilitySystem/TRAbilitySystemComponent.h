// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "TRAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class TRITEM_API UTRAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif
	
};

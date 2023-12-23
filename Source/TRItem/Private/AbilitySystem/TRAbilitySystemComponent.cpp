// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TRAbilitySystemComponent.h"



#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void UTRAbilitySystemComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Ability System Component");

	Snapshot->Status.Add(Category);
}

#endif

#pragma endregion Visual Logger

// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TRAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

UTRAttributeSet::UTRAttributeSet()
{
	// constructor is too early for setter, but we can use the initter here
	InitHealth(2000);
	InitMaxHealth(2000);

	InitXPLevel(1);
	InitXPTotal(0);
}

void UTRAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// GAS, replicate anyways even if doesn't change - want to respond to every event
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, XPTotal, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, XPLevel, COND_None, REPNOTIFY_Always);
}

void UTRAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, Health, OldHealth);
}

void UTRAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, MaxHealth, OldMaxHealth);
}

void UTRAttributeSet::OnRep_XPTotal(const FGameplayAttributeData& OldXPTotal) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, XPTotal, OldXPTotal);
}

void UTRAttributeSet::OnRep_XPLevel(const FGameplayAttributeData& OldXPLevel) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, XPLevel, OldXPLevel);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/TRAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"

#include "Net/UnrealNetwork.h"

UTRAttributeSet::UTRAttributeSet()
{
	// constructor is too early for setter, but we can use the initter here

	InitXPLevel(1);
	InitXPTotal(0);
}

void UTRAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// GAS, replicate anyways even if doesn't change - want to respond to every event
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, MaxArmor, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, Speed, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, XPTotal, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTRAttributeSet, XPLevel, COND_None, REPNOTIFY_Always);
}

void UTRAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		// This only clamps the value returned by querying the modifier but doesn't permantently change the current health
		NewValue = GetClampedHealth(NewValue);
	}
	else if (Attribute == GetArmorAttribute())
	{
		NewValue = GetClampedArmor(NewValue);
	}
}

void UTRAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	auto& EvaluatedData = Data.EvaluatedData;

	if (EvaluatedData.Attribute == GetHealthAttribute())
	{
		// This will permanently clamp the health attribute to the proper range
		SetHealth(GetClampedHealth(GetHealth()));
	}
	else if (EvaluatedData.Attribute == GetArmorAttribute())
	{
		SetArmor(GetClampedArmor(GetArmor()));
	}
}

void UTRAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, Health, OldHealth);
}

void UTRAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, MaxHealth, OldMaxHealth);
}

void UTRAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, Armor, OldArmor);
}

void UTRAttributeSet::OnRep_MaxArmor(const FGameplayAttributeData& OldMaxArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, MaxArmor, OldMaxArmor);
}

void UTRAttributeSet::OnRep_Speed(const FGameplayAttributeData& OldSpeed) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, Speed, OldSpeed);
}

void UTRAttributeSet::OnRep_XPTotal(const FGameplayAttributeData& OldXPTotal) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, XPTotal, OldXPTotal);
}

void UTRAttributeSet::OnRep_XPLevel(const FGameplayAttributeData& OldXPLevel) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTRAttributeSet, XPLevel, OldXPLevel);
}

float UTRAttributeSet::GetClampedHealth(float CurrentHealth) const
{
	return FMath::Clamp(CurrentHealth, 0.0f, GetMaxHealth());
}

float UTRAttributeSet::GetClampedArmor(float CurrentArmor) const
{
	return FMath::Clamp(CurrentArmor, 0.0f, GetMaxArmor());
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "TRAttributeSet.generated.h"

// See UAttributeSet.h for where this convenience macro is mentioned to create convenience getters, setters, and initters

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
 	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// Use with 
// ATTRIBUTE_ACCESSORS(UTRAttributeSet, MyAttribute)

/**
 * 
 */
UCLASS()
class TRITEM_API UTRAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UTRAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData Health;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_MaxHealth, BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_XPTotal, BlueprintReadOnly, Category = "XP Attributes")
	FGameplayAttributeData XPTotal;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_XPLevel, BlueprintReadOnly, Category = "XP Attributes")
	FGameplayAttributeData XPLevel;

	ATTRIBUTE_ACCESSORS(UTRAttributeSet, Health)
	ATTRIBUTE_ACCESSORS(UTRAttributeSet, MaxHealth)
	ATTRIBUTE_ACCESSORS(UTRAttributeSet, XPTotal)
	ATTRIBUTE_ACCESSORS(UTRAttributeSet, XPLevel)

private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	UFUNCTION()
	void OnRep_XPTotal(const FGameplayAttributeData& OldXPTotal) const;

	UFUNCTION()
	void OnRep_XPLevel(const FGameplayAttributeData& OldXPLevel) const;

};

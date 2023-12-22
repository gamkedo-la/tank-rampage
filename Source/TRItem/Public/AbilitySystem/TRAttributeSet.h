// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "TRAttributeSet.generated.h"

// See UAttributeSet.h for where this convenience macro is mentioned to create convenience getters and setters

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
	
};

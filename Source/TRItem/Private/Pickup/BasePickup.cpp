// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/BasePickup.h"

#include "Logging/LoggingUtils.h"
#include "TRItemLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

ABasePickup::ABasePickup()
{	
	PrimaryActorTick.bCanEverTick = false;
}

void ABasePickup::BeginPlay()
{
	Super::BeginPlay();
}

void ABasePickup::ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!ensure(GameplayEffectClass))
	{
		return;
	}

	auto TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetAbilitySystemComponent)
	{
		return;
	}

	auto EffectContextHandle = TargetAbilitySystemComponent->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	EffectContextHandle.AddOrigin(GetActorLocation());

	auto GameplayEffectSpecHandle = TargetAbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, 1.0f, EffectContextHandle);
	check(GameplayEffectSpecHandle.Data);

	UE_VLOG_UELOG(this, LogTRItem, Log, TEXT("%s: Applying effect %s to %s"), *GetName(), *LoggingUtils::GetName(GameplayEffectClass), *LoggingUtils::GetName(Target));

	TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*GameplayEffectSpecHandle.Data);
}

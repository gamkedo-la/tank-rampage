// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "TRAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectAssetTagsAdded, const FGameplayTagContainer&, AssetTagsContainer);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGameplayTagChanged, UTRAbilitySystemComponent* /*ABS*/, const FGameplayTag& /* Tag*/, bool /* bAdded*/);
/**
 * 
 */
UCLASS()
class TRITEM_API UTRAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnEffectAssetTagsAdded OnEffectAssetTagsAdded{};

	FOnGameplayTagChanged OnGameplayTagChanged;


	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void OnTagUpdated(const FGameplayTag& Tag, bool bTagExists) override;

private:
	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& GameplayEffectSpec, FActiveGameplayEffectHandle ActiveGameplayEffectHandle);

private:
	FDelegateHandle EffectAppliedDelegateHandle;
	
};

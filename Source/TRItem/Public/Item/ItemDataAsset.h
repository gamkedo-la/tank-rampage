// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

class UWeapon;

/**
 * 
 */
UCLASS()
class TRITEM_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Category = "Main Gun", EditDefaultsOnly, BlueprintReadOnly)
	float MainGunCooldownTimeSeconds{ 3.0f };

	UPROPERTY(Category = "Main Gun", EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UWeapon> MainGunClass{ };
};

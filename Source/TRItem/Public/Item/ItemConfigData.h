// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "ItemConfigData.generated.h"

class UItem;

/**
 * 
 */
USTRUCT(BlueprintType)
struct TRITEM_API FItemConfigData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = "Item", EditAnywhere, BlueprintReadOnly)
	float CooldownTimeSeconds { 3.0f };

	UPROPERTY(Category = "Item", EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UItem> Class{};

	UPROPERTY(Category = "Item | UI", EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> UIMaterial{};

	UPROPERTY(Category = "Item | UI", EditAnywhere, BlueprintReadOnly)
	bool bShowInHUDItemSlots{ true };

	FString ToString() const;
};

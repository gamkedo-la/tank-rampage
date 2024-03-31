// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TRHUD.generated.h"

/**
 * 
 */
UCLASS()
class TRUI_API ATRHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void ShowHUD() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnGameOver();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnToggleHUDVisibility(bool bVisible);
};

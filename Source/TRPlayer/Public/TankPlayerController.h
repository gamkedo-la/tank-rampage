// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePlayerController.h"
#include "TankPlayerController.generated.h"


class UInputMappingContext;
struct FInputActionValue;
class UInputAction;

/**
 * 
 */
UCLASS()
class TRPLAYER_API ATankPlayerController : public ABasePlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void InitializeInputMappingContext();
	void InitializeCamera();

	void OnLook(const FInputActionValue& Value);

private:

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> InputMappingContext{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction{};

	UPROPERTY(EditDefaultsOnly, Category = Input, meta=(ClampMin = "-90", ClampMax = "90"))
	float MinPitch{ -20.0f };

	UPROPERTY(EditDefaultsOnly, Category = Input, meta = (ClampMin = "-90", ClampMax = "90"))
	float MaxPitch{ 45.0f };
};

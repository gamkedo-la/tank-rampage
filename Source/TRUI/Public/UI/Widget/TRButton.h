// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "TRButton.generated.h"

class USoundBase;

/**
 * 
 */
UCLASS()
class TRUI_API UTRButton : public UButton
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gamepad | Button")
	void Hover();

	UFUNCTION(BlueprintCallable, Category = "Gamepad | Button")
	void ExitHover();

	UFUNCTION(BlueprintCallable, Category = "Gamepad | Button")
	void Click();

protected:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

private:

	void BindEvents();

	UFUNCTION()
	void DoHover();

	void TickHover(float DeltaTime);

	void UnregisterTimer();

protected:
	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverMaxScale{ 1.0f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverUpdateTime{ 1 / 30.0f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverScaleTime{ 1.0f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverDelayTime{ 0.1f };

	UPROPERTY(EditAnywhere, Category = "Hover")
	float HoverEaseFactor{ 2.0f };

	FDelegateHandle HoverHandle{};

	float HoverStartTime{ -1.0f };
};

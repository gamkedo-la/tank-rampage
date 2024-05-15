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

protected:
	virtual void PostInitProperties() override;

private:

	void BindEvents();

	UFUNCTION()
	void DoClick();

	UFUNCTION()
	void DoHover();

	void PlayAudio(USoundBase* Sound) const;

protected:
	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> HoverSfx{};

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<USoundBase> ClickSfx{};
};

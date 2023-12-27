// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelUnlocksContext.h"

#include <optional>

#include "LevelUnlocksComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ULevelUnlocksComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULevelUnlocksComponent();

	std::optional<FLevelUnlocksContext> GetNextLevelUnlockOptions(int32 NextLevel) const;

	UFUNCTION(BlueprintCallable)
	void ApplyLevelUnlock(APawn* Pawn, const FLevelUnlock& Unlock);

protected:
	virtual void BeginPlay() override;

private:

	// TODO: A DataTable is a better option for maintaining this
	// Then in begin play could convert it to this TArray<FLevelUnlocksConfig> structure
	UPROPERTY(EditDefaultsOnly)
	TArray<FLevelUnlocksConfig> LevelUnlocks;
};


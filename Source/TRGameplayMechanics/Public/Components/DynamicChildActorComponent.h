// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "DynamicChildActorComponent.generated.h"

/**
 * Child Actor Component that can be disabled on start and later enabled or disabled.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class TRGAMEPLAYMECHANICS_API UDynamicChildActorComponent : public UChildActorComponent
{
	GENERATED_BODY()

public:
	 UFUNCTION(BlueprintCallable, Category = "Dynamic Child Actor Component")
	 void SpawnActor();

	 UFUNCTION(BlueprintCallable, Category = "Dynamic Child Actor Component")
	 void DestroyActor();

	 virtual void CreateChildActor(TFunction<void(AActor*)> CustomizerFunc = nullptr) override;

	 virtual void OnRegister() override;

private:
	bool ShouldCreateActor() const;
	bool ShouldSpawnActor() const;

private:
	UPROPERTY(EditAnywhere, Category = "Spawn")
	bool bSpawnOnRegister{};

	UPROPERTY(EditAnywhere, Category = "Spawn")
	bool bSpawnInEditor{};
};

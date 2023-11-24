// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TankAimingComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTankAimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	struct FTankComponents
	{
		UPrimitiveComponent* Barrel{};
		UPrimitiveComponent* Turret{};
	};

	UTankAimingComponent();

	void SetTankComponents(const FTankComponents& TankComponents);

	void AimAt(const FVector& Location, float LaunchSpeed);


#if ENABLE_VISUAL_LOG

	void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;

#endif

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent();

private:
	void MoveBarrelTowards(const FVector& AimDirection) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Barrel{};

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Turret{};
};

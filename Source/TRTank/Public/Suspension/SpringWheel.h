// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpringWheel.generated.h"

class UStaticMeshComponent;
class UPhysicsConstraintComponent;

UCLASS()
class TRTANK_API ASpringWheel : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpringWheel();

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UPhysicsConstraintComponent> MassWheelConstraint{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> Wheel{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> Mass{};
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpringWheel.generated.h"

class USphereComponent;
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
	void SetupConstraint();

private:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UPhysicsConstraintComponent> MassAxleConstraint{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<USphereComponent> WheelComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<USphereComponent> AxleComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UPhysicsConstraintComponent> AxleWheelConstraint{};
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "SpringWheel.generated.h"

class USphereComponent;
class UPhysicsConstraintComponent;

UCLASS()
class TRTANK_API ASpringWheel : public AActor, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()
	
public:	
	ASpringWheel();

	void AddDrivingForce(float ForceMagnitude);

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
private:
	void SetupConstraint();
	const UObject* GetLogContext() const;

	bool IsGrounded() const;

	void ApplyDrivingForce();

	void SetupTickDependencies();

private:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UPhysicsConstraintComponent> MassAxleConstraint{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<USphereComponent> WheelComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<USphereComponent> AxleComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UPhysicsConstraintComponent> AxleWheelConstraint{};

	UPROPERTY(Transient)
	TObjectPtr<AActor> AttachParent{};

	UPROPERTY(EditDefaultsOnly, Category = Driving)
	float GroundTraceExtent{ 20 };

	float CurrentForce{};
};

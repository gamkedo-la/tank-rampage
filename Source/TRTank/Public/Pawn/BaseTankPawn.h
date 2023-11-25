// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "BaseTankPawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UTankAimingComponent;
class UTankTurretComponent;
class UTankBarrelComponent;

UCLASS()
class TRTANK_API ABaseTankPawn : public APawn, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABaseTankPawn();

	USpringArmComponent* GetCameraSpringArm() const;

	UFUNCTION(BlueprintCallable)
	void AimAt(const FVector& Location);

	UFUNCTION(BlueprintCallable)
	void Fire();

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual void NotifyControllerChanged() override;

private:
	void UpdateSpringArmTickEnabled();


protected:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTankAimingComponent> TankAimingComponent{};

private:
	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankBody{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTankTurretComponent> TankTurret{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTankBarrelComponent> TankBarrel{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankTreadRight{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankTreadLeft{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankTreadSideLeft{};

	UPROPERTY(Category = "Tank Model", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TankTreadSideRight{};

	UPROPERTY(Category = "Camera", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraSpringArm{};

	UPROPERTY(Category = "Camera", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera{};

	UPROPERTY(Category = "Firing", EditDefaultsOnly)
	float TankShellSpeed{ 100000 };
};


#pragma region Inline Definitions

inline USpringArmComponent* ABaseTankPawn::GetCameraSpringArm() const
{
	check(CameraSpringArm);
	return CameraSpringArm;
}

#pragma endregion Inline Definitions

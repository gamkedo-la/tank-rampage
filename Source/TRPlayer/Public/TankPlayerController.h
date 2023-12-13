// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePlayerController.h"
#include "Interfaces/TankOwner.h"

#include <optional>

#include "TankPlayerController.generated.h"


class UInputMappingContext;
struct FInputActionValue;
class UInputAction;
class ABaseTankPawn;
class UXPCollectionComponent;

/**
 * 
 */
UCLASS()
class TRPLAYER_API ATankPlayerController : public ABasePlayerController, public ITankOwner
{
	GENERATED_BODY()

public:
	ATankPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	virtual ABaseTankPawn* GetControlledTank() const override;

private:
	void InitializeInputMappingContext();
	void InitializeCamera();
	void AimTowardCrosshair();

	void OnLook(const FInputActionValue& Value);
	void OnFire();
	void OnMove(const FInputActionValue& Value);

	std::optional<FVector> GetRaySightHitLocation() const;

	FVector2D GetCrosshairScreenspaceLocation() const;
	FVector GetCrosshairWorldDirection(const FVector2D& ScreenLocation) const;
	std::optional<FVector> GetLookVectorHitLocation(const FVector& LookDirection) const;

private:

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UXPCollectionComponent> XPCollectionComponent{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> InputMappingContext{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction{};

	UPROPERTY(EditDefaultsOnly, Category = Input, meta=(ClampMin = "-90", ClampMax = "90"))
	float MinPitch{ -20.0f };

	UPROPERTY(EditDefaultsOnly, Category = Input, meta = (ClampMin = "-90", ClampMax = "90"))
	float MaxPitch{ 45.0f };

	UPROPERTY(EditDefaultsOnly, Category = Aim)
	FVector2D CrosshairPositionFraction{};

	float MaxAimDistanceMeters{ 1000.0f };
};

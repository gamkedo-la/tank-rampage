// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePlayerController.h"
#include "Interfaces/TankOwner.h"

#include <optional>

#include "TankPlayerController.generated.h"


struct FAimingData;
class UInputMappingContext;
struct FInputActionValue;
class UInputAction;
class ABaseTankPawn;
class UHealthComponent;
class UEnhancedInputComponent;
struct FInputActionInstance;

/**
 * 
 */
UCLASS()
class TRPLAYER_API ATankPlayerController : public ABasePlayerController, public ITankOwner
{
	GENERATED_BODY()

public:
	ATankPlayerController();

	virtual void GameHasEnded(AActor* EndGameFocus, bool bIsWinner) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	virtual ABaseTankPawn* GetControlledTank() const override;

	virtual void OnPossess(APawn* InPawn) override;

private:

	bool IsControlledTankAlive() const;

	void InitializeInputMappingContext();
	void InitializeCamera();
	void BindWeaponSelectActions(UEnhancedInputComponent& EnhancedInputComponent);

	void AimTowardCrosshair();

	void OnLook(const FInputActionValue& Value);
	void OnFire();
	void OnMove(const FInputActionValue& Value);

	void OnSelectWeapon(const FInputActionInstance& InputActionInstance);

	void SelectWeapon(int32 WeaponIndex) const;

	void GetAimingData(FAimingData& AimingData) const;
	
	FVector2D GetCrosshairScreenspaceLocation() const;

	UFUNCTION()
	void OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, AController* EventInstigator, AActor* ChangeCauser);

private:

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> InputMappingContext{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction{};

	UPROPERTY(EditDefaultsOnly, Category = Input)
	TArray<const UInputAction*> WeaponSelectActions{};

	UPROPERTY(EditDefaultsOnly, Category = Input, meta=(ClampMin = "-90", ClampMax = "90"))
	float MinPitch{ -20.0f };

	UPROPERTY(EditDefaultsOnly, Category = Input, meta = (ClampMin = "-90", ClampMax = "90"))
	float MaxPitch{ 45.0f };

	UPROPERTY(EditDefaultsOnly, Category = Aim)
	FVector2D CrosshairPositionFraction{};

	float MaxAimDistanceMeters{ 1000.0f };
};

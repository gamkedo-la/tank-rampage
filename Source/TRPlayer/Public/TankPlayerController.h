// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePlayerController.h"
#include "Interfaces/TankOwner.h"


#include "Containers/TimedCircularBuffer.h"

#include <optional>

#include "TankPlayerController.generated.h"


struct FAimingData;
class UInputMappingContext;
struct FInputActionValue;
class UInputAction;
class ABaseTankPawn;
class UHealthComponent;
class UItemInventory;
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

	UFUNCTION(BlueprintPure)
	const UInputAction* GetInputActionForItemNameAndIndex(const FName& ItemName, int32 ItemIndex) const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	virtual ABaseTankPawn* GetControlledTank() const override;
	

	virtual void OnPossess(APawn* InPawn) override;

private:

	ABaseTankPawn* GetControlledTankIfInputShouldBeActive() const;

	bool IsControlledTankAlive() const;

	void InitializeInputMappingContext();
	void InitializeCamera();
	void BindWeaponSelectActions(UEnhancedInputComponent& EnhancedInputComponent);

	void AimTowardCrosshair();

	void OnLook(const FInputActionValue& Value);
	void OnFire();
	void OnMove(const FInputActionValue& Value);
	void OnActivateTurbo();

	void OnSelectWeapon(const FInputActionInstance& InputActionInstance);
	void OnNextWeapon(const FInputActionInstance& InputActionInstance);
	void OnPreviousWeapon(const FInputActionInstance& InputActionInstance);

	void SelectWeapon(int32 WeaponIndex) const;

	void GetAimingData(FAimingData& AimingData, float ZeroingDistance) const;
	void CrosshairToAimingData(const FVector2D& CrosshairScreenLocation, FAimingData& AimingData) const;

	std::optional<FVector> GetAimingTargetLocation(const FVector& AimStartLocation, const FVector& AimTargetDirection, float ZeroingDistance) const;
	
	FVector2D GetCrosshairScreenspaceLocation() const;

	UFUNCTION()
	void OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, float PreviousMaxHealthValue, AController* EventInstigator, AActor* ChangeCauser);

	bool IsWeaponScrollSwitchTriggerable(const FInputActionInstance& InputActionInstance) const;

	UItemInventory* GetItemInventory() const;
	
private:
	
	using FVectorBuffer = TR::TTimedCircularBuffer < FVector, decltype([]() { return FVector{ ForceInitToZero }; }) > ;
	mutable FVectorBuffer AimDirectionBuffer;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> InputMappingContext{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction{};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ActivateTurboAction{};

	UPROPERTY(EditDefaultsOnly, Category = Input)
	TArray<const UInputAction*> WeaponSelectActions{};

	UPROPERTY(EditDefaultsOnly, Category = Input)
	TObjectPtr<UInputAction> WeaponNextAction{};

	UPROPERTY(EditDefaultsOnly, Category = Input)
	TObjectPtr<UInputAction> WeaponPreviousAction{};

	UPROPERTY(EditDefaultsOnly, Category = Input, meta=(ClampMin = "-90", ClampMax = "90"))
	float MinPitch{ -20.0f };

	UPROPERTY(EditDefaultsOnly, Category = Input, meta = (ClampMin = "-90", ClampMax = "90"))
	float MaxPitch{ 45.0f };

	UPROPERTY(EditDefaultsOnly, Category = Aim)
	FVector2D CrosshairPositionFraction{};

	UPROPERTY(EditDefaultsOnly, Category = Aim)
	int32 AimDirectionSmoothingSamples{ 6 };

	float MaxAimDistanceMeters{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = Input, meta=(ClampMin = "0"))
	float WeaponScrollRetriggerDelay{ 0.25f };

	float WeaponScrollLastTriggerTime{ -1.0f };
};

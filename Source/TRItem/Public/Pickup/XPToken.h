// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/BasePickup.h"
#include "XPToken.generated.h"

class UGameplayEffect;

/*
* XP token pickup from killed enemies.
*/
UCLASS()
class TRITEM_API AXPToken : public ABasePickup
{
	GENERATED_BODY()
	
public:	
	AXPToken();

protected:
	virtual void BeginPlay() override;

	virtual void OnOverlap(APawn* PlayerPawn) override;

private:
	UPROPERTY(Category = "Collision", VisibleAnywhere)
	class USphereComponent* CollisionVolume{};

	UPROPERTY(Category = "Mesh", VisibleAnywhere)
	class UStaticMeshComponent* Mesh{};
};

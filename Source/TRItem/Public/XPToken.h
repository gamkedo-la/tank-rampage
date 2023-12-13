// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XPToken.generated.h"

/*
* XP token pickup from killed enemies.
*/
UCLASS()
class TRITEM_API AXPToken : public AActor
{
	GENERATED_BODY()
	
public:	
	AXPToken();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(Category = "Collision", VisibleAnywhere)
	class USphereComponent* CollisionVolume{};

	UPROPERTY(Category = "Mesh", VisibleAnywhere)
	class UStaticMeshComponent* Mesh{};
};

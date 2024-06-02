// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseCollisionRelevanceComponent.generated.h"


UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRCORE_API UBaseCollisionRelevanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBaseCollisionRelevanceComponent();

protected:
	virtual void BeginPlay() override;

	virtual void RegisterCollisions() PURE_VIRTUAL(UBaseCollisionRelevanceComponent::RegisterCollisions, ;);

	virtual void OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
		PURE_VIRTUAL(UBaseCollisionRelevanceComponent::OnRelevantCollision, ;);

	UFUNCTION(BlueprintCallable)
	void RegisterOwner();

	UFUNCTION(BlueprintCallable)
	void RegisterComponent(UPrimitiveComponent* Component);

private:
	UFUNCTION()
	void OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
		
	bool IsRelevantCollision(const FHitResult& Hit) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	TArray<FString> ActorSubstrings;

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	TArray<FString> ComponentSubstrings;

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypes;

	/* Set this true to notify all but the configured names and types, making the configuration a deny list instead of an allow list. */
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	bool bNotifyAllButSpecified{ };

	bool bMatchTrueCondition{};
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/FlippedOverCorrectionComponent.h"

#include "Utils/CollisionUtils.h"

#include "TRTankLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlippedOverCorrectionComponent)

DEFINE_VLOG_EVENT(EventActorFlippedOver, Display, "Flipped Over")

UFlippedOverCorrectionComponent::UFlippedOverCorrectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UFlippedOverCorrectionComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UFlippedOverCorrectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	auto MyActor = GetOwner();
	check(MyActor);

	if (IsActorAboveSpeedThreshold())
	{
		bIsFlippedOver = false;
		return;
	}

	auto World = GetWorld();
	check(World);

	TR::CollisionUtils::FGroundData GroundData;

	const auto bFlippedOverThisFrame = IsActorFlippedOver(GroundData);

	if (bIsFlippedOver && !bFlippedOverThisFrame)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
			TEXT("%s-%s: TickComponent: Actor no longer flipped over."),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		bIsFlippedOver = false;
	}
	else if (!bIsFlippedOver && bFlippedOverThisFrame)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
			TEXT("%s-%s: TickComponent: Actor first detected as flipped over"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		
		FlippedOverStartGameTimeSeconds = World->GetTimeSeconds();
		bIsFlippedOver = true;
	}
	else if (bFlippedOverThisFrame && (World->GetTimeSeconds() - FlippedOverStartGameTimeSeconds) >= FlippedOverTriggerDurationTime)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Display,
			TEXT("%s-%s: TickComponent: Actor flipped over for over %fs - ResetActorToGround"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), FlippedOverTriggerDurationTime);

		ResetActorToGround(GroundData);
	}
}

bool UFlippedOverCorrectionComponent::IsActorFlippedOver(TR::CollisionUtils::FGroundData& GroundData) const
{
	const auto MyActor = GetOwner();
	check(MyActor);

	const auto GroundDataOptional = TR::CollisionUtils::GetGroundData(*MyActor);

	if (!GroundDataOptional)
	{
		UE_VLOG_UELOG(GetOwner(), LogTRTank, Warning,
			TEXT("%s-%s: IsActorFlippedOver: Could not determine ground location"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		return false;
	}

	GroundData = *GroundDataOptional;

	const FVector& VehicleUpVector = MyActor->GetActorUpVector();
	const auto DotProduct = VehicleUpVector | GroundData.Normal;

	const float Angle = FMath::Abs(FMath::RadiansToDegrees(FMath::Acos(DotProduct)));
	const bool bIsCurrentlyFlippedOver = Angle >= MinFlippedOverPitchDetectionAngle;

	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log,
		TEXT("%s-%s: IsActorFlippedOver: %s: Angle=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), LoggingUtils::GetBoolString(bIsCurrentlyFlippedOver), Angle);

	UE_VLOG_ARROW(GetOwner(), LogTRTank, Log, GroundData.Location, GroundData.Location + 100.0f * GroundData.Normal, FColor::Red, TEXT("World Up"));
	UE_VLOG_ARROW(GetOwner(), LogTRTank, Log, GroundData.Location, GroundData.Location + 100.0f * VehicleUpVector, FColor::Red, TEXT("Actor Up"));

	return bIsCurrentlyFlippedOver;
}

bool UFlippedOverCorrectionComponent::IsActorAboveSpeedThreshold() const
{
	auto MyActor = GetOwner();
	check(MyActor);

	const auto CurrentVelocitySq = MyActor->GetVelocity().SizeSquared();
	if (CurrentVelocitySq > FMath::Square(MaxFlippedOverSpeed))
	{
		UE_LOG(LogTRTank, Verbose, TEXT("%s: Was possibly flipped over but speed=%f > MaxFlippedOverSpeed=%f"), *GetName(), FMath::Sqrt(CurrentVelocitySq), MaxFlippedOverSpeed);

		return true;
	}

	return false;
}

void UFlippedOverCorrectionComponent::ResetActorToGround(const TR::CollisionUtils::FGroundData& GroundData)
{
	auto MyActor = GetOwner();
	check(MyActor);

	TR::CollisionUtils::ResetActorToGround(GroundData, *MyActor);

	UE_VLOG_EVENT_WITH_DATA(GetOwner(), EventActorFlippedOver);
	UE_VLOG_LOCATION(GetOwner(), LogTRTank, Log, MyActor->GetActorLocation(), 50.0f, FColor::Red, TEXT("Flip Reset"));
	UE_VLOG_ARROW(GetOwner(), LogTRTank, Log, MyActor->GetActorLocation(), MyActor->GetActorLocation() + GroundData.Normal * 200.0f, FColor::Red, TEXT("Flip Dir"));
	UE_VLOG_UELOG(GetOwner(), LogTRTank, Log, TEXT("%s-%s: ResetActorToGround - ResetLocation=%s; ResetRotation=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *MyActor->GetActorLocation().ToCompactString(), *MyActor->GetActorRotation().ToCompactString());

	bIsFlippedOver = false;
}

#if ENABLE_VISUAL_LOG

void UFlippedOverCorrectionComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Flipped Over Correction Component");

	Category.Add(TEXT("Is Flipped Over"), LoggingUtils::GetBoolString(bIsFlippedOver));
	Category.Add(TEXT("Time To Correction"), !bIsFlippedOver ? TEXT("N/A") : FString::Printf(TEXT("%.2f"),
		FlippedOverTriggerDurationTime - (GetWorld()->GetTimeSeconds() - FlippedOverStartGameTimeSeconds)));

	Snapshot->Status.Add(Category);
}

#endif

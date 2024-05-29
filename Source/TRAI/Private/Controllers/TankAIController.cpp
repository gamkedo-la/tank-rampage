// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/TankAIController.h"

#include "Pawn/BaseTankPawn.h"
#include "Components/TankAimingComponent.h"
#include "Components/HealthComponent.h"

#include "Subsystems/TankAISharedStateSubsystem.h"\

#include "Navigation/CrowdFollowingComponent.h"

#include "Kismet/GameplayStatics.h" 
#include "TRAILogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Subsystems/TankEventsSubsystem.h"
#include "Item/ItemInventory.h"
#include "Item/ItemNames.h"
#include "Item/Weapon.h"

#include "Curves/CurveFloat.h"


ATankAIController::ATankAIController(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer) //.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
}


void ATankAIController::BeginPlay()
{
	Super::BeginPlay();

	// Do alternate checks for line of sight
	bLOSflag = true;
	bSkipExtraLOSChecks = false;
}

void ATankAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto Tank = GetControlledTank();
	if (!ensure(Tank))
	{
		return;
	}

	// Give AI a basic item
	Tank->GetItemInventory()->AddItemByName(TR::ItemNames::MainGunName);

	Tank->GetHealthComponent()->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
}

void ATankAIController::OnUnPossess()
{
	auto Tank = GetControlledTank();

	Super::OnUnPossess();

	if (Tank)
	{
		// Clear inventory when possessed by player as player start pawn is first possessed by AI and then possessed by player
		Tank->GetItemInventory()->Clear();
		Tank->GetHealthComponent()->OnHealthChanged.RemoveDynamic(this, &ThisClass::OnHealthChanged);
	}
}

void ATankAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ExecuteAI();
}

void ATankAIController::ExecuteAI()
{
	auto World = GetWorld();
	check(World);

	const auto AIContextOptional = GetAIContext();

	if (!AIContextOptional)
	{
		return;
	}

	const auto& AIContext = *AIContextOptional;

	if (AIContext.NowSeconds < StartDelayTime)
	{
		return;
	}

	if (!IsPlayerInRange(AIContext))
	{
		ResetState();
		return;
	}

	bHasLOS = HasLineOfSight(AIContext);
	bInInfaredRange = IsInInfaredRange(AIContext);

	// We are out of range but may have a reported position by another AI that is relevant to move toward
	if (!bInInfaredRange && !bHasLOS)
	{
		InitReportedPositionReactTimeIfApplicable(AIContext);

		if (ShouldMoveTowardReportedPosition(AIContext))
		{
			UE_VLOG_LOCATION(this, LogTRAI, VeryVerbose, AIContext.AISubsystem.LastPlayerSeenLocation, 25.0f, FColor::Yellow, TEXT("Last Known Player Loc"));
			MoveTowardPlayer(AIContext);
		}

		return;
	}

	// Directly perceived player either through direct line of sight or infared
	// Only aim and fire if have LOS

	if (!PassesDirectPerceptionReactionTimeDelay())
	{
		return;
	}

	UpdateSharedPerceptionState(AIContext);

	MoveTowardPlayer(AIContext);

	if (bHasLOS)
	{
		InitTargetErrorIfApplicable(AIContext);
		AimAtPlayerTank(AIContext);
		Fire(AIContext);
	}
}

void ATankAIController::InitReportedPositionReactTimeIfApplicable(const FTankAIContext& AIContext)
{
	if (ReportedPositionReactTime < 0)
	{
		ReportedPositionReactTime = FMath::FRandRange(
			AIContext.NowSeconds + ReportedPositionMinDelayTime,
			AIContext.NowSeconds + ReportedPositionMaxDelayTime);
	}
}

void ATankAIController::InitTargetErrorIfApplicable(const FTankAIContext& AIContext)
{
	if (AIContext.NowSeconds - TargetingErrorLastTime >= TargetingErrorResetTime)
	{
		InitTargetingError(AIContext);
		TargetingErrorLastTime = AIContext.NowSeconds;
	}
}

void ATankAIController::ResetState()
{
	FirstInRangeTime = TargetingErrorLastTime = ReportedPositionReactTime = -1;
	bHasLOS = bInInfaredRange = false;
	ShotsFired = 0;
}

void ATankAIController::UpdateSharedPerceptionState(const FTankAIContext& AIContext) const
{
	AIContext.AISubsystem.LastPlayerSeenLocation = AIContext.PlayerTank.GetActorLocation();
	AIContext.AISubsystem.LastPlayerSeenTime = AIContext.NowSeconds;

	UE_VLOG_LOCATION(this, LogTRAI, VeryVerbose, AIContext.AISubsystem.LastPlayerSeenLocation, 25.0f, FColor::Green, TEXT("Current Player Loc"));
}

bool ATankAIController::PassesDirectPerceptionReactionTimeDelay()
{
	auto World = GetWorld();
	check(World);

	const auto NowSeconds = World->GetTimeSeconds();

	if (FirstInRangeTime < 0)
	{
		FirstInRangeTime = NowSeconds;
	}

	return NowSeconds - FirstInRangeTime >= ReactionTime;
}

ABaseTankPawn* ATankAIController::GetControlledTank() const
{
	return Cast<ABaseTankPawn>(GetPawn());
}

ABaseTankPawn* ATankAIController::GetPlayerTank() const
{
	return Cast<ABaseTankPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void ATankAIController::Fire(const FTankAIContext& AIContext)
{
	auto& MyTank = AIContext.MyTank;

	auto TankAimingComponent = MyTank.GetTankAimingComponent();

	const auto FiringStatus = TankAimingComponent->GetTankFiringStatus();
	if (FiringStatus == ETankFiringStatus::Locked)
	{
		if (MyTank.Fire())
		{
			++ShotsFired;
		}
	}
}

void ATankAIController::AimAtPlayerTank(const FTankAIContext& AIContext)
{
	const auto& PlayerTank = AIContext.PlayerTank;
	auto& AITank = AIContext.MyTank;

	auto PlayerVelocity = PlayerTank.GetVelocity();

	auto ItemInventory = AITank.GetItemInventory();
	check(ItemInventory);

	auto ActiveWeapon = ItemInventory->GetActiveWeapon();
	if (!ActiveWeapon)
	{
		return;
	}

	FVector PredictiveOffset{ EForceInit::ForceInitToZero };
	if (ActiveWeapon->IsLaunchable() && PlayerVelocity.SizeSquared() > FMath::Square(PlayerVelocityPredictiveThreshold))
	{
		// Assuming only the "X" direction ignoring gravity
		const float PlayerDistanceMeters = AIContext.MyTank.GetDistanceTo(&PlayerTank);
		const float InterceptTime = PlayerDistanceMeters / ActiveWeapon->GetLaunchSpeed();

		PredictiveOffset = PlayerVelocity * InterceptTime;
	}

	const auto PredictedPosition = PlayerTank.GetActorLocation() + PredictiveOffset;
	const auto& AimTarget = PredictedPosition + TargetingError;

	FAimingData AimingData;
	AimingData.bAimTargetFound = true;
	AimingData.AimTargetLocation = AimTarget;
	
	AITank.AimAt(AimingData);
}

TOptional<ATankAIController::FTankAIContext> ATankAIController::GetAIContext() const
{
	auto PlayerTank = GetPlayerTank();
	auto ControlledTank = GetControlledTank();

	if (!PlayerTank || !ControlledTank)
	{
		return {};
	}

	auto World = GetWorld();
	check(World);

	auto AISubsystem = World->GetSubsystem<UTankAISharedStateSubsystem>();
	if (!ensure(AISubsystem))
	{
		return {};
	}

	return FTankAIContext
	{
		.MyTank = *ControlledTank,
		.PlayerTank = *PlayerTank,
		.AISubsystem = *AISubsystem,
		.NowSeconds = World->GetTimeSeconds(),
		.DistSqToPlayer = ControlledTank->GetSquaredDistanceTo(PlayerTank)
	};
}

bool ATankAIController::MoveTowardPlayer(const FTankAIContext& AIContext)
{
	const auto MinMoveDistance = MinMoveDistanceMeters * 100;

	const auto& AITank = AIContext.MyTank;
	const auto& TargetLocation = AIContext.AISubsystem.LastPlayerSeenLocation;

	const bool bShouldMove = !bHasLOS || FVector::DistSquared(AITank.GetActorLocation(), TargetLocation) > FMath::Square(MinMoveDistance);

	if (!bShouldMove)
	{
		return false;
	}

	MoveToLocation(TargetLocation, MinMoveDistanceMeters * 100, true, true, true);

	return true;
}

bool ATankAIController::ShouldMoveTowardReportedPosition(const FTankAIContext& AIContext) const
{
	auto& AISubsystem = AIContext.AISubsystem;

	if (AISubsystem.LastPlayerSeenTime < 0)
	{
		return false;
	}

	// Still waiting to process the information
	if (AIContext.NowSeconds > ReportedPositionReactTime)
	{
		return false;
	}

	return true;
}

bool ATankAIController::IsPlayerInRange(const FTankAIContext& AIContext) const
{
	return AIContext.DistSqToPlayer <= FMath::Square(MaxAggroDistanceMeters * 100);
}

void ATankAIController::InitTargetingError(const FTankAIContext& AIContext)
{
	if (!TargetingErrorByDistanceMeters)
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s-%s: InitTargetingError: No TargetingErrorByDistanceMeters curve set!"),
			*GetName(), *AIContext.MyTank.GetName());
		return;
	}

	if (!TargetingErrorMultiplierByShotsFired)
	{
		UE_VLOG_UELOG(this, LogTRAI, Warning, TEXT("%s-%s: InitTargetingError: No TargetingErrorMultiplierByShotsFired curve set - number of shots fired will not impact targeting"),
			*GetName(), *AIContext.MyTank.GetName());
	}

	const float PlayerDistanceMeters = AIContext.MyTank.GetDistanceTo(&AIContext.PlayerTank) / 100;
	float TargetingErrorMagnitudeMeters = TargetingErrorByDistanceMeters->FloatCurve.Eval(PlayerDistanceMeters);

	if (TargetingErrorMultiplierByShotsFired)
	{
		TargetingErrorMagnitudeMeters *= TargetingErrorMultiplierByShotsFired->FloatCurve.Eval(ShotsFired);
	}

	TargetingError = FMath::RandRange(-TargetingErrorMagnitudeMeters, TargetingErrorMagnitudeMeters) * 100 * FMath::VRand();
}

void ATankAIController::OnHealthChanged(UHealthComponent* HealthComponent, float PreviousHealthValue, float PreviousMaxHealthValue, AController* EventInstigator, AActor* ChangeCauser)
{
	check(HealthComponent);

	if (auto Tank = GetControlledTank(); Tank && HealthComponent->IsDead())
	{
		auto World = GetWorld();
		check(World);

		auto TankEventsSubsystem = World->GetSubsystem<UTankEventsSubsystem>();
		if (ensure(TankEventsSubsystem))
		{
			TankEventsSubsystem->OnTankDestroyed.Broadcast(Tank, EventInstigator, ChangeCauser);
		}

		Tank->DetachFromControllerPendingDestroy();
		Tank->Destroy();
	}
}

bool ATankAIController::HasLineOfSight(const FTankAIContext& AIContext) const
{
	// Do alternate checks so multiple line trace points are used for reference
	return LineOfSightTo(&AIContext.PlayerTank, FVector::ZeroVector, true);
}

bool ATankAIController::IsInInfaredRange(const FTankAIContext& AIContext) const
{
	return AIContext.DistSqToPlayer <= FMath::Square(MaxInfaredDistanceMeters * 100);
}

#if ENABLE_VISUAL_LOG
void ATankAIController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	auto& Category = Snapshot->Status[0];

	Category.Add(TEXT("Shots Fired"), FString::Printf(TEXT("%d"), ShotsFired));
	Category.Add(TEXT("HasLOS"), LoggingUtils::GetBoolString(bHasLOS));
	Category.Add(TEXT("InInfaredRange"), LoggingUtils::GetBoolString(bInInfaredRange));

	if (auto AISubsystem = GetWorld()->GetSubsystem<UTankAISharedStateSubsystem>(); AISubsystem)
	{
		if (AISubsystem->LastPlayerSeenTime >= 0)
		{
			Category.Add(TEXT("LastPlayerSeenTime"), FString::Printf(TEXT("%.1f"), AISubsystem->LastPlayerSeenTime));
			Category.Add(TEXT("LastPlayerPosition"),  AISubsystem->LastPlayerSeenLocation.ToCompactString());
		}
		else
		{
			Category.Add(TEXT("LastPlayerSeenTime"), TEXT("N/A"));
			Category.Add(TEXT("LastPlayerPosition"), TEXT("N/A"));
		}
	}

	Category.Add(TEXT("FirstInRangeTime"), FString::Printf(TEXT("%.1f"), FirstInRangeTime));
	Category.Add(TEXT("ReportedPositionReactTime"), FString::Printf(TEXT("%.1f"), ReportedPositionReactTime));
	Category.Add(TEXT("TargetingErrorLastTime"), FString::Printf(TEXT("%.1f"), TargetingErrorLastTime));
	Category.Add(TEXT("Targeting Error"), TargetingError.ToCompactString());
}

#endif

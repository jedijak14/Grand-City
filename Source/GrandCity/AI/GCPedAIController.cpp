#include "AI/GCPedAIController.h"
#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Core/GCPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "World/GCLocationActor.h"

AGCPedAIController::AGCPedAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGCPedAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Ped = Cast<AGCPedestrian>(InPawn);
	DecisionTimer = FMath::FRandRange(0.1f, 1.0f);
	if (Ped)
	{
		SetPedSpeed(Ped->WalkSpeed);
		PickWanderTarget();
	}
}

void AGCPedAIController::SetPedSpeed(float Speed)
{
	if (Ped)
	{
		Ped->GetCharacterMovement()->MaxWalkSpeed = Speed;
	}
}

void AGCPedAIController::PickWanderTarget()
{
	if (!Ped)
	{
		return;
	}
	FVector Dest;
	if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), Ped->GetActorLocation(), Dest, WanderRadius))
	{
		MoveToLocation(Dest, 90.0f);
	}
}

void AGCPedAIController::StartFlee(const FVector& DangerLocation)
{
	if (!Ped || Ped->bIsPolice || Ped->bDead)
	{
		return;
	}
	State = EGCPedState::Flee;
	FleeFromLocation = DangerLocation;
	FleeUntilTime = GetWorld()->GetTimeSeconds() + 7.0f;
	SetPedSpeed(Ped->RunSpeed);
	UpdateFlee();
}

void AGCPedAIController::ResumeFromPoliceStop()
{
    if (!Ped || Ped->bArrested || Ped->bDead)
    {
        return;
    }
    State = EGCPedState::Wander;
    SetPedSpeed(Ped->WalkSpeed);
    PickWanderTarget();
}

void AGCPedAIController::UpdateFlee()
{
	if (!Ped)
	{
		return;
	}
	const FVector AwayDir = (Ped->GetActorLocation() - FleeFromLocation).GetSafeNormal2D();
	const FVector Goal = Ped->GetActorLocation() + AwayDir * 2200.0f;
	FVector Dest;
	if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), Goal, Dest, 1200.0f))
	{
		MoveToLocation(Dest, 80.0f);
	}
	else
	{
		MoveToLocation(Goal, 80.0f);
	}
}

bool AGCPedAIController::HasLineOfSightTo(AActor* Target) const
{
	if (!Ped || !Target)
	{
		return false;
	}
	const FVector Start = Ped->GetActorLocation() + FVector(0, 0, 150.0f);
	const FVector End = Target->GetActorLocation() + FVector(0, 0, 110.0f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceLOS), false, Ped);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
		&& Hit.GetActor() != Target;
	return !bBlocked;
}

void AGCPedAIController::BustedPlayer(APawn* PlayerPawn)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}
    const AGCPlayerState* PlayerState = PC->GetPlayerState<AGCPlayerState>();
    const int32 ArrestStars = PlayerState ? FMath::Max(1, PlayerState->WantedStars) : 1;
	if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
	{
		PS->SpendMoney(300 * ArrestStars);
		PS->ClearWanted();
	}
	// Drop the player outside the nearest police station.
	if (AGCCharacter* PlayerChar = Cast<AGCCharacter>(PlayerPawn))
	{
		for (TActorIterator<AGCLocationActor> It(GetWorld()); It; ++It)
		{
			if (It->LocationType == EGCLocationType::PoliceStation)
			{
				PlayerChar->SetActorLocation(
					It->GetActorLocation() + It->GetActorForwardVector() * 350.0f + FVector(0, 0, 120.0f),
					false, nullptr, ETeleportType::TeleportPhysics);
				break;
			}
		}
        PlayerChar->BeginJailSentence(6.0f + 4.0f * ArrestStars);
	}
	ArrestProgress = 0.0f;
}

void AGCPedAIController::UpdatePolice(float DeltaSeconds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PlayerPawn || !PS || PS->WantedStars <= 0 || !Ped)
	{
		if (State == EGCPedState::PoliceChase)
		{
			State = EGCPedState::Wander;
			SetPedSpeed(Ped->WalkSpeed);
			PickWanderTarget();
		}
		return;
	}

	State = EGCPedState::PoliceChase;
	SetPedSpeed(Ped->RunSpeed);

	const float Dist = FVector::Distance(Ped->GetActorLocation(), PlayerPawn->GetActorLocation());

    // Police AI prioritizes an arrest over lethal force. The player can escape before
    // officers close in; gunfire requires a separate, explicit defensive-force system.
	MoveToActor(PlayerPawn, ArrestRange * 0.7f);

	// Arrest only works on an on-foot, slow player at 1-2 stars.
	AGCCharacter* PlayerChar = Cast<AGCCharacter>(PlayerPawn);
	if (PlayerChar && Dist <= ArrestRange && PlayerPawn->GetVelocity().Size() < 350.0f)
	{
		ArrestProgress += DeltaSeconds;
		if (ArrestProgress >= ArrestSeconds)
		{
			BustedPlayer(PlayerPawn);
		}
	}
	else
	{
		ArrestProgress = FMath::Max(0.0f, ArrestProgress - DeltaSeconds * 1.5f);
	}
}

bool AGCPedAIController::RecoverFromStuck(float DeltaSeconds)
{
    if (!Ped || Ped->bDead || Ped->bKnockedDown || Ped->bIsPolice || GetMoveStatus() != EPathFollowingStatus::Moving)
    {
        StuckDuration = 0.0f;
        StuckSampleTimer = 0.0f;
        LastStuckSampleLocation = Ped ? Ped->GetActorLocation() : FVector::ZeroVector;
        return false;
    }

    StuckSampleTimer += DeltaSeconds;
    StuckDuration += DeltaSeconds;
    const float SampleInterval = FMath::Max(0.1f, StuckSampleInterval);
    if (StuckSampleTimer < SampleInterval)
    {
        return false;
    }

    const FVector CurrentLocation = Ped->GetActorLocation();
    const float SampledDistance = FVector::Dist2D(CurrentLocation, LastStuckSampleLocation);
    const float HorizontalSpeed = Ped->GetVelocity().Size2D();
    LastStuckSampleLocation = CurrentLocation;
    StuckSampleTimer = 0.0f;
    if (HorizontalSpeed >= StuckSpeedThreshold || SampledDistance >= 45.0f)
    {
        StuckDuration = 0.0f;
        return false;
    }
    if (StuckDuration < FMath::Max(0.5f, StuckRecoverySeconds))
    {
        return false;
    }

    StopMovement();
    const FVector AwayDirection = (CurrentLocation - FleeFromLocation).GetSafeNormal2D();
    const FVector PreferredCenter = State == EGCPedState::Flee && !AwayDirection.IsNearlyZero()
        ? CurrentLocation + AwayDirection * 1300.0f : CurrentLocation;
    bool bFoundRecoveryPoint = false;
    for (int32 Attempt = 0; Attempt < 8; ++Attempt)
    {
        FVector Candidate = FVector::ZeroVector;
        const float SearchRadius = Attempt < 4 ? 900.0f : 1800.0f;
        if (!UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), PreferredCenter, Candidate, SearchRadius))
        {
            continue;
        }
        if (FVector::Dist2D(Candidate, CurrentLocation) < 450.0f)
        {
            continue;
        }
        MoveToLocation(Candidate, 80.0f, true, true, true, true, nullptr, true);
        bFoundRecoveryPoint = true;
        break;
    }

    StuckDuration = 0.0f;
    StuckSampleTimer = 0.0f;
    LastStuckSampleLocation = CurrentLocation;
    if (!bFoundRecoveryPoint && State == EGCPedState::Flee)
    {
        UpdateFlee();
    }
    return bFoundRecoveryPoint;
}

void AGCPedAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Ped || Ped->bDead || Ped->bKnockedDown)
	{
		return;
	}

    if (Ped->bStoppedForPolice && !Ped->bIsPolice)
    {
        return;
    }

	if (Ped->bIsPolice)
	{
		Ped->MaybeSayAmbient();
		UpdatePolice(DeltaSeconds);
		return;
	}
	Ped->MaybeSayAmbient();
    if (RecoverFromStuck(DeltaSeconds))
    {
        return;
    }

	if (State == EGCPedState::Flee)
	{
		DecisionTimer -= DeltaSeconds;
		if (GetWorld()->GetTimeSeconds() >= FleeUntilTime)
		{
			State = EGCPedState::Wander;
			SetPedSpeed(Ped->WalkSpeed);
			PickWanderTarget();
		}
		else if (DecisionTimer <= 0.0f)
		{
			DecisionTimer = 1.2f;
			UpdateFlee();
		}
		return;
	}

	// Wander: pick a new point when idle/arrived.
	DecisionTimer -= DeltaSeconds;
	if (DecisionTimer <= 0.0f)
	{
		DecisionTimer = FMath::FRandRange(2.0f, 5.0f);
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			PickWanderTarget();
		}
	}
}

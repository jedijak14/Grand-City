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
	if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
	{
		PS->SpendMoney(300 * FMath::Max(1, PS->WantedStars));
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
	const bool bLOS = HasLineOfSightTo(PlayerPawn);

	// Lethal force at 3+ stars when in range and sight.
	if (PS->WantedStars >= 3 && Dist < ShootRange && bLOS)
	{
		StopMovement();
		Ped->FireAtActor(PlayerPawn);
		ArrestProgress = 0.0f;
		return;
	}

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

void AGCPedAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Ped || Ped->bDead)
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

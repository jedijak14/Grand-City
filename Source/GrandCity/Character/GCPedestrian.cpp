#include "Character/GCPedestrian.h"
#include "AI/GCPedAIController.h"
#include "Character/GCCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/GCHealthComponent.h"
#include "Components/GCVoiceLineComponent.h"
#include "Components/GCWeaponComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicle/GCVehiclePawn.h"

AGCPedestrian::AGCPedestrian()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AGCPedAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);

	Health = CreateDefaultSubobject<UGCHealthComponent>(TEXT("Health"));
	Weapon = CreateDefaultSubobject<UGCWeaponComponent>(TEXT("Weapon"));
	Weapon->Damage = 9.0f; // police sidearm - dangerous in numbers, survivable solo
	Weapon->FireRateRPM = 240.0f;
	Weapon->SpreadDegrees = 2.6f;

	Voice = CreateDefaultSubobject<UGCVoiceLineComponent>(TEXT("Voice"));

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
	Move->NavAgentProps.AgentRadius = 38.0f;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 420.0f;
}

void AGCPedestrian::BeginPlay()
{
	Super::BeginPlay();
	if (WalletCash <= 0)
	{
		WalletCash = FMath::RandRange(35, 220);
	}
	if (!bIsPolice && FMath::FRand() < 0.12f)
	{
		bHoldingCoffee = true;
	}
	if (Health)
	{
		Health->OnDeath.AddDynamic(this, &AGCPedestrian::OnDeath);
	}
}

void AGCPedestrian::ShowSubtitleToNearbyPlayer(const FString& Text) const
{
	if (Text.IsEmpty())
	{
		return;
	}
	if (AGCCharacter* PlayerChar = Cast<AGCCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		if (FVector::Distance(PlayerChar->GetActorLocation(), GetActorLocation()) < 1100.0f)
		{
			PlayerChar->ShowSubtitle(Text, 4.0f);
		}
	}
}

void AGCPedestrian::SayLine(FName LineID)
{
	if (bDead || !Voice)
	{
		return;
	}
	FString Subtitle;
	if (Voice->PlayLine(LineID, Subtitle))
	{
		ShowSubtitleToNearbyPlayer(Subtitle);
	}
}

void AGCPedestrian::MaybeSayAmbient()
{
	if (bDead || !Voice || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextAmbientVoiceTime)
	{
		return;
	}
	NextAmbientVoiceTime = Now + FMath::FRandRange(10.0f, 22.0f);
	if (FMath::FRand() > 0.35f)
	{
		return;
	}
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn || FVector::Distance(PlayerPawn->GetActorLocation(), GetActorLocation()) > 1100.0f)
	{
		return;
	}
	FString Subtitle;
	if (Voice->PlayRandomWithPrefix(bIsPolice ? TEXT("Police") : TEXT("Ambient"), Subtitle))
	{
		ShowSubtitleToNearbyPlayer(Subtitle);
	}
}

void AGCPedestrian::FleeFrom(const FVector& DangerLocation)
{
	if (AGCPedAIController* AI = Cast<AGCPedAIController>(GetController()))
	{
		AI->StartFlee(DangerLocation);
	}
}

void AGCPedestrian::FireAtActor(AActor* Target)
{
	if (!Target || bDead || !bIsPolice)
	{
		return;
	}
	const FVector Start = GetActorLocation() + FVector(0, 0, 145.0f) + GetActorForwardVector() * 40.0f;
	const FVector TargetPoint = Target->GetActorLocation() + FVector(0, 0, 110.0f);
	const FVector Dir = (TargetPoint - Start).GetSafeNormal();
	SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
	FVector HitPoint;
	Weapon->FireOnce(Start, Dir, true, HitPoint);
}

bool AGCPedestrian::CanWitnessCrimeAt(const FVector& IncidentLocation) const
{
	if (bDead || !GetWorld() || FVector::Distance(GetActorLocation(), IncidentLocation) > 2400.0f)
	{
		return false;
	}
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 145.0f);
	const FVector End = IncidentLocation + FVector(0.0f, 0.0f, 85.0f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CivilianWitnessLOS), false, this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
		|| FVector::DistSquared(Hit.ImpactPoint, End) < FMath::Square(130.0f);
}

void AGCPedestrian::ReactToWitnessedCrime(const FVector& IncidentLocation, bool bScream)
{
	if (bDead)
	{
		return;
	}
	if (bScream && !bIsPolice && PanicScreamSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PanicScreamSound, GetActorLocation());
	}
	FString Subtitle;
	const FName VoiceLine = bIsPolice ? FName(TEXT("PoliceCrimeWitness")) : FName(TEXT("CivilianScream"));
	if (!Voice || !Voice->PlayLine(VoiceLine, Subtitle))
	{
		Subtitle = bIsPolice ? TEXT("Police! Stop! Call it in!") : TEXT("Help! Somebody call the police!");
	}
	ShowSubtitleToNearbyPlayer(Subtitle);
	FleeFrom(IncidentLocation);
}

bool AGCPedestrian::AttemptPickpocket(AGCCharacter* Thief)
{
	if (!Thief || bDead || bIsPolice || bHasBeenPickpocketed)
	{
		return false;
	}
	const float Distance = FVector::Distance(Thief->GetActorLocation(), GetActorLocation());
	if (Distance > 190.0f)
	{
		Thief->ShowSubtitle(TEXT("Get closer to someone to try pickpocketing."), 3.0f);
		return false;
	}

	const FVector TowardThief = (Thief->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const bool bFacingThief = FVector::DotProduct(GetActorForwardVector(), TowardThief) > 0.25f;
	const bool bObviousApproach = !Thief->bIsCrouched || Thief->GetVelocity().Size2D() > 220.0f || bFacingThief;
	const float DetectionChance = bObviousApproach ? 0.78f : 0.12f;
	if (FMath::FRand() < DetectionChance)
	{
		bHasBeenPickpocketed = true;
		ReactToWitnessedCrime(Thief->GetActorLocation(), true);
		if (AGCPlayerState* PS = Thief->GetPlayerState<AGCPlayerState>())
		{
			PS->AddWantedStars(1);
		}
		Thief->ShowSubtitle(TEXT("They caught your hand in their pocket! The victim raised the alarm."), 5.0f);
		return false;
	}

	bHasBeenPickpocketed = true;
	const int32 StolenCash = FMath::Max(0, WalletCash);
	if (AGCPlayerState* PS = Thief->GetPlayerState<AGCPlayerState>())
	{
		PS->AddMoney(StolenCash);
	}
	Thief->ShowSubtitle(FString::Printf(TEXT("Quietly lifted $%d. Stay low and move carefully."), StolenCash), 4.0f);
	return true;
}

void AGCPedestrian::KnockDown(float DurationSeconds)
{
	if (bDead || bKnockedDown)
	{
		return;
	}
	bKnockedDown = true;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();

	if (bHoldingCoffee)
	{
		bHoldingCoffee = false;
		if (CoffeeSpillFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CoffeeSpillFX,
				GetActorLocation() + FVector(0.0f, 0.0f, 75.0f));
		}
		if (CoffeeSpillSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CoffeeSpillSound, GetActorLocation());
		}
	}
	GetWorldTimerManager().SetTimer(KnockdownTimer, this, &AGCPedestrian::RecoverFromKnockdown,
		FMath::Clamp(DurationSeconds, 0.8f, 6.0f), false);
}

void AGCPedestrian::RecoverFromKnockdown()
{
	if (bDead)
	{
		return;
	}
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bKnockedDown = false;
}

void AGCPedestrian::OnDeath(AActor* Killer)
{
	bDead = true;
	StartRagdoll();

	// A homicide creates wanted stars only when an NPC can actually see the incident.
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	const bool bKilledByPlayer = Killer && Killer == PlayerPawn;
	if (bKilledByPlayer)
	{
		TArray<AActor*> Nearby;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGCPedestrian::StaticClass(), Nearby);
		bool bWitnessed = false;
		bool bScreamPlayed = false;
		int32 WitnessWantedStars = 1;
		for (AActor* Actor : Nearby)
		{
			AGCPedestrian* Other = Cast<AGCPedestrian>(Actor);
			if (!Other || Other == this || !Other->CanWitnessCrimeAt(GetActorLocation()))
			{
				continue;
			}
			bWitnessed = true;
			WitnessWantedStars = FMath::Max(WitnessWantedStars, Other->bIsPolice ? 2 : 1);
			const bool bPlayScream = !Other->bIsPolice && !bScreamPlayed;
			Other->ReactToWitnessedCrime(GetActorLocation(), bPlayScream);
			bScreamPlayed |= bPlayScream;
		}
		if (bWitnessed)
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
				{
					PS->AddWantedStars(WitnessWantedStars);
				}
			}
		}
	}

	SetLifeSpan(25.0f); // body cleanup; spawner will replace the population
}

void AGCPedestrian::StartRagdoll()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	if (AController* C = GetController())
	{
		C->UnPossess();
	}
}

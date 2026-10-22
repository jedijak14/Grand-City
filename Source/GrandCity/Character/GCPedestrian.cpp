#include "Character/GCPedestrian.h"
#include "AI/GCPedAIController.h"
#include "Character/GCCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/GCHealthComponent.h"
#include "Components/GCVoiceLineComponent.h"
#include "Components/GCWeaponComponent.h"
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
}

void AGCPedestrian::BeginPlay()
{
	Super::BeginPlay();
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

void AGCPedestrian::OnDeath(AActor* Killer)
{
	bDead = true;
	StartRagdoll();

	// If the player did it, that is a witnessed crime.
	const bool bKilledByPlayer = Killer && (Killer->IsA<AGCCharacter>() || Killer->IsA<AGCVehiclePawn>());
	if (bKilledByPlayer)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
			{
				PS->AddWantedStars(bIsPolice ? 2 : 1);
			}
		}
	}

	// Nearby civilians panic.
	TArray<AActor*> Nearby;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGCPedestrian::StaticClass(), Nearby);
	for (AActor* Actor : Nearby)
	{
		AGCPedestrian* Other = Cast<AGCPedestrian>(Actor);
		if (Other && Other != this && !Other->bIsPolice
			&& FVector::Distance(Other->GetActorLocation(), GetActorLocation()) < 1600.0f)
		{
			Other->FleeFrom(GetActorLocation());
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

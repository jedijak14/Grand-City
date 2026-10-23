#include "World/GCWorldSpawner.h"
#include "AI/GCVehicleAIController.h"
#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Core/GCGameUserSettings.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Vehicle/GCVehiclePawn.h"

AGCWorldSpawner::AGCWorldSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	PedestrianClass = AGCPedestrian::StaticClass();
	TrafficVehicleClass = AGCVehiclePawn::StaticClass();
	PoliceVehicleClass = AGCVehiclePawn::StaticClass();
}

void AGCWorldSpawner::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(ManageTimer, this, &AGCWorldSpawner::ManagePopulation,
		ManageIntervalSeconds, true, 1.0f);
}

bool AGCWorldSpawner::FindGroundSpawn(const FVector& AroundLocation, FVector& OutLocation) const
{
	const float Angle = FMath::FRandRange(0.0f, 360.0f);
	const float Radius = FMath::FRandRange(SpawnMinRadius, SpawnMaxRadius);
	const FVector Candidate = AroundLocation + FVector(
		FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
		FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius, 0.0f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpawnerGround), false);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Candidate + FVector(0, 0, 2500.0f),
		Candidate - FVector(0, 0, 2500.0f), ECC_Visibility, Params))
	{
		if (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Water")))
		{
			return false;
		}
		OutLocation = Hit.ImpactPoint + FVector(0, 0, 160.0f);
		return true;
	}
	return false;
}

void AGCWorldSpawner::CleanupList(TArray<AActor*>& List, const FVector& PlayerLocation)
{
	for (int32 i = List.Num() - 1; i >= 0; --i)
	{
		AActor* Actor = List[i];
		if (!IsValid(Actor))
		{
			List.RemoveAt(i);
			continue;
		}
		if (FVector::Distance(Actor->GetActorLocation(), PlayerLocation) > DespawnRadius)
		{
			// Never despawn a vehicle the player is sitting in.
			if (AGCVehiclePawn* Vehicle = Cast<AGCVehiclePawn>(Actor))
			{
				if (Vehicle->GetDriver())
				{
					continue;
				}
			}
			Actor->Destroy();
			List.RemoveAt(i);
		}
	}
}

void AGCWorldSpawner::SpawnPedestrian(const FVector& PlayerLocation)
{
	FVector Loc;
	if (!PedestrianClass || !FindGroundSpawn(PlayerLocation, Loc))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	if (AGCPedestrian* Ped = GetWorld()->SpawnActor<AGCPedestrian>(PedestrianClass, Loc,
		FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Params))
	{
		SpawnedPeds.Add(Ped);
	}
}

void AGCWorldSpawner::SpawnPolicePed(const FVector& PlayerLocation)
{
	FVector Loc;
	if (!PedestrianClass || !FindGroundSpawn(PlayerLocation, Loc))
	{
		return;
	}
	// Deferred spawn so the police flag is set before BeginPlay/AI possess.
	AGCPedestrian* Ped = GetWorld()->SpawnActorDeferred<AGCPedestrian>(PedestrianClass,
		FTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Loc));
	if (Ped)
	{
		Ped->bIsPolice = true;
		Ped->FinishSpawning(FTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Loc));
		SpawnedPolicePeds.Add(Ped);
	}
}

void AGCWorldSpawner::SpawnTrafficCar(const FVector& PlayerLocation)
{
	FVector Loc;
	if (!TrafficVehicleClass || !FindGroundSpawn(PlayerLocation, Loc))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	AGCVehiclePawn* Car = GetWorld()->SpawnActor<AGCVehiclePawn>(TrafficVehicleClass, Loc + FVector(0, 0, 80.0f),
		FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Params);
	if (!Car)
	{
		return;
	}
    // Low-frequency, actionable impaired-driving incidents in ambient traffic.
    const float ImpairmentRoll = FMath::FRand();
    if (ImpairmentRoll < 0.025f)
    {
        Car->bDriverImpaired = true;
        Car->bDriverSlurredSpeech = true;
        Car->bDriverUnsteadyGait = true;
    }
    else if (ImpairmentRoll < 0.045f)
    {
        Car->bDriverDrugImpaired = true;
        Car->bDriverUnsteadyGait = true;
    }
    if (FMath::FRand() < 0.025f) Car->bTailLightsFunctional = false;
    if (FMath::FRand() < 0.02f) Car->bSmokingExhaust = true;
	if (AGCVehicleAIController* AI = GetWorld()->SpawnActor<AGCVehicleAIController>())
	{
		AI->Possess(Car);
		AI->SetMode(EGCVehicleAIMode::Traffic);
	}
	SpawnedTraffic.Add(Car);
}

void AGCWorldSpawner::SpawnPoliceCar(const FVector& PlayerLocation)
{
	FVector Loc;
	if (!PoliceVehicleClass || !FindGroundSpawn(PlayerLocation, Loc))
	{
		return;
	}
	AGCVehiclePawn* Car = GetWorld()->SpawnActorDeferred<AGCVehiclePawn>(PoliceVehicleClass,
		FTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Loc + FVector(0, 0, 80.0f)));
	if (!Car)
	{
		return;
	}
	Car->bIsPoliceVehicle = true;
	Car->FinishSpawning(FTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Loc + FVector(0, 0, 80.0f)));
	if (AGCVehicleAIController* AI = GetWorld()->SpawnActor<AGCVehicleAIController>())
	{
		AI->Possess(Car);
		AI->SetMode(EGCVehicleAIMode::PolicePursuit);
	}
	SpawnedPoliceCars.Add(Car);
}

void AGCWorldSpawner::ManagePopulation()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}
	const FVector PlayerLoc = PlayerPawn->GetActorLocation();

	int32 DesiredPeds = 24;
	int32 DesiredTraffic = 14;
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		DesiredPeds = Settings->PedestrianDensity;
		DesiredTraffic = Settings->TrafficDensity;
	}

	int32 Wanted = 0;
	if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
	{
		Wanted = PS->WantedStars;
	}
	const int32 DesiredPoliceCars = Wanted;                       // 1 star = 1 unit ... 5 = 5 units
	const bool bPlayerOnFoot = PlayerPawn->IsA<AGCCharacter>();
	const int32 DesiredPolicePeds = (bPlayerOnFoot && Wanted >= 2) ? (Wanted - 1) : 0;

	CleanupList(SpawnedPeds, PlayerLoc);
	CleanupList(SpawnedTraffic, PlayerLoc);
	CleanupList(SpawnedPoliceCars, PlayerLoc);
	CleanupList(SpawnedPolicePeds, PlayerLoc);

	// Spawn at most one of each kind per tick to avoid hitches.
	if (SpawnedPeds.Num() < DesiredPeds) { SpawnPedestrian(PlayerLoc); }
	if (SpawnedTraffic.Num() < DesiredTraffic) { SpawnTrafficCar(PlayerLoc); }
	if (SpawnedPoliceCars.Num() < DesiredPoliceCars) { SpawnPoliceCar(PlayerLoc); }
	if (SpawnedPolicePeds.Num() < DesiredPolicePeds) { SpawnPolicePed(PlayerLoc); }
}

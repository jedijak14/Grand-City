#include "Core/GCCrimeSystemSubsystem.h"

#include "Character/GCPedestrian.h"
#include "Core/GCPlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

int32 UGCCrimeSystemSubsystem::GetCrimeStars(EGCCrimeType CrimeType) const
{
    switch (CrimeType)
    {
    case EGCCrimeType::Homicide: return 2;
    case EGCCrimeType::StoreRobbery: return 1;
    case EGCCrimeType::Pickpocket: return 1;
    case EGCCrimeType::VehicleTheft: return 1;
    case EGCCrimeType::IllegalPropertyClaim: return 2;
    case EGCCrimeType::UnlawfulGunfire: return 1;
    default: return 1;
    }
}

bool UGCCrimeSystemSubsystem::CanBeHeardAt(const FVector& ListenerLocation, const FVector& IncidentLocation,
    float NoiseRadius, AActor* ListenerActor, AActor* SuspectActor) const
{
    if (!GetWorld() || NoiseRadius <= 0.0f)
    {
        return false;
    }
    const float Distance = FVector::Dist2D(ListenerLocation, IncidentLocation);
    if (Distance > NoiseRadius)
    {
        return false;
    }
    const FVector Start = ListenerLocation + FVector(0.0f, 0.0f, 140.0f);
    const FVector End = IncidentLocation + FVector(0.0f, 0.0f, 80.0f);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CrimeAudibility), false);
    if (IsValid(ListenerActor))
    {
        Params.AddIgnoredActor(ListenerActor);
    }
    if (IsValid(SuspectActor))
    {
        Params.AddIgnoredActor(SuspectActor);
    }
    const bool bOccluded = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
        && FVector::DistSquared(Hit.ImpactPoint, End) > FMath::Square(140.0f);
    const float OcclusionFactor = bOccluded ? 0.42f : 1.0f;
    return Distance <= NoiseRadius * OcclusionFactor;
}

bool UGCCrimeSystemSubsystem::ReportCrime(APawn* Suspect, const FVector& IncidentLocation,
    EGCCrimeType CrimeType, float NoiseRadius)
{
    UWorld* World = GetWorld();
    if (!World || !Suspect)
    {
        return false;
    }
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (Suspect != PlayerPawn && (!Suspect->GetController() || !Suspect->GetController()->IsPlayerController()))
    {
        return false;
    }
    AGCPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!PlayerState)
    {
        return false;
    }

    const int32 CrimeStars = GetCrimeStars(CrimeType);
    bool bAnyWitness = false;
    bool bPoliceWitness = false;
    AGCPedestrian* CivilianReporter = nullptr;
    int32 ImmediateStars = 0;
    for (TActorIterator<AGCPedestrian> It(World); It; ++It)
    {
        AGCPedestrian* Witness = *It;
        if (!IsValid(Witness) || Witness == Suspect || Witness->bDead)
        {
            continue;
        }
        const bool bSeesIncident = Witness->CanWitnessCrimeAt(IncidentLocation);
        const bool bHearsIncident = !bSeesIncident && CanBeHeardAt(
            Witness->GetActorLocation(), IncidentLocation, NoiseRadius, Witness, Suspect);
        if (!bSeesIncident && !bHearsIncident)
        {
            continue;
        }

        bAnyWitness = true;
        const bool bScream = !Witness->bIsPolice && (bSeesIncident || CrimeType == EGCCrimeType::UnlawfulGunfire);
        Witness->ReactToWitnessedCrime(IncidentLocation, bScream);
        if (Witness->bIsPolice)
        {
            bPoliceWitness = true;
            ImmediateStars = FMath::Max(ImmediateStars, FMath::Max(CrimeStars, 2));
        }
        else if (!CivilianReporter)
        {
            CivilianReporter = Witness;
        }
    }

    if (bPoliceWitness && ImmediateStars > 0)
    {
        PlayerState->AddWantedStars(ImmediateStars);
    }
    else if (IsValid(CivilianReporter))
    {
        CivilianReporter->QueuePoliceReport(CrimeStars, FMath::FRandRange(3.0f, 7.0f));
    }
    return bAnyWitness;
}

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GCCrimeSystemSubsystem.generated.h"

class AActor;
class APawn;

UENUM(BlueprintType)
enum class EGCCrimeType : uint8
{
    Homicide,
    StoreRobbery,
    Pickpocket,
    VehicleTheft,
    IllegalPropertyClaim,
    UnlawfulGunfire
};

/** Central witness/noise gate: unlawful actions create wanted stars only when observed or heard. */
UCLASS()
class GRANDCITY_API UGCCrimeSystemSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Gridlock|Crime")
    bool ReportCrime(APawn* Suspect, const FVector& IncidentLocation, EGCCrimeType CrimeType, float NoiseRadius = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Gridlock|Crime")
    bool CanBeHeardAt(const FVector& ListenerLocation, const FVector& IncidentLocation, float NoiseRadius,
        AActor* ListenerActor = nullptr, AActor* SuspectActor = nullptr) const;

private:
    int32 GetCrimeStars(EGCCrimeType CrimeType) const;
};

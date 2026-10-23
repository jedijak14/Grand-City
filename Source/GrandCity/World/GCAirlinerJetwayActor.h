#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCAirlinerJetwayActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class AGCAirlinerPawn;

/** Greybox passenger boarding bridge; entering its aircraft-end trigger boards a valid ticket holder. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCAirlinerJetwayActor : public AActor
{
    GENERATED_BODY()

public:
    AGCAirlinerJetwayActor();
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jetway")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jetway")
    TObjectPtr<UStaticMeshComponent> Walkway;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jetway")
    TObjectPtr<UStaticMeshComponent> LeftRailing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jetway")
    TObjectPtr<UStaticMeshComponent> RightRailing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jetway")
    TObjectPtr<UStaticMeshComponent> Canopy;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jetway")
    TObjectPtr<UBoxComponent> AircraftDoorTrigger;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Jetway")
    TObjectPtr<AGCAirlinerPawn> LinkedAirliner = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetway", meta = (ClampMin = "500.0", ClampMax = "2500.0"))
    float BridgeLengthCm = 1100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetway", meta = (ClampMin = "160.0", ClampMax = "450.0"))
    float BridgeWidthCm = 220.0f;

protected:
    UFUNCTION()
    void OnAircraftDoorOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

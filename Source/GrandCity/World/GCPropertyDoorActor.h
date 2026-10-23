#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCPropertyDoorActor.generated.h"

class AGCCharacter;
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

/** Keyed residential access point. Owned and stolen deeds share the same explicit access permission. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCPropertyDoorActor : public AActor
{
    GENERATED_BODY()

public:
    AGCPropertyDoorActor();
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Property Access")
    TObjectPtr<UBoxComponent> Trigger = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Property Access")
    TObjectPtr<UStaticMeshComponent> DoorSign = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property Access")
    FString PropertyID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property Access")
    FString DoorName = TEXT("Residence");

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Property Access")
    TObjectPtr<AActor> DestinationMarker = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property Access")
    bool bEnteringInterior = true;

    UFUNCTION(BlueprintCallable, Category = "Property Access")
    bool UseDoor(AGCCharacter* Character);

protected:
    UFUNCTION()
    void OnTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
};

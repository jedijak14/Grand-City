#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCDroppedItemActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/** Small physics prop dropped by pedestrians; collision and mass are configured conservatively. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCDroppedItemActor : public AActor
{
    GENERATED_BODY()

public:
    AGCDroppedItemActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dropped Item")
    TObjectPtr<UStaticMeshComponent> ItemMesh = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Dropped Item")
    void InitializeDroppedItem(UStaticMesh* Mesh, const FVector& InitialImpulse);
};

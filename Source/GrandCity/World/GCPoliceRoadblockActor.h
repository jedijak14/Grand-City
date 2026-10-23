#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCPoliceRoadblockActor.generated.h"

class UStaticMeshComponent;

/** Portable deployable barrier set. Engine cube meshes are used as a readable greybox until art is assigned. */
UCLASS(Blueprintable)
class GRANDCITY_API AGCPoliceRoadblockActor : public AActor
{
    GENERATED_BODY()

public:
    AGCPoliceRoadblockActor();
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Police|Roadblock")
    TObjectPtr<UStaticMeshComponent> LeftBarrier = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Police|Roadblock")
    TObjectPtr<UStaticMeshComponent> CenterBarrier = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Police|Roadblock")
    TObjectPtr<UStaticMeshComponent> RightBarrier = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Police|Roadblock")
    FString DeployedByCitizenID;
};

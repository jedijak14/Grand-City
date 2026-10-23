#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GCClothPhysicsLibrary.generated.h"

class AActor;
class USkeletalMeshComponent;

/** Runtime controls for UE Chaos Cloth on skeletal meshes that already contain Clothing Data. */
UCLASS()
class GRANDCITY_API UGCClothPhysicsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Enables/suspends built-in SkeletalMesh Clothing Simulation on every skeletal mesh component
     * owned by Actor. This does not manufacture Clothing Data; the mesh section must be authored
     * as Chaos Cloth in the editor first.
     */
    UFUNCTION(BlueprintCallable, Category = "Gridlock|Cloth")
    static void ApplyClothSimulation(AActor* Actor, bool bEnableSimulation, float MaxDistanceScale = 1.0f);
};

#include "Components/GCClothPhysicsLibrary.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UGCClothPhysicsLibrary::ApplyClothSimulation(AActor* Actor, bool bEnableSimulation, float MaxDistanceScale)
{
    if (!IsValid(Actor))
    {
        return;
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshes, true);
    const float SafeDistanceScale = bEnableSimulation ? FMath::Clamp(MaxDistanceScale, 0.0f, 10.0f) : 0.0f;

    for (USkeletalMeshComponent* Mesh : SkeletalMeshes)
    {
        if (!IsValid(Mesh) || !Mesh->GetSkeletalMeshAsset())
        {
            continue;
        }

        // bDisableClothSimulation and blend/distance scales are the component's runtime controls.
        // Resume/Suspend also recreate the live simulation state when the global setting changes.
        Mesh->bDisableClothSimulation = !bEnableSimulation;
        Mesh->SetClothMaxDistanceScale(SafeDistanceScale);
        Mesh->SetClothBlendWeight(bEnableSimulation ? 1.0f : 0.0f);
        if (bEnableSimulation)
        {
            Mesh->ResumeClothingSimulation();
        }
        else
        {
            Mesh->SuspendClothingSimulation();
        }
    }
}

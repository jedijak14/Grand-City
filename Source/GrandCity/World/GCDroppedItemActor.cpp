#include "World/GCDroppedItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AGCDroppedItemActor::AGCDroppedItemActor()
{
    PrimaryActorTick.bCanEverTick = false;
    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    RootComponent = ItemMesh;
    ItemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    ItemMesh->SetSimulatePhysics(true);
    ItemMesh->SetNotifyRigidBodyCollision(true);
    ItemMesh->SetMassOverrideInKg(NAME_None, 0.8f, true);
}

void AGCDroppedItemActor::InitializeDroppedItem(UStaticMesh* Mesh, const FVector& InitialImpulse)
{
    if (!Mesh)
    {
        Destroy();
        return;
    }
    ItemMesh->SetStaticMesh(Mesh);
    SetLifeSpan(35.0f);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ItemMesh->SetSimulatePhysics(true);
    ItemMesh->WakeAllRigidBodies();
    ItemMesh->AddImpulse(InitialImpulse, NAME_None, true);
}

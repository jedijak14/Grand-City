#include "World/GCPoliceRoadblockActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGCPoliceRoadblockActor::AGCPoliceRoadblockActor()
{
    PrimaryActorTick.bCanEverTick = false;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto CreateBarrier = [this, &CubeMesh](const TCHAR* Name, float LocalY)
    {
        UStaticMeshComponent* Barrier = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Barrier->SetupAttachment(RootComponent);
        Barrier->SetRelativeLocation(FVector(0.0f, LocalY, 60.0f));
        Barrier->SetRelativeScale3D(FVector(2.8f, 0.18f, 0.55f));
        Barrier->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Barrier->SetGenerateOverlapEvents(false);
        if (CubeMesh.Succeeded())
        {
            Barrier->SetStaticMesh(CubeMesh.Object);
        }
        return Barrier;
    };
    LeftBarrier = CreateBarrier(TEXT("LeftBarrier"), -320.0f);
    CenterBarrier = CreateBarrier(TEXT("CenterBarrier"), 0.0f);
    RightBarrier = CreateBarrier(TEXT("RightBarrier"), 320.0f);
}

void AGCPoliceRoadblockActor::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(900.0f);
}

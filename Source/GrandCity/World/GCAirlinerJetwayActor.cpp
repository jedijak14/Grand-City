#include "World/GCAirlinerJetwayActor.h"

#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Vehicle/GCAirlinerPawn.h"
#include "UObject/ConstructorHelpers.h"

AGCAirlinerJetwayActor::AGCAirlinerJetwayActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Walkway = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Walkway"));
    Walkway->SetupAttachment(Root);
    Walkway->SetCollisionProfileName(TEXT("BlockAll"));

    LeftRailing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRailing"));
    LeftRailing->SetupAttachment(Root);
    LeftRailing->SetCollisionProfileName(TEXT("BlockAll"));

    RightRailing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRailing"));
    RightRailing->SetupAttachment(Root);
    RightRailing->SetCollisionProfileName(TEXT("BlockAll"));

    Canopy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Canopy"));
    Canopy->SetupAttachment(Root);
    Canopy->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    AircraftDoorTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("AircraftDoorTrigger"));
    AircraftDoorTrigger->SetupAttachment(Root);
    AircraftDoorTrigger->SetBoxExtent(FVector(120.0f, 90.0f, 130.0f));
    AircraftDoorTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AircraftDoorTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    AircraftDoorTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    AircraftDoorTrigger->SetGenerateOverlapEvents(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Walkway->SetStaticMesh(CubeMesh.Object);
        LeftRailing->SetStaticMesh(CubeMesh.Object);
        RightRailing->SetStaticMesh(CubeMesh.Object);
        Canopy->SetStaticMesh(CubeMesh.Object);
    }
}

void AGCAirlinerJetwayActor::BeginPlay()
{
    Super::BeginPlay();
    const float SafeLength = FMath::Clamp(BridgeLengthCm, 500.0f, 2500.0f);
    const float SafeWidth = FMath::Clamp(BridgeWidthCm, 160.0f, 450.0f);
    Walkway->SetRelativeLocation(FVector(0.0f, -SafeLength * 0.5f, 5.0f));
    Walkway->SetRelativeScale3D(FVector(SafeWidth / 200.0f, SafeLength / 200.0f, 0.08f));
    LeftRailing->SetRelativeLocation(FVector(-SafeWidth * 0.46f, -SafeLength * 0.5f, 85.0f));
    LeftRailing->SetRelativeScale3D(FVector(0.06f, SafeLength / 200.0f, 0.8f));
    RightRailing->SetRelativeLocation(FVector(SafeWidth * 0.46f, -SafeLength * 0.5f, 85.0f));
    RightRailing->SetRelativeScale3D(FVector(0.06f, SafeLength / 200.0f, 0.8f));
    Canopy->SetRelativeLocation(FVector(0.0f, -SafeLength * 0.5f, 240.0f));
    Canopy->SetRelativeScale3D(FVector(SafeWidth / 200.0f, SafeLength / 200.0f, 0.05f));
    AircraftDoorTrigger->OnComponentBeginOverlap.AddDynamic(this, &AGCAirlinerJetwayActor::OnAircraftDoorOverlap);
}

void AGCAirlinerJetwayActor::OnAircraftDoorOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AGCCharacter* Character = Cast<AGCCharacter>(OtherActor);
    if (!Character || !IsValid(LinkedAirliner))
    {
        return;
    }
    if (!LinkedAirliner->BoardPassenger(Character))
    {
        Character->ShowSubtitle(TEXT("Boarding bridge: use a checked-in pass for this flight; walk through to board."), 4.0f);
    }
}

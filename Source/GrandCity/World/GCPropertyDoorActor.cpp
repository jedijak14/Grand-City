#include "World/GCPropertyDoorActor.h"

#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AGCPropertyDoorActor::AGCPropertyDoorActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("AccessTrigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(120.0f, 100.0f, 150.0f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    DoorSign = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorSign"));
    DoorSign->SetupAttachment(Trigger);
    DoorSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGCPropertyDoorActor::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGCPropertyDoorActor::OnTriggerBegin);
    Trigger->OnComponentEndOverlap.AddDynamic(this, &AGCPropertyDoorActor::OnTriggerEnd);
}

void AGCPropertyDoorActor::OnTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AGCCharacter* Character = Cast<AGCCharacter>(OtherActor))
    {
        Character->SetCurrentPropertyDoor(this);
        const AGCPlayerState* State = Character->GetPlayerState<AGCPlayerState>();
        const bool bHasAccess = State && State->OwnsProperty(PropertyID);
        Character->ShowSubtitle(bHasAccess
            ? FString::Printf(TEXT("%s — press E to %s"), *DoorName, bEnteringInterior ? TEXT("enter") : TEXT("exit"))
            : FString::Printf(TEXT("%s is locked. A valid deed is required."), *DoorName));
    }
}

void AGCPropertyDoorActor::OnTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
    if (AGCCharacter* Character = Cast<AGCCharacter>(OtherActor))
    {
        if (Character->GetCurrentPropertyDoor() == this)
        {
            Character->SetCurrentPropertyDoor(nullptr);
        }
    }
}

bool AGCPropertyDoorActor::UseDoor(AGCCharacter* Character)
{
    if (!IsValid(Character))
    {
        return false;
    }
    const AGCPlayerState* State = Character->GetPlayerState<AGCPlayerState>();
    if (!State || !State->OwnsProperty(PropertyID))
    {
        Character->ShowSubtitle(FString::Printf(TEXT("Access denied — %s requires a valid deed."), *DoorName));
        return false;
    }
    if (!IsValid(DestinationMarker))
    {
        Character->ShowSubtitle(TEXT("This property entrance is not connected to an interior yet."));
        return false;
    }
    Character->SetActorLocation(DestinationMarker->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
    Character->ShowSubtitle(bEnteringInterior ? TEXT("You entered your residence.") : TEXT("You exited your residence."));
    return true;
}

#include "World/GCBusStopActor.h"

#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "Vehicle/GCBusPawn.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

AGCBusStopActor::AGCBusStopActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BusStopTrigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(260.0f, 260.0f, 180.0f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	StopSign = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StopSign"));
	StopSign->SetupAttachment(Trigger);
	StopSign->SetRelativeLocation(FVector(0.0f, 150.0f, 140.0f));
	StopSign->SetRelativeScale3D(FVector(0.35f, 0.12f, 0.8f));
	StopSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGCBusStopActor::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGCBusStopActor::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AGCBusStopActor::OnTriggerEnd);
}

void AGCBusStopActor::CycleDestination()
{
	DestinationStops.RemoveAll([](AGCBusStopActor* Stop) { return !IsValid(Stop); });
	if (DestinationStops.IsEmpty())
	{
		return;
	}
	SelectedDestinationIndex = (SelectedDestinationIndex + 1) % DestinationStops.Num();
	if (AGCCharacter* Passenger = CurrentPassenger.Get())
	{
		AGCBusStopActor* Destination = DestinationStops[SelectedDestinationIndex];
		Passenger->ShowSubtitle(FString::Printf(TEXT("Bus route: %s to %s — $%d. Press E to board."),
			*StopName, *Destination->StopName, Fare), 4.0f);
	}
}

void AGCBusStopActor::Ride(AGCCharacter* Passenger)
{
	DestinationStops.RemoveAll([](AGCBusStopActor* Stop) { return !IsValid(Stop); });
	if (!Passenger || !DestinationStops.IsValidIndex(SelectedDestinationIndex))
	{
		if (Passenger)
		{
			Passenger->ShowSubtitle(TEXT("No bus route is available from this stop."), 4.0f);
		}
		return;
	}
	AGCBusStopActor* Destination = DestinationStops[SelectedDestinationIndex];
	for (TActorIterator<AGCBusPawn> It(GetWorld()); It; ++It)
	{
		AGCBusPawn* Bus = *It;
		if (!IsValid(Bus) || FVector::Dist2D(Bus->GetActorLocation(), GetActorLocation()) > 900.0f)
		{
			continue;
		}
		if (Bus->RouteStops.Contains(this) && Bus->RouteStops.Contains(Destination)
			&& Bus->BoardPassenger(Passenger, Destination, Fare))
		{
			return;
		}
	}
	Passenger->ShowSubtitle(FString::Printf(TEXT("The %s route to %s is not at this stop yet."), *StopName, *Destination->StopName), 4.0f);
}

void AGCBusStopActor::OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AGCCharacter* Passenger = Cast<AGCCharacter>(Other))
	{
		CurrentPassenger = Passenger;
		Passenger->SetCurrentBusStop(this);
		const FString DestinationName = DestinationStops.IsValidIndex(SelectedDestinationIndex)
			&& IsValid(DestinationStops[SelectedDestinationIndex]) ? DestinationStops[SelectedDestinationIndex]->StopName : TEXT("choose a route");
        float BestArrivalSeconds = TNumericLimits<float>::Max();
        for (TActorIterator<AGCBusPawn> BusIt(GetWorld()); BusIt; ++BusIt)
        {
            if (AGCBusPawn* Bus = *BusIt)
            {
                const float ETA = Bus->GetEstimatedArrivalSeconds(this);
                if (ETA >= 0.0f)
                {
                    BestArrivalSeconds = FMath::Min(BestArrivalSeconds, ETA);
                }
            }
        }
        const FString ArrivalText = BestArrivalSeconds < TNumericLimits<float>::Max()
            ? (BestArrivalSeconds < 60.0f
                ? FString::Printf(TEXT("next bus ~%ds"), FMath::CeilToInt(BestArrivalSeconds))
                : FString::Printf(TEXT("next bus ~%d min"), FMath::CeilToInt(BestArrivalSeconds / 60.0f)))
            : TEXT("no active bus estimate");
        Passenger->ShowSubtitle(FString::Printf(TEXT("%s to %s — %s. Tab routes; E board ($%d)."),
            *StopName, *DestinationName, *ArrivalText, Fare), 6.0f);
	}
}

void AGCBusStopActor::OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGCCharacter* Passenger = Cast<AGCCharacter>(Other))
	{
		if (Passenger->CurrentBusStop == this)
		{
			Passenger->SetCurrentBusStop(nullptr);
		}
		if (CurrentPassenger.Get() == Passenger)
		{
			CurrentPassenger.Reset();
		}
	}
}

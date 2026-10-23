#include "Vehicle/GCBusPawn.h"

#include "AI/GCVehicleAIController.h"
#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "World/GCBusStopActor.h"

AGCBusPawn::AGCBusPawn()
{
	CollisionBody->SetBoxExtent(FVector(380.0f, 135.0f, 145.0f));
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -70.0f));
	BodyMesh->SetRelativeScale3D(FVector(3.8f, 1.25f, 1.45f));
	MassKg = 7800.0f;
	EngineMaxTorqueNm = 1300.0f;
	Wheels.Empty();
	for (const FVector& Wheel : { FVector(250.0f, 100.0f, 10.0f), FVector(-180.0f, 100.0f, 10.0f),
		FVector(250.0f, -100.0f, 10.0f), FVector(-180.0f, -100.0f, 10.0f) })
	{
		FGCWheelSetup Setup;
		Setup.LocalOffset = Wheel;
		Setup.Radius = 47.0f;
		Setup.Width = 32.0f;
		Setup.bSteering = Wheel.X > 0.0f;
		Wheels.Add(Setup);
	}
	DriverSeat->SetRelativeLocation(FVector(230.0f, -55.0f, 0.0f));
	ExitPoint->SetRelativeLocation(FVector(0.0f, -185.0f, -50.0f));

	PassengerSeat = CreateDefaultSubobject<USceneComponent>(TEXT("PassengerSeat"));
	PassengerSeat->SetupAttachment(CollisionBody);
	PassengerSeat->SetRelativeLocation(FVector(-25.0f, 20.0f, 90.0f));
}

void AGCBusPawn::BeginPlay()
{
	Super::BeginPlay();
    RouteStops.RemoveAll([](AGCBusStopActor* Stop) { return !IsValid(Stop); });
    CurrentRouteTargetIndex = 0;
	TArray<FVector> Points;
	for (AGCBusStopActor* Stop : RouteStops)
	{
		if (IsValid(Stop))
		{
			Points.Add(Stop->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f));
		}
	}
	if (Points.Num() >= 2)
	{
		if (AGCVehicleAIController* RouteAI = GetWorld()->SpawnActor<AGCVehicleAIController>())
		{
            RouteAI->TrafficCruiseKmh = RouteCruiseKmh;
			RouteAI->SetBusRoute(Points, StopDwellSeconds);
			RouteAI->Possess(this);
		}
	}
}

bool AGCBusPawn::BoardPassenger(AGCCharacter* Character, AGCBusStopActor* DestinationStop, int32 Fare)
{
	if (!Character || !IsValid(DestinationStop) || PassengerCharacter || GetSpeedKmh() > 10.0f
		|| FVector::Distance(GetActorLocation(), Character->GetActorLocation()) > 500.0f)
	{
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PlayerState = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PlayerState || !PlayerState->SpendMoney(Fare))
	{
		Character->ShowSubtitle(FString::Printf(TEXT("Bus fare is $%d; your selected tender has insufficient funds."), Fare), 4.0f);
		return false;
	}
	PassengerCharacter = Character;
	PassengerDestination = DestinationStop;
	Character->BeginVehicleTransition();
	Character->SetCurrentBus(this);
	Character->CompleteEnterVehicle(this, PassengerSeat);
	Character->ShowSubtitle(FString::Printf(TEXT("Boarded Gridlock Transit. Next stop: %s. Press E to get off."),
		*DestinationStop->StopName), 4.0f);
	return true;
}

float AGCBusPawn::GetEstimatedArrivalSeconds(AGCBusStopActor* Stop) const
{
    if (!IsValid(Stop) || RouteStops.Num() < 2 || !RouteStops.Contains(Stop))
    {
        return -1.0f;
    }
    const float CruiseSpeedCmPerSecond = FMath::Max(100.0f, RouteCruiseKmh * (100000.0f / 3600.0f));
    FVector SegmentStart = GetActorLocation();
    float TravelSeconds = 0.0f;
    int32 Index = FMath::Clamp(CurrentRouteTargetIndex, 0, RouteStops.Num() - 1);
    for (int32 Step = 0; Step < RouteStops.Num(); ++Step)
    {
        const int32 StopIndex = (Index + Step) % RouteStops.Num();
        const AGCBusStopActor* RouteStop = RouteStops[StopIndex];
        if (!IsValid(RouteStop))
        {
            return -1.0f;
        }
        const FVector StopLocation = RouteStop->GetActorLocation();
        TravelSeconds += FVector::Dist2D(SegmentStart, StopLocation) / CruiseSpeedCmPerSecond;
        if (RouteStop == Stop)
        {
            return TravelSeconds;
        }
        SegmentStart = StopLocation;
        TravelSeconds += StopDwellSeconds;
    }
    return -1.0f;
}

void AGCBusPawn::NotifyRouteStopReached(int32 StopIndex)
{
    if (RouteStops.IsValidIndex(StopIndex))
    {
        CurrentRouteTargetIndex = (StopIndex + 1) % RouteStops.Num();
    }
	if (!PassengerCharacter || !IsValid(PassengerDestination) || !RouteStops.IsValidIndex(StopIndex))
	{
		return;
	}
	if (RouteStops[StopIndex] == PassengerDestination)
	{
		PassengerCharacter->ShowSubtitle(FString::Printf(TEXT("Arriving at %s."), *PassengerDestination->StopName), 3.0f);
		DisembarkPassenger();
	}
}

void AGCBusPawn::DisembarkPassenger()
{
	if (!PassengerCharacter)
	{
		return;
	}
	AGCCharacter* Character = PassengerCharacter;
	PassengerCharacter = nullptr;
	PassengerDestination = nullptr;
	Character->SetCurrentBus(nullptr);
	Character->CompleteExitVehicle(ExitPoint->GetComponentLocation() + FVector(0.0f, 0.0f, 55.0f));
}

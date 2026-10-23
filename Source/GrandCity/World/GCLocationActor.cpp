#include "World/GCLocationActor.h"
#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Components/BoxComponent.h"
#include "Components/GCHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicle/GCBoatPawn.h"
#include "World/GCComputerTerminalActor.h"
#include "World/GCElevatorActor.h"

AGCLocationActor::AGCLocationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(450.0f, 450.0f, 260.0f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(Trigger);

	SignMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignMesh"));
	SignMesh->SetupAttachment(Trigger);
	SignMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 420.0f));
	SignMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RentalBoatClass = AGCBoatPawn::StaticClass();
}

void AGCLocationActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyDefaultTexts();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGCLocationActor::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AGCLocationActor::OnTriggerEnd);
}

void AGCLocationActor::ApplyDefaultTexts()
{
	auto SetIfEmpty = [](FText& Field, const TCHAR* Value)
	{
		if (Field.IsEmpty())
		{
			Field = FText::FromString(Value);
		}
	};
	switch (LocationType)
	{
	case EGCLocationType::Hospital:
		SetIfEmpty(DisplayName, TEXT("Central Hospital"));
		SetIfEmpty(InteractionText, TEXT("Get treated ($200)"));
		break;
	case EGCLocationType::Shop:
		SetIfEmpty(DisplayName, TEXT("24/7 Store"));
		SetIfEmpty(InteractionText, TEXT("Rob the till"));
		break;
	case EGCLocationType::PoliceStation:
		SetIfEmpty(DisplayName, TEXT("Police Station"));
		SetIfEmpty(InteractionText, TEXT("Surrender & pay fines"));
		break;
	case EGCLocationType::Airport:
		SetIfEmpty(DisplayName, TEXT("International Airport"));
		SetIfEmpty(InteractionText, TEXT("Book flight ($350)"));
		break;
	case EGCLocationType::Marina:
		SetIfEmpty(DisplayName, TEXT("Marina"));
		SetIfEmpty(InteractionText, TEXT("Rent a boat ($150)"));
		break;
	case EGCLocationType::SkyscraperElevator:
		SetIfEmpty(DisplayName, TEXT("Tower Elevator"));
		SetIfEmpty(InteractionText, TEXT("Call elevator to the linked landing"));
		break;
	}
}

void AGCLocationActor::OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		Character->SetCurrentLocation(this);
		const FString Prompt = FString::Printf(TEXT("%s — Press F: %s"),
			*DisplayName.ToString(), *InteractionText.ToString());
		Character->ShowSubtitle(Prompt, 4.0f);
	}
}

void AGCLocationActor::OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		if (Character->CurrentLocation == this)
		{
			Character->SetCurrentLocation(nullptr);
		}
	}
}

void AGCLocationActor::Interact(AGCCharacter* Character)
{
	if (!Character)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;

	switch (LocationType)
	{
	case EGCLocationType::Hospital:
		if (!Character->Health)
		{
			Character->ShowSubtitle(TEXT("Medical services are temporarily unavailable."), 4.0f);
			break;
		}
		if (Character->Health->Health >= Character->Health->MaxHealth)
		{
			Character->ShowSubtitle(TEXT("You're already at full health."), 4.0f);
			break;
		}
		if (PS && !PS->SpendMoney(HealCost))
		{
			Character->ShowSubtitle(FString::Printf(TEXT("Treatment costs $%d. You don't have enough cash."), HealCost), 4.0f);
			break;
		}
		Character->Health->Heal(Character->Health->MaxHealth);
		Character->ShowSubtitle(TEXT("Treatment complete. You're back to full health."), 4.0f);
		break;

	case EGCLocationType::Shop:
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now < NextRobTime)
		{
			Character->ShowSubtitle(TEXT("The till is empty and the heat is on. Try again later."), 4.0f);
			return;
		}
		NextRobTime = Now + RobCooldownSeconds;
		if (PS)
		{
			PS->AddMoney(FMath::RandRange(RobRewardMin, RobRewardMax));
			PS->AddWantedStars(2);
		}
		// Civilians nearby run for it.
		TArray<AActor*> Nearby;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGCPedestrian::StaticClass(), Nearby);
		for (AActor* Actor : Nearby)
		{
			if (AGCPedestrian* Ped = Cast<AGCPedestrian>(Actor))
			{
				if (!Ped->bIsPolice && FVector::Distance(Ped->GetActorLocation(), GetActorLocation()) < 2000.0f)
				{
					Ped->FleeFrom(Character->GetActorLocation());
				}
			}
		}
		break;
	}

	case EGCLocationType::PoliceStation:
		if (!PS || PS->WantedStars <= 0)
		{
			Character->ShowSubtitle(TEXT("You're not wanted. The station has nothing to process."), 4.0f);
			break;
		}
		{
			const int32 Fine = 300 * PS->WantedStars;
			if (!PS->SpendMoney(Fine))
			{
				Character->ShowSubtitle(FString::Printf(TEXT("Your fine is $%d. You don't have enough cash."), Fine), 4.0f);
				break;
			}
			PS->ClearWanted();
			Character->ShowSubtitle(FString::Printf(TEXT("Fine paid: $%d. You're clear."), Fine), 4.0f);
		}
		break;

	case EGCLocationType::Airport:
	{
		// If this level has proper check-in computers, the full ticket flow lives there.
		bool bHasComputers = false;
		for (TActorIterator<AGCComputerTerminalActor> It(GetWorld()); It; ++It)
		{
			bHasComputers = true;
			break;
		}
		if (bHasComputers)
		{
			Character->ShowSubtitle(PS && PS->HasActiveTicket() && PS->ActiveTicket.Status == EGCTicketStatus::CheckedIn
				? TEXT("You're checked in — head to the aircraft at the gate and press F to board.")
				: TEXT("Use the check-in computers in the terminal to book and check in."), 5.0f);
			break;
		}
		// Legacy quick-flight fallback for maps without terminals/airliners.
		if (!FlightDestination)
		{
			Character->ShowSubtitle(TEXT("No destination is linked to this airport."), 4.0f);
			break;
		}
		if (!PS || !PS->SpendMoney(FlightCost))
		{
			Character->ShowSubtitle(FString::Printf(TEXT("The ticket costs $%d. You don't have enough cash."), FlightCost), 4.0f);
			break;
		}
		// Booked passenger flight (cutaway). Piloting your own plane is the other way there.
		if (PC)
		{
			PC->ClientSetCameraFade(true, FColor::Black, FVector2D(0.0f, 1.0f), 0.4f, true);
		}
		const FVector DestLoc = FlightDestination->GetActorLocation()
			+ FlightDestination->GetActorForwardVector() * 500.0f + FVector(0, 0, 140.0f);
		Character->SetActorLocation(DestLoc, false, nullptr, ETeleportType::TeleportPhysics);
		if (PC)
		{
			PC->ClientSetCameraFade(false, FColor::Black, FVector2D(1.0f, 0.0f), 1.2f, false);
		}
		break;
	}

	case EGCLocationType::Marina:
		if (!RentalBoatClass)
		{
			Character->ShowSubtitle(TEXT("Boat rentals are unavailable here."), 4.0f);
			break;
		}
		if (!PS || !PS->SpendMoney(BoatRentalCost))
		{
			Character->ShowSubtitle(FString::Printf(TEXT("Boat rental costs $%d. You don't have enough cash."), BoatRentalCost), 4.0f);
			break;
		}
		{
			const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 900.0f + FVector(0, 0, 60.0f);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			AGCBoatPawn* Boat = GetWorld()->SpawnActor<AGCBoatPawn>(RentalBoatClass, SpawnLoc, GetActorRotation(), Params);
			if (!Boat)
			{
				PS->AddMoney(BoatRentalCost);
				Character->ShowSubtitle(TEXT("The rental could not be spawned. Your money was refunded."), 4.0f);
			}
			else
			{
				Character->ShowSubtitle(TEXT("Boat rental paid. Your boat is ready at the marina."), 4.0f);
			}
		}
		break;

	case EGCLocationType::SkyscraperElevator:
		if (ElevatorActor && TeleportDestination)
		{
			if (!ElevatorActor->CallFromLanding(this, TeleportDestination, Character))
			{
				Character->ShowSubtitle(TEXT("Elevator is moving or the destination is unavailable."), 3.0f);
			}
		}
		else if (TeleportDestination)
		{
			// Compatibility for older maps; link an AGCElevatorActor to get animated transport.
			Character->SetActorLocation(TeleportDestination->GetActorLocation() + FVector(0, 0, 120.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			Character->ShowSubtitle(TEXT("This elevator landing has no linked destination."), 4.0f);
		}
		break;
	}
}

#include "World/GCComputerTerminalActor.h"
#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/GCVoiceLineComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"

AGCComputerTerminalActor::AGCComputerTerminalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(260.0f, 260.0f, 220.0f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));

	DeskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeskMesh"));
	DeskMesh->SetupAttachment(Trigger);
	DeskMesh->SetRelativeLocation(FVector(-80.0f, 0.0f, -90.0f));

	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(Trigger);
	ScreenMesh->SetRelativeLocation(FVector(-60.0f, 0.0f, 30.0f));
	ScreenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StaffSeat = CreateDefaultSubobject<USceneComponent>(TEXT("StaffSeat"));
	StaffSeat->SetupAttachment(Trigger);
	StaffSeat->SetRelativeLocation(FVector(-190.0f, 0.0f, -100.0f));

	Voice = CreateDefaultSubobject<UGCVoiceLineComponent>(TEXT("Voice"));
}

void AGCComputerTerminalActor::BeginPlay()
{
	Super::BeginPlay();
	if (Offers.Num() == 0)
	{
		GenerateDefaultOffers();
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGCComputerTerminalActor::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AGCComputerTerminalActor::OnTriggerEnd);
}

void AGCComputerTerminalActor::GenerateDefaultOffers()
{
	const float Hours[3] = { 8.5f, 12.75f, 18.25f };
	for (int32 i = 0; i < 3; ++i)
	{
		FGCFlightOffer Offer;
		Offer.Airline = AirlineName;
		Offer.FlightNumber = FString::Printf(TEXT("GC%d"), 101 + i * 2);
		Offer.ToCode = DestinationCode;
		Offer.ToCity = DestinationCity;
		Offer.DepartureHour = Hours[i];
		Offer.EconomyPrice = 290 + i * 60;
		Offer.DurationMinutes = 55;
		Offers.Add(Offer);
	}
}

FString AGCComputerTerminalActor::ClassName(EGCSeatClass SeatClass)
{
	switch (SeatClass)
	{
	case EGCSeatClass::Business: return TEXT("Business");
	case EGCSeatClass::First: return TEXT("First");
	default: return TEXT("Economy");
	}
}

FString AGCComputerTerminalActor::HourToText(float Hour)
{
	const int32 H = FMath::FloorToInt(Hour) % 24;
	const int32 M = FMath::FloorToInt((Hour - FMath::FloorToFloat(Hour)) * 60.0f);
	return FString::Printf(TEXT("%02d:%02d"), H, M);
}

void AGCComputerTerminalActor::CycleOffer()
{
	if (Offers.Num() > 0)
	{
		SelectedOfferIndex = (SelectedOfferIndex + 1) % Offers.Num();
	}
}

void AGCComputerTerminalActor::CycleClass()
{
	SelectedClass = SelectedClass == EGCSeatClass::Economy ? EGCSeatClass::Business
		: (SelectedClass == EGCSeatClass::Business ? EGCSeatClass::First : EGCSeatClass::Economy);
}

int32 AGCComputerTerminalActor::PriceForSelected() const
{
	if (!Offers.IsValidIndex(SelectedOfferIndex))
	{
		return 0;
	}
	const float Multiplier = SelectedClass == EGCSeatClass::First ? 3.8f
		: (SelectedClass == EGCSeatClass::Business ? 2.2f : 1.0f);
	return FMath::RoundToInt(Offers[SelectedOfferIndex].EconomyPrice * Multiplier);
}

void AGCComputerTerminalActor::Speak(AGCCharacter* Character, FName LineID, const FString& Fallback)
{
	FString Subtitle;
	bool bSpoken = false;
	if (Voice)
	{
		bSpoken = Voice->PlayLine(LineID, Subtitle);
	}
	if (Character)
	{
		Character->ShowSubtitle(bSpoken && !Subtitle.IsEmpty() ? Subtitle : Fallback, 5.0f);
	}
}

FString AGCComputerTerminalActor::GetScreenText(AGCCharacter* Character) const
{
	FString Text = FString::Printf(TEXT("== %s  (%s) ==\nSelf check-in & booking\n\n"), *AirportCity, *AirportCode);

	for (int32 i = 0; i < Offers.Num(); ++i)
	{
		const FGCFlightOffer& O = Offers[i];
		Text += FString::Printf(TEXT("%s %s  ->  %s %s   dep %s   from $%d\n"),
			i == SelectedOfferIndex ? TEXT(">") : TEXT(" "),
			*O.FlightNumber, *O.ToCode, *O.ToCity, *HourToText(O.DepartureHour), O.EconomyPrice);
	}

	Text += FString::Printf(TEXT("\nClass: %s    Ticket price: $%d\n"), *ClassName(SelectedClass), PriceForSelected());

	if (Character)
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
			{
				Text += FString::Printf(TEXT("Wallet: $%d\n"), PS->Money);
				if (PS->HasActiveTicket())
				{
					const FGCFlightTicket& T = PS->ActiveTicket;
					Text += FString::Printf(TEXT("\n--- YOUR TICKET %s ---\n%s  %s -> %s  dep %s\nStatus: %s   Seat: %s   Gate: %s\n"),
						*T.TicketID, *T.FlightNumber, *T.FromCode, *T.ToCode, *HourToText(T.DepartureHour),
						T.Status == EGCTicketStatus::Booked ? TEXT("BOOKED - press E to CHECK IN")
							: (T.Status == EGCTicketStatus::CheckedIn ? TEXT("CHECKED IN - go to the airliner and press F") : TEXT("USED")),
						T.Seat.IsEmpty() ? TEXT("(at check-in)") : *T.Seat, *T.Gate);
				}
			}
		}
	}

	Text += TEXT("\n[Tab] flight   [B] class   [E] book / check in");
	return Text;
}

bool AGCComputerTerminalActor::BookFlight(AGCCharacter* Character, FGCFlightTicket& OutTicket)
{
	OutTicket = FGCFlightTicket();
	if (!Character || !Offers.IsValidIndex(SelectedOfferIndex))
	{
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PS)
	{
		return false;
	}
	const int32 Price = PriceForSelected();
	if (!PS->SpendMoney(Price))
	{
		Speak(Character, TEXT("PaymentDeclined"), TEXT("I'm sorry, that payment didn't go through. The fare is on the screen if you'd like a different flight or class."));
		return false;
	}

	const FGCFlightOffer& O = Offers[SelectedOfferIndex];
	OutTicket.TicketID = FString::Printf(TEXT("GC-%06d"), FMath::RandRange(100000, 999999));
	OutTicket.Airline = O.Airline;
	OutTicket.FlightNumber = O.FlightNumber;
	OutTicket.FromCode = AirportCode;
	OutTicket.ToCode = O.ToCode;
	OutTicket.FromCity = AirportCity;
	OutTicket.ToCity = O.ToCity;
	OutTicket.Gate = FString::Printf(TEXT("A%d"), FMath::RandRange(1, 8));
	OutTicket.SeatClass = SelectedClass;
	OutTicket.BoardingGroup = SelectedClass == EGCSeatClass::First ? 1 : (SelectedClass == EGCSeatClass::Business ? 2 : 4);
	OutTicket.Status = EGCTicketStatus::Booked;
	OutTicket.PricePaid = Price;
	OutTicket.DepartureHour = O.DepartureHour;

	PS->SetTicket(OutTicket);
	Speak(Character, TEXT("BookingConfirmed"), TEXT("You're booked! Shall I check you in now? Press E and I'll print your boarding pass with your seat and gate."));
	return true;
}

bool AGCComputerTerminalActor::CheckIn(AGCCharacter* Character)
{
	if (!Character)
	{
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PS || !PS->HasActiveTicket())
	{
		return false;
	}

	FGCFlightTicket Ticket = PS->ActiveTicket;
	if (Ticket.FromCode != AirportCode)
	{
		Speak(Character, TEXT("WrongAirport"), TEXT("That ticket departs from a different airport — check the airport code on your booking."));
		return false;
	}
	if (Ticket.Status != EGCTicketStatus::Booked)
	{
		Speak(Character, TEXT("AlreadyCheckedIn"), TEXT("You're already checked in. Head through to the gate and board the aircraft when you're ready."));
		return false;
	}

	const int32 Row = Ticket.SeatClass == EGCSeatClass::First ? FMath::RandRange(1, 4)
		: (Ticket.SeatClass == EGCSeatClass::Business ? FMath::RandRange(5, 10) : FMath::RandRange(11, 38));
	const TCHAR Letters[] = TEXT("ABCDEF");
	Ticket.Seat = FString::Printf(TEXT("%d%c"), Row, Letters[FMath::RandRange(0, 5)]);
	Ticket.Status = EGCTicketStatus::CheckedIn;
	PS->SetTicket(Ticket);

	Speak(Character, TEXT("CheckInComplete"),
		FString::Printf(TEXT("All checked in! Seat %s, gate %s. Have a great flight — boarding at the aircraft whenever you're ready."), *Ticket.Seat, *Ticket.Gate));
	return true;
}

void AGCComputerTerminalActor::UseComputer(AGCCharacter* Character)
{
	if (!Character)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}

	if (!PS->HasActiveTicket())
	{
		FGCFlightTicket Ticket;
		BookFlight(Character, Ticket);
	}
	else if (PS->ActiveTicket.Status == EGCTicketStatus::Booked)
	{
		CheckIn(Character);
	}
	else if (PS->ActiveTicket.Status == EGCTicketStatus::CheckedIn)
	{
		Speak(Character, TEXT("GateAnnouncement"),
			FString::Printf(TEXT("Flight %s to %s is boarding at gate %s. Walk to the aircraft and press F to board."),
				*PS->ActiveTicket.FlightNumber, *PS->ActiveTicket.ToCity, *PS->ActiveTicket.Gate));
	}
}

void AGCComputerTerminalActor::OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		Character->SetCurrentComputer(this);
		Speak(Character, TEXT("CheckInGreeting"),
			TEXT("Hello, and welcome to Grand City Air! I can book your flight and check you in right here — the screen shows today's departures."));
	}
}

void AGCComputerTerminalActor::OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		if (Character->CurrentComputer == this)
		{
			Character->SetCurrentComputer(nullptr);
		}
	}
}

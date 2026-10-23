#pragma once

#include "CoreMinimal.h"
#include "GCFlightTypes.generated.h"

UENUM(BlueprintType)
enum class EGCSeatClass : uint8
{
	Economy,
	Business,
	First
};

UENUM(BlueprintType)
enum class EGCTicketStatus : uint8
{
	None,
	Booked,
	CheckedIn,
	Boarded,
	Flown,
	Cancelled
};

UENUM(BlueprintType)
enum class EGCEntertainmentMode : uint8
{
	Off,
	FlightMap,
	Movies,
	Music,
	Games
};

/** One purchasable flight shown on an airport computer. */
USTRUCT(BlueprintType)
struct FGCFlightOffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Airline = TEXT("Coastline Airways");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FlightNumber = TEXT("CW 410");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ToCode = TEXT("NRT");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ToCity = TEXT("North Island");

	/** Hour of day 0-24 (matches the day/night clock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DepartureHour = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EconomyPrice = 350;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DurationMinutes = 55;
};

/** A booked ticket in the player's wallet (AGCPlayerState). */
USTRUCT(BlueprintType)
struct FGCFlightTicket
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString TicketID;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString PassengerName = TEXT("PLAYER / GUEST");

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString PassengerID;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString Airline = TEXT("Coastline Airways");

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString FlightNumber = TEXT("CW 410");

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString FromCode;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString ToCode;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString FromCity;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString ToCity;

	/** Assigned at check-in, e.g. "14A". */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString Seat;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FString Gate = TEXT("A3");

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	int32 BoardingGroup = 2;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	EGCSeatClass SeatClass = EGCSeatClass::Economy;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	EGCTicketStatus Status = EGCTicketStatus::None;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	int32 PricePaid = 0;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	float DepartureHour = 12.0f;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString ArrivalTime;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    int32 DurationMinutes = 55;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime FlightDateUtc;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString BoardingTime;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString Terminal = TEXT("1");

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString BookingReference;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString BarcodePayload;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    bool bPaperCopyPrinted = false;
};

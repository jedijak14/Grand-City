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
	FString Airline = TEXT("Gridline Air");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FlightNumber = TEXT("GC100");

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TicketID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PassengerName = TEXT("PLAYER / GUEST");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Airline = TEXT("Gridline Air");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FlightNumber = TEXT("GC100");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FromCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ToCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FromCity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ToCity;

	/** Assigned at check-in, e.g. "14A". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Seat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Gate = TEXT("A3");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BoardingGroup = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGCSeatClass SeatClass = EGCSeatClass::Economy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGCTicketStatus Status = EGCTicketStatus::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PricePaid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DepartureHour = 12.0f;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "World/GCFlightTypes.h"
#include "GCAirlinerPawn.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UGCVoiceLineComponent;
class AGCCharacter;
class AGCPedestrian;

UENUM(BlueprintType)
enum class EGCFlightPhase : uint8
{
	Parked,
	Boarding,
	TakeoffRoll,
	Climb,
	Cruise,
	Descent,
	FinalApproach,
	Landed,
	Arrived
};

/**
 * Passenger airliner for booked flights (AGCPlanePawn is the one YOU fly).
 *
 * Board with a checked-in ticket (airport computer) -> welcome + safety
 * announcements -> takeoff roll, climb, cruise, descent, landing along a
 * smooth Bezier route to DestinationActor's airport -> arrival announcement.
 * In your seat: seatback entertainment [1] map [2] movies [3] music [4] games,
 * [V] cabin/external camera. NPC passengers fill the other seats.
 * Crew voice lines live on the Voice component (see Docs/VOICE_LINES.md).
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCAirlinerPawn : public APawn
{
	GENERATED_BODY()

public:
	AGCAirlinerPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	USceneComponent* Root;

	/** Query-only volume so the on-foot interaction scan finds the aircraft. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	UBoxComponent* InteractionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	UStaticMeshComponent* FuselageMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	USceneComponent* PlayerSeat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	TArray<USceneComponent*> PassengerSeats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	USceneComponent* ExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner|Camera")
	UCameraComponent* CabinCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner|Camera")
	USpringArmComponent* ExternalArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner|Camera")
	UCameraComponent* ExternalCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Airliner")
	UGCVoiceLineComponent* Voice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Route")
	FString FromCode = TEXT("GCI");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Route")
	FString ToCode = TEXT("NIS");

	/** Airport/location actor at the destination (its position/rotation = touchdown point/heading). */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Airliner|Route")
	AActor* DestinationActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Route")
	float FlightDurationSeconds = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Route")
	float CruiseAltitudeCm = 42000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Route")
	bool bRequireCheckedInTicket = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Passengers")
	TSubclassOf<AGCPedestrian> PassengerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Entertainment")
	TArray<FString> MovieTitles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airliner|Entertainment")
	TArray<FString> MusicTracks;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	EGCFlightPhase Phase = EGCFlightPhase::Parked;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	EGCEntertainmentMode EntertainmentMode = EGCEntertainmentMode::FlightMap;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	float FlightProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	float DisplaySpeedKmh = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	float DisplayAltitudeM = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	FString CurrentSubtitle;

	UPROPERTY(BlueprintReadOnly, Category = "Airliner|State")
	float SubtitleExpiryTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Airliner")
	bool BoardPassenger(AGCCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Airliner")
	void Disembark();

	UFUNCTION(BlueprintCallable, Category = "Airliner")
	FString GetSeatbackScreenText() const;

	UFUNCTION(BlueprintCallable, Category = "Airliner")
	FString GetPhaseText() const;

private:
	UPROPERTY()
	AGCCharacter* PlayerPassenger = nullptr;

	UPROPERTY()
	TArray<AGCPedestrian*> NPCPassengers;

	bool bFlying = false;
	float FlightElapsed = 0.0f;
	float DepartCountdown = -1.0f;
	float ArrivedTimer = 0.0f;
	bool bGateAnnouncementPlayed = false;
	int32 SelectedMovie = 0;
	int32 SelectedTrack = 0;
	bool bCabinCameraActive = true;

	FVector RouteP0;
	FVector RouteP1;
	FVector RouteP2;
	FVector RouteP3;
	FVector LastPosition;
	float LastYaw = 0.0f;
	EGCFlightPhase LastAnnouncedPhase = EGCFlightPhase::Parked;

	void StartFlight();
	void FinishFlight();
	void SpawnNPCPassengers();
	void ReleaseNPCPassengers();
	void Announce(FName LineID, const FString& Fallback);
	void SetPhase(EGCFlightPhase NewPhase);
	static FVector CubicBezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
	static FVector CubicBezierDerivative(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);

	void Input_EntertainmentOff();
	void Input_EntertainmentMap();
	void Input_EntertainmentMovies();
	void Input_EntertainmentMusic();
	void Input_EntertainmentGames();
	void Input_ToggleCamera();
	void Input_Exit();
	void Input_CameraTurn(float Value);
	void Input_CameraLookUp(float Value);
};

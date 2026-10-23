#include "Vehicle/GCAirlinerPawn.h"
#include "Camera/CameraComponent.h"
#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Character/GCPedestrian.h"
#include "Components/CapsuleComponent.h"
#include "Components/GCVoiceLineComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"

AGCAirlinerPawn::AGCAirlinerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Root);
	InteractionBox->SetBoxExtent(FVector(650.0f, 650.0f, 280.0f));
	InteractionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_Pawn);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);

	FuselageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FuselageMesh"));
	FuselageMesh->SetupAttachment(Root);
	FuselageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlayerSeat = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerSeat"));
	PlayerSeat->SetupAttachment(Root);
	PlayerSeat->SetRelativeLocation(FVector(180.0f, -70.0f, 90.0f));

	for (int32 i = 0; i < 8; ++i)
	{
		USceneComponent* Seat = CreateDefaultSubobject<USceneComponent>(
			*FString::Printf(TEXT("PassengerSeat%d"), i));
		Seat->SetupAttachment(Root);
		const float RowX = 100.0f - (i / 2) * 95.0f;
		const float SideY = (i % 2 == 0) ? 75.0f : -75.0f;
		Seat->SetRelativeLocation(FVector(RowX, SideY, 90.0f));
		PassengerSeats.Add(Seat);
	}

	ExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ExitPoint"));
	ExitPoint->SetupAttachment(Root);
	ExitPoint->SetRelativeLocation(FVector(420.0f, -320.0f, 20.0f));

	CabinCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CabinCamera"));
	CabinCamera->SetupAttachment(Root);
	CabinCamera->SetRelativeLocation(FVector(210.0f, -95.0f, 165.0f));
	CabinCamera->SetRelativeRotation(FRotator(-4.0f, 18.0f, 0.0f)); // window-seat view
	CabinCamera->FieldOfView = 72.0f;

	ExternalArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ExternalArm"));
	ExternalArm->SetupAttachment(Root);
	ExternalArm->SetRelativeLocation(FVector(0.0f, 0.0f, 260.0f));
	ExternalArm->TargetArmLength = 2400.0f;
	ExternalArm->bUsePawnControlRotation = true;
	ExternalArm->bInheritPitch = false;
	ExternalArm->bInheritRoll = false;
	ExternalArm->bEnableCameraLag = true;

	ExternalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ExternalCamera"));
	ExternalCamera->SetupAttachment(ExternalArm);
	ExternalCamera->FieldOfView = 70.0f;
	ExternalCamera->SetActive(false);

	Voice = CreateDefaultSubobject<UGCVoiceLineComponent>(TEXT("Voice"));

	PassengerClass = AGCPedestrian::StaticClass();

	MovieTitles = {
		TEXT("Midnight Runway (Action)"),
		TEXT("The Last Lighthouse (Drama)"),
		TEXT("Turbo Pigeons 3 (Family)"),
		TEXT("Ocean Deep: Making Waves (Documentary)"),
		TEXT("Gridlock Nights (Crime)")
	};
	MusicTracks = {
		TEXT("Skyline Radio - Morning Mix"),
		TEXT("Coastal FM - Smooth Cruise"),
		TEXT("Night Drive Classics"),
		TEXT("Island Breeze Sessions")
	};
}

void AGCAirlinerPawn::BeginPlay()
{
	Super::BeginPlay();
	LastPosition = GetActorLocation();
	LastYaw = GetActorRotation().Yaw;
}

// ---------------------------------------------------------------- helpers
FVector AGCAirlinerPawn::CubicBezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
	const float U = 1.0f - T;
	return U * U * U * P0 + 3.0f * U * U * T * P1 + 3.0f * U * T * T * P2 + T * T * T * P3;
}

FVector AGCAirlinerPawn::CubicBezierDerivative(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
	const float U = 1.0f - T;
	return 3.0f * U * U * (P1 - P0) + 6.0f * U * T * (P2 - P1) + 3.0f * T * T * (P3 - P2);
}

FString AGCAirlinerPawn::GetPhaseText() const
{
	switch (Phase)
	{
	case EGCFlightPhase::Parked: return TEXT("At gate");
	case EGCFlightPhase::Boarding: return TEXT("Boarding");
	case EGCFlightPhase::TakeoffRoll: return TEXT("Takeoff");
	case EGCFlightPhase::Climb: return TEXT("Climbing");
	case EGCFlightPhase::Cruise: return TEXT("Cruising");
	case EGCFlightPhase::Descent: return TEXT("Descending");
	case EGCFlightPhase::FinalApproach: return TEXT("Final approach");
	case EGCFlightPhase::Landed: return TEXT("Landed");
	default: return TEXT("Arrived");
	}
}

void AGCAirlinerPawn::Announce(FName LineID, const FString& Fallback)
{
	FString Subtitle;
	bool bSpoken = false;
	if (Voice)
	{
		bSpoken = Voice->PlayLine(LineID, Subtitle);
	}
	CurrentSubtitle = bSpoken && !Subtitle.IsEmpty() ? Subtitle : Fallback;
	SubtitleExpiryTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 7.0f : 7.0f;
	if (PlayerPassenger)
	{
		PlayerPassenger->ShowSubtitle(CurrentSubtitle, 7.0f);
	}
}

void AGCAirlinerPawn::SetPhase(EGCFlightPhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}
	Phase = NewPhase;
	switch (NewPhase)
	{
	case EGCFlightPhase::TakeoffRoll:
		Announce(TEXT("SafetyBriefing"), TEXT("Cabin crew, doors to automatic. Please fasten your seatbelts, stow tray tables, and enjoy takeoff."));
		break;
	case EGCFlightPhase::Climb:
		Announce(TEXT("CaptainClimb"), TEXT("This is your captain. We're climbing smoothly to cruising altitude — the seatbelt sign will come off shortly."));
		break;
	case EGCFlightPhase::Cruise:
		Announce(TEXT("CruiseService"), TEXT("We've reached cruising altitude. Your seatback screen has movies, music, games and a live flight map — press 1 to 4."));
		break;
	case EGCFlightPhase::Descent:
		Announce(TEXT("Descent"), TEXT("We're beginning our descent. Please return to your seats, fasten seatbelts, and stow larger items."));
		break;
	case EGCFlightPhase::FinalApproach:
		Announce(TEXT("FinalApproach"), TEXT("Cabin crew, prepare for landing. We'll be on the ground in just a few minutes."));
		break;
	case EGCFlightPhase::Landed:
		Announce(TEXT("Landed"), TEXT("Welcome to our destination! Please remain seated until the aircraft reaches the gate."));
		break;
	default:
		break;
	}
	LastAnnouncedPhase = NewPhase;
}

// ---------------------------------------------------------------- boarding
bool AGCAirlinerPawn::BoardPassenger(AGCCharacter* Character)
{
	if (!Character || PlayerPassenger || Phase == EGCFlightPhase::TakeoffRoll || bFlying)
	{
		return false;
	}
	if (FVector::Distance(GetActorLocation(), Character->GetActorLocation()) > 1200.0f)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (bRequireCheckedInTicket)
	{
		if (!PS || !PS->HasActiveTicket() || PS->ActiveTicket.Status != EGCTicketStatus::CheckedIn)
		{
			Character->ShowSubtitle(TEXT("Gate agent: I'll need a checked-in boarding pass — the check-in computers are in the terminal."), 5.0f);
			return false;
		}
		if (PS->ActiveTicket.FromCode != FromCode || PS->ActiveTicket.ToCode != ToCode)
		{
			Character->ShowSubtitle(TEXT("Gate agent: This boarding pass is for a different route. Check the screens for your flight."), 5.0f);
			return false;
		}
		FGCFlightTicket Ticket = PS->ActiveTicket;
		Ticket.Status = EGCTicketStatus::Boarded;
		PS->SetTicket(Ticket);
	}

	PlayerPassenger = Character;
	AController* CharController = Character->GetController();
	Character->BeginVehicleTransition();
	Character->SetActorLocation(PlayerSeat->GetComponentLocation());
	Character->CompleteEnterVehicle(this, PlayerSeat);
	if (CharController)
	{
		CharController->Possess(this);
	}

	Phase = EGCFlightPhase::Boarding;
	DepartCountdown = 5.0f;
	SpawnNPCPassengers();
	Announce(TEXT("WelcomeAboard"), TEXT("Welcome aboard Gridline Air! Find your seat, stow your bags, and we'll be pushing back shortly."));
	return true;
}

void AGCAirlinerPawn::Disembark()
{
	if (!PlayerPassenger || (bFlying && Phase != EGCFlightPhase::Arrived))
	{
		return;
	}
	AController* AirController = GetController();
	const bool bCompletedFlight = Phase == EGCFlightPhase::Arrived;

	PlayerPassenger->BeginVehicleTransition();
	PlayerPassenger->CompleteExitVehicle(ExitPoint->GetComponentLocation() + FVector(0, 0, 60.0f));

	if (APlayerController* PC = Cast<APlayerController>(AirController))
	{
		if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
		{
			if (bCompletedFlight)
			{
				PS->ClearTicket(); // trip complete; buy the return at the other airport
			}
		}
	}
	if (AirController)
	{
		AirController->Possess(PlayerPassenger);
	}
	PlayerPassenger = nullptr;
	ReleaseNPCPassengers();

	if (bCompletedFlight)
	{
		// Turn the aircraft around for the next passenger (same parking spot logic next flight).
		Phase = EGCFlightPhase::Parked;
	}
}

void AGCAirlinerPawn::SpawnNPCPassengers()
{
	if (!PassengerClass || NPCPassengers.Num() > 0)
	{
		return;
	}
	for (USceneComponent* Seat : PassengerSeats)
	{
		AGCPedestrian* Ped = GetWorld()->SpawnActorDeferred<AGCPedestrian>(PassengerClass,
			Seat->GetComponentTransform());
		if (!Ped)
		{
			continue;
		}
		Ped->AutoPossessAI = EAutoPossessAI::Disabled;
		Ped->bIsPolice = false;
		Ped->FinishSpawning(Seat->GetComponentTransform());
		Ped->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ped->GetCharacterMovement()->DisableMovement();
		Ped->AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		NPCPassengers.Add(Ped);
	}
}

void AGCAirlinerPawn::ReleaseNPCPassengers()
{
	for (AGCPedestrian* Ped : NPCPassengers)
	{
		if (!IsValid(Ped))
		{
			continue;
		}
		Ped->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Ped->SetActorLocation(ExitPoint->GetComponentLocation()
			+ FVector(FMath::FRandRange(-200.0f, 200.0f), FMath::FRandRange(-200.0f, 200.0f), 60.0f),
			false, nullptr, ETeleportType::TeleportPhysics);
		Ped->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Ped->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Ped->SetLifeSpan(30.0f);
	}
	NPCPassengers.Reset();
}

// ---------------------------------------------------------------- flight
void AGCAirlinerPawn::StartFlight()
{
	if (!DestinationActor)
	{
		Announce(TEXT("Delay"), TEXT("We're sorry — this flight is delayed because the destination airport isn't set up in the level yet."));
		return;
	}
	RouteP0 = GetActorLocation();
	RouteP3 = DestinationActor->GetActorLocation() + FVector(0, 0, 140.0f);
	const FVector StartForward = GetActorForwardVector();
	const FVector DestForward = DestinationActor->GetActorForwardVector();
	const float CruiseZ = FMath::Max(RouteP0.Z, RouteP3.Z) + CruiseAltitudeCm;

	RouteP1 = RouteP0 + StartForward * 11000.0f;                  // takeoff roll stays level
	RouteP1.Z = RouteP0.Z + 300.0f;
	RouteP2 = RouteP3 - DestForward * 14000.0f;                  // long final from cruise
	RouteP2.Z = CruiseZ;

	bFlying = true;
	FlightElapsed = 0.0f;
	FlightProgress = 0.0f;
	ArrivedTimer = 0.0f;
	bGateAnnouncementPlayed = false;
	LastPosition = RouteP0;
	SetPhase(EGCFlightPhase::TakeoffRoll);
}

void AGCAirlinerPawn::FinishFlight()
{
	bFlying = false;
	SetPhase(EGCFlightPhase::Landed);
	Phase = EGCFlightPhase::Arrived; // Landed announcement above, state settles to Arrived
	DisplaySpeedKmh = 0.0f;
}

void AGCAirlinerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DepartCountdown > 0.0f && PlayerPassenger && !bFlying)
	{
		DepartCountdown -= DeltaSeconds;
		if (DepartCountdown <= 0.0f)
		{
			StartFlight();
		}
	}

	if (Phase == EGCFlightPhase::Arrived && PlayerPassenger && !bGateAnnouncementPlayed)
	{
		ArrivedTimer += DeltaSeconds;
		if (ArrivedTimer > 3.0f)
		{
			bGateAnnouncementPlayed = true;
			Announce(TEXT("GateArrival"), TEXT("We've arrived at the gate. Thank you for flying Gridline Air — press E to head into the terminal."));
		}
	}

	if (!bFlying || !DestinationActor)
	{
		return;
	}

	FlightElapsed += DeltaSeconds;
	const float T = FMath::Clamp(FlightElapsed / FMath::Max(10.0f, FlightDurationSeconds), 0.0f, 1.0f);
	FlightProgress = T;

	const FVector NewPos = CubicBezier(RouteP0, RouteP1, RouteP2, RouteP3, T);
	const FVector Derivative = CubicBezierDerivative(RouteP0, RouteP1, RouteP2, RouteP3, T);

	// Speed/altitude displays from actual motion.
	if (DeltaSeconds > 0.0f)
	{
		DisplaySpeedKmh = (FVector::Distance(NewPos, LastPosition) / DeltaSeconds) * 0.036f;
	}
	DisplayAltitudeM = FMath::Max(0.0f, (NewPos.Z - RouteP0.Z) / 100.0f);

	FRotator NewRot = GetActorRotation();
	if (!Derivative.IsNearlyZero())
	{
		NewRot = Derivative.GetSafeNormal().Rotation();
		// Bank into turns from yaw rate.
		float YawDelta = FMath::FindDeltaAngleDegrees(LastYaw, NewRot.Yaw);
		const float YawRate = DeltaSeconds > 0.0f ? YawDelta / DeltaSeconds : 0.0f;
		NewRot.Roll = FMath::Clamp(-YawRate * 18.0f, -26.0f, 26.0f);
		LastYaw = NewRot.Yaw;
	}

	SetActorLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::None);
	LastPosition = NewPos;

	// Phase from progress (route shape makes altitude follow naturally).
	if (T < 0.10f) { SetPhase(EGCFlightPhase::TakeoffRoll); }
	else if (T < 0.28f) { SetPhase(EGCFlightPhase::Climb); }
	else if (T < 0.68f) { SetPhase(EGCFlightPhase::Cruise); }
	else if (T < 0.86f) { SetPhase(EGCFlightPhase::Descent); }
	else if (T < 1.0f) { SetPhase(EGCFlightPhase::FinalApproach); }

	if (T >= 1.0f)
	{
		SetActorLocationAndRotation(RouteP3, DestinationActor->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		FinishFlight();
	}
}

// ---------------------------------------------------------------- entertainment
FString AGCAirlinerPawn::GetSeatbackScreenText() const
{
	FString Header = FString::Printf(TEXT("GRIDLINE AIR  %s -> %s   [%s]\n"), *FromCode, *ToCode, *GetPhaseText());
	switch (EntertainmentMode)
	{
	case EGCEntertainmentMode::Off:
		return Header + TEXT("Screen off. [1] Map  [2] Movies  [3] Music  [4] Games");
	case EGCEntertainmentMode::Movies:
	{
		FString Out = Header + TEXT("MOVIES  (press 2 to change film)\n");
		for (int32 i = 0; i < MovieTitles.Num(); ++i)
		{
			Out += FString::Printf(TEXT("%s %s\n"), i == SelectedMovie ? TEXT(">") : TEXT(" "), *MovieTitles[i]);
		}
		return Out;
	}
	case EGCEntertainmentMode::Music:
	{
		FString Out = Header + TEXT("MUSIC  (press 3 to change channel)\n");
		for (int32 i = 0; i < MusicTracks.Num(); ++i)
		{
			Out += FString::Printf(TEXT("%s %s\n"), i == SelectedTrack ? TEXT(">") : TEXT(" "), *MusicTracks[i]);
		}
		return Out;
	}
	case EGCEntertainmentMode::Games:
		return Header + TEXT("GAMES\n> Runway Racer (mini-game comes with the UMG pass)\n  Sky Sudoku\n  Trivia: World Airports");
	default: // FlightMap
	{
		const int32 Filled = FMath::RoundToInt(FlightProgress * 24.0f);
		FString Bar;
		for (int32 i = 0; i < 24; ++i) { Bar += (i < Filled ? TEXT("#") : TEXT("-")); }
		const float RemainingSec = (1.0f - FlightProgress) * FlightDurationSeconds;
		return Header + FString::Printf(
			TEXT("FLIGHT MAP\n%s [%s] %s\nSpeed %.0f km/h   Altitude %.0f m\nTime remaining ~%d min (game time)\n[1] Map [2] Movies [3] Music [4] Games [0] Off"),
			*FromCode, *Bar, *ToCode, DisplaySpeedKmh, DisplayAltitudeM,
			FMath::Max(0, FMath::RoundToInt(RemainingSec / 60.0f)));
	}
	}
}

void AGCAirlinerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AGCAirlinerPawn::Input_EntertainmentOff);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AGCAirlinerPawn::Input_EntertainmentMap);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AGCAirlinerPawn::Input_EntertainmentMovies);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AGCAirlinerPawn::Input_EntertainmentMusic);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AGCAirlinerPawn::Input_EntertainmentGames);
	PlayerInputComponent->BindAction(TEXT("ToggleCamera"), IE_Pressed, this, &AGCAirlinerPawn::Input_ToggleCamera);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AGCAirlinerPawn::Input_Exit);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AGCAirlinerPawn::Input_CameraTurn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGCAirlinerPawn::Input_CameraLookUp);
}

void AGCAirlinerPawn::Input_EntertainmentOff() { EntertainmentMode = EGCEntertainmentMode::Off; }
void AGCAirlinerPawn::Input_EntertainmentMap() { EntertainmentMode = EGCEntertainmentMode::FlightMap; }
void AGCAirlinerPawn::Input_EntertainmentMovies()
{
	if (EntertainmentMode == EGCEntertainmentMode::Movies && MovieTitles.Num() > 0)
	{
		SelectedMovie = (SelectedMovie + 1) % MovieTitles.Num();
	}
	EntertainmentMode = EGCEntertainmentMode::Movies;
}
void AGCAirlinerPawn::Input_EntertainmentMusic()
{
	if (EntertainmentMode == EGCEntertainmentMode::Music && MusicTracks.Num() > 0)
	{
		SelectedTrack = (SelectedTrack + 1) % MusicTracks.Num();
	}
	EntertainmentMode = EGCEntertainmentMode::Music;
}
void AGCAirlinerPawn::Input_EntertainmentGames() { EntertainmentMode = EGCEntertainmentMode::Games; }

void AGCAirlinerPawn::Input_ToggleCamera()
{
	bCabinCameraActive = !bCabinCameraActive;
	CabinCamera->SetActive(bCabinCameraActive);
	ExternalCamera->SetActive(!bCabinCameraActive);
}

void AGCAirlinerPawn::Input_Exit() { Disembark(); }
void AGCAirlinerPawn::Input_CameraTurn(float Value) { AddControllerYawInput(Value); }
void AGCAirlinerPawn::Input_CameraLookUp(float Value) { AddControllerPitchInput(Value); }

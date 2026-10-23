#include "Core/GCPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Character/GCCharacter.h"
#include "UI/GCLoadingScreenWidget.h"
#include "UI/GCIdentitySetupWidget.h"
#include "UI/GCIdentityCardWidget.h"
#include "UI/GCPoliceActionWheelWidget.h"
#include "UI/GCBoardingPassWidget.h"
#include "UI/GCMainMenuWidget.h"
#include "UI/GCSettingsWidget.h"
#include "Character/GCPedestrian.h"
#include "Components/GCDeformationComponent.h"
#include "Components/GCHealthComponent.h"
#include "Core/GCPlayerState.h"
#include "Core/GCProfileSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicle/GCVehiclePawn.h"
#include "World/GCDayNightManager.h"

AGCPlayerController::AGCPlayerController()
{
	DefaultVehicleClass = AGCVehiclePawn::StaticClass();
	MainMenuClass = UGCMainMenuWidget::StaticClass();
	SettingsClass = UGCSettingsWidget::StaticClass();
	LoadingClass = UGCLoadingScreenWidget::StaticClass();
	IdentitySetupClass = UGCIdentitySetupWidget::StaticClass();
	bShowMouseCursor = false;
}

void AGCPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController())
    {
        return;
    }

    for (TActorIterator<AGCDayNightManager> It(GetWorld()); It; ++It)
    {
        CachedDayNightManager = *It;
        break;
    }

    AGCPlayerState* PS = GetPlayerState<AGCPlayerState>();
    bool bHadSave = false;
    if (PS)
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UGCProfileSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UGCProfileSaveSubsystem>())
            {
                SaveSubsystem->LoadPlayerState(PS, bHadSave);
            }
        }
        if (bHadSave)
        {
            if (AGCDayNightManager* DayNight = CachedDayNightManager.Get())
            {
                DayNight->SetGameClock(PS->LastSavedTimeOfDay, PS->LastSavedGameDayIndex);
            }
            const FString CurrentMap = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
            if (PS->bHasSavedReturnLocation && CurrentMap.Equals(PS->LastSavedMapName, ESearchCase::IgnoreCase) && GetPawn())
            {
                GetPawn()->SetActorTransform(PS->LastSavedPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
            }
        }
        if (!PS->CitizenID.IsEmpty())
        {
            EnsurePersonalVehicles();
            if (AGCCharacter* Character = Cast<AGCCharacter>(GetPawn()))
            {
                Character->RefreshOutfitForProfile();
            }
        }
    }

    if (!PS || PS->CitizenID.IsEmpty())
    {
        ShowIdentitySetup();
    }
    else if (bShowMainMenuOnStart)
    {
        ShowMainMenu();
    }
}

void AGCPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsLocalController())
    {
        if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
        {
            PS->SyncOwnedVehicles();
            PS->RequestPersistentSave();
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AGCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AGCPlayerController::TogglePauseMenu);
        InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AGCPlayerController::ToggleIdentityCard);
        InputComponent->BindKey(EKeys::F8, IE_Pressed, this, &AGCPlayerController::OpenPhoneBoardingPass);
        InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &AGCPlayerController::PrintBoardingPass);
	}
}

void AGCPlayerController::ShowPoliceActionWheel(bool bVehicleMode)
{
    const AGCPlayerState* PlayerState = GetPlayerState<AGCPlayerState>();
    if (!IsLocalController() || !GetPawn() || !PlayerState || !PlayerState->IsPoliceOnDuty())
    {
        HidePoliceActionWheel();
        return;
    }
    if (!PoliceActionWheelWidget)
    {
        PoliceActionWheelWidget = CreateWidget<UGCPoliceActionWheelWidget>(this, UGCPoliceActionWheelWidget::StaticClass());
        if (PoliceActionWheelWidget)
        {
            PoliceActionWheelWidget->OnActionSelected.AddDynamic(this, &AGCPlayerController::HandlePoliceActionSelected);
            PoliceActionWheelWidget->AddToViewport(70);
        }
    }
    if (PoliceActionWheelWidget)
    {
        PoliceActionWheelWidget->ConfigureForVehicle(bVehicleMode);
        PoliceActionWheelWidget->SetVisibility(ESlateVisibility::Visible);
        SetMenuInput(true);
    }
}

void AGCPlayerController::HidePoliceActionWheel()
{
    if (PoliceActionWheelWidget && PoliceActionWheelWidget->GetVisibility() == ESlateVisibility::Visible)
    {
        PoliceActionWheelWidget->SetVisibility(ESlateVisibility::Collapsed);
        SetMenuInput(false);
    }
}

void AGCPlayerController::HandlePoliceActionSelected(FName ActionID)
{
    AGCCharacter* Character = Cast<AGCCharacter>(GetPawn());
    if (!Character)
    {
        HidePoliceActionWheel();
        return;
    }
    Character->DispatchPoliceAction(ActionID);
    HidePoliceActionWheel();
}

void AGCPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateWantedVisibility(DeltaTime);
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
        PS->TotalPlaytimeSeconds += FMath::Max(0.0, static_cast<double>(DeltaTime));
        if (AGCDayNightManager* DayNight = CachedDayNightManager.Get())
        {
            PS->ProcessPropertyDay(DayNight->GameDayIndex);
        }
        AutosaveTimer += DeltaTime;
        if (AutosaveTimer >= 30.0f)
        {
            AutosaveTimer = 0.0f;
            PS->SyncOwnedVehicles();
            PS->RequestPersistentSave();
        }
		const int32 Wage = PS->TickJobIncome(DeltaTime);
		if (Wage > 0)
		{
			if (AGCCharacter* Character = Cast<AGCCharacter>(GetPawn()))
			{
				Character->ShowSubtitle(FString::Printf(TEXT("Job pay deposited to digital wallet: +$%d."), Wage), 4.0f);
			}
		}
	}
}

void AGCPlayerController::UpdateWantedVisibility(float DeltaTime)
{
	AGCPlayerState* PS = GetPlayerState<AGCPlayerState>();
	APawn* MyPawn = GetPawn();
	if (!PS || !MyPawn || PS->WantedStars <= 0)
	{
		if (PS)
		{
			PS->TickWantedDecay(DeltaTime, false);
		}
		return;
	}

	WantedCheckTimer -= DeltaTime;
	static bool bCachedSeen = true;
	if (WantedCheckTimer <= 0.0f)
	{
		WantedCheckTimer = 0.3f;
		bCachedSeen = false;

		auto CheckCop = [&](AActor* Cop) -> bool
		{
			if (!Cop)
			{
				return false;
			}
			if (FVector::Distance(Cop->GetActorLocation(), MyPawn->GetActorLocation()) > 6500.0f)
			{
				return false;
			}
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(WantedLOS), false, Cop);
			Params.AddIgnoredActor(MyPawn);
			const FVector Start = Cop->GetActorLocation() + FVector(0, 0, 140.0f);
			const FVector End = MyPawn->GetActorLocation() + FVector(0, 0, 100.0f);
			return !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		};

		for (TActorIterator<AGCPedestrian> It(GetWorld()); It && !bCachedSeen; ++It)
		{
			if (It->bIsPolice && !It->bDead)
			{
				bCachedSeen = CheckCop(*It);
			}
		}
		for (TActorIterator<AGCVehiclePawn> It(GetWorld()); It && !bCachedSeen; ++It)
		{
			if (It->bIsPoliceVehicle)
			{
				bCachedSeen = CheckCop(*It);
			}
		}
	}
	PS->TickWantedDecay(DeltaTime, bCachedSeen);
}

// ---------------------------------------------------------------- dev commands
void AGCPlayerController::GCGiveMoney(int32 Amount)
{
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
		PS->AddMoney(Amount);
	}
}

void AGCPlayerController::GCSetWanted(int32 Stars)
{
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
		PS->ClearWanted();
		if (Stars > 0)
		{
			PS->AddWantedStars(Stars);
		}
	}
}

void AGCPlayerController::EnsurePersonalVehicles()
{
    AGCPlayerState* PS = GetPlayerState<AGCPlayerState>();
    APawn* PlayerPawn = GetPawn();
    if (!PS || !PlayerPawn || !GetWorld() || !DefaultVehicleClass)
    {
        return;
    }
    if (PS->OwnedVehicleRecords.IsEmpty())
    {
        GCSpawnCar();
        return;
    }

    const FString CurrentMap = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    int32 SpawnedIndex = 0;
    for (const FGCOwnedVehicleRecord& Record : PS->OwnedVehicleRecords)
    {
        bool bAlreadyPresent = false;
        for (TActorIterator<AGCVehiclePawn> It(GetWorld()); It; ++It)
        {
            if (It->VehicleID.Equals(Record.VehicleID, ESearchCase::IgnoreCase))
            {
                bAlreadyPresent = true;
                break;
            }
        }
        if (bAlreadyPresent)
        {
            continue;
        }

        UClass* VehicleClass = DefaultVehicleClass.Get();
        if (!Record.VehicleClassPath.IsEmpty())
        {
            if (UClass* SavedClass = LoadClass<AGCVehiclePawn>(nullptr, *Record.VehicleClassPath))
            {
                VehicleClass = SavedClass;
            }
        }
        FVector SpawnLocation = PlayerPawn->GetActorLocation()
            + PlayerPawn->GetActorForwardVector() * (650.0f + 520.0f * SpawnedIndex)
            + FVector(0.0f, 0.0f, 160.0f);
        FRotator SpawnRotation(0.0f, PlayerPawn->GetActorRotation().Yaw, 0.0f);
        if (Record.LastKnownMap.Equals(CurrentMap, ESearchCase::IgnoreCase))
        {
            SpawnLocation = Record.LastKnownTransform.GetLocation();
            SpawnRotation = Record.LastKnownTransform.Rotator();
        }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AGCVehiclePawn* Vehicle = GetWorld()->SpawnActor<AGCVehiclePawn>(VehicleClass, SpawnLocation, SpawnRotation, Params);
        if (!Vehicle)
        {
            continue;
        }
        Vehicle->VehicleID = Record.VehicleID;
        Vehicle->DisplayName = Record.DisplayName;
        Vehicle->LicensePlate = Record.LicensePlate;
        Vehicle->bIsPlayerOwned = true;
        Vehicle->bWasStolen = Record.RegistrationStatus == EGCVehicleTitleStatus::Stolen;
        Vehicle->bIsPoliceVehicle = PS->CurrentJob == EGCJob::Police;
        Vehicle->bWrecked = Record.bWrecked;
        if (Vehicle->Health)
        {
            Vehicle->Health->Health = Vehicle->Health->MaxHealth * FMath::Clamp(Record.HealthFraction, 0.0f, 1.0f);
        }
        Vehicle->EngineTemperatureC = FMath::Max(20.0f, Record.EngineTemperatureC);
        Vehicle->RadiatorIntegrity = FMath::Clamp(Record.RadiatorIntegrity, 0.0f, 1.0f);
        for (int32 WheelIndex = 0; WheelIndex < Record.TireIntegrity.Num() && Vehicle->TireConditions.IsValidIndex(WheelIndex); ++WheelIndex)
        {
            Vehicle->DamageTire(WheelIndex, 1.0f - FMath::Clamp(Record.TireIntegrity[WheelIndex], 0.0f, 1.0f));
        }
        PS->RegisterOwnedVehicle(Vehicle, Vehicle->bWasStolen);
        ++SpawnedIndex;
        if (SpawnedIndex >= 5)
        {
            break;
        }
    }
}

void AGCPlayerController::GCSpawnCar()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || !DefaultVehicleClass)
	{
		return;
	}
	const FVector Loc = MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 650.0f + FVector(0, 0, 160.0f);
	const FRotator Rot(0.0f, MyPawn->GetActorRotation().Yaw, 0.0f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (AGCVehiclePawn* SpawnedVehicle = GetWorld()->SpawnActor<AGCVehiclePawn>(DefaultVehicleClass, Loc, Rot, Params))
    {
        SpawnedVehicle->bIsPlayerOwned = true;
        if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
        {
            SpawnedVehicle->bIsPoliceVehicle = PS->CurrentJob == EGCJob::Police;
            PS->RegisterOwnedVehicle(SpawnedVehicle, false);
        }
    }
}

void AGCPlayerController::GCTeleport(float X, float Y, float Z)
{
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->SetActorLocation(FVector(X, Y, Z), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AGCPlayerController::GCSetTime(float Hour)
{
	for (TActorIterator<AGCDayNightManager> It(GetWorld()); It; ++It)
	{
		It->SetTimeOfDay(Hour);
		break;
	}
}

void AGCPlayerController::GCGod()
{
	APawn* MyPawn = GetPawn();
	AGCCharacter* Char = Cast<AGCCharacter>(MyPawn);
	if (!Char && MyPawn)
	{
		// Seated in a vehicle: the character is attached to the seat.
		TArray<AActor*> Attached = MyPawn->Children;
		for (AActor* Child : Attached)
		{
			Char = Cast<AGCCharacter>(Child);
			if (Char) { break; }
		}
	}
	if (Char && Char->Health)
	{
		Char->Health->bGodMode = !Char->Health->bGodMode;
	}
}

void AGCPlayerController::GCSetJob(FString JobName)
{
	AGCPlayerState* PS = GetPlayerState<AGCPlayerState>();
	if (!PS)
	{
		return;
	}
	const EGCJob PreviousJob = PS->CurrentJob;
    const FString N = JobName.ToLower();
	if (N.Contains(TEXT("president"))) { PS->CurrentJob = EGCJob::President; }
	else if (N.Contains(TEXT("cop")) || N.Contains(TEXT("police"))) { PS->CurrentJob = EGCJob::Police; }
	else if (N.Contains(TEXT("medic")) || N.Contains(TEXT("doctor"))) { PS->CurrentJob = EGCJob::Medic; }
	else if (N.Contains(TEXT("taxi"))) { PS->CurrentJob = EGCJob::Taxi; }
	else if (N.Contains(TEXT("shop"))) { PS->CurrentJob = EGCJob::ShopOwner; }
	else if (N.Contains(TEXT("border"))) { PS->CurrentJob = EGCJob::BorderPatrol; }
	else if (N.Contains(TEXT("hitman"))) { PS->CurrentJob = EGCJob::Hitman; }
	else { PS->CurrentJob = EGCJob::Civilian; }
    if (PS->CurrentJob == EGCJob::Police
        && (PreviousJob != EGCJob::Police || !PS->bPoliceShiftActive || PS->bPoliceShiftFailed))
    {
        PS->StartPoliceShift();
    }
    if (PS->CurrentJob != PreviousJob && PS->CurrentJob != EGCJob::Police)
    {
        PS->bPoliceShiftActive = false;
    }
    for (TActorIterator<AGCVehiclePawn> It(GetWorld()); It; ++It)
    {
        if (It->bIsPlayerOwned)
        {
            It->bIsPoliceVehicle = PS->CurrentJob == EGCJob::Police;
            if (!It->bIsPoliceVehicle)
            {
                It->SetSiren(false);
            }
        }
    }
    PS->RequestPersistentSave();
}

void AGCPlayerController::GCRepairVehicle()
{
	if (AGCVehiclePawn* Vehicle = Cast<AGCVehiclePawn>(GetPawn()))
	{
		if (Vehicle->Deformation)
		{
			Vehicle->Deformation->RepairAll();
		}
        Vehicle->RepairMechanicalSystems();
		if (Vehicle->Health)
		{
			Vehicle->Health->Revive();
		}
		Vehicle->bWrecked = false;
	}
}

// ---------------------------------------------------------------- menus (Phase 2)
UGCIdentityCardWidget* AGCPlayerController::EnsureIdentityCard()
{
    if (!IdentityCardWidget)
    {
        IdentityCardWidget = CreateWidget<UGCIdentityCardWidget>(this, UGCIdentityCardWidget::StaticClass());
        if (IdentityCardWidget)
        {
            IdentityCardWidget->AddToViewport(25);
            IdentityCardWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    return IdentityCardWidget;
}

void AGCPlayerController::ShowBoardingPass(bool bPaperCopy)
{
    if (!IsLocalController()) return;
    AGCPlayerState* PS = GetPlayerState<AGCPlayerState>();
    if (!PS || !PS->HasActiveTicket())
    {
        if (AGCCharacter* Character = Cast<AGCCharacter>(GetPawn()))
        {
            Character->ShowSubtitle(TEXT("No flight booking is saved to your phone."));
        }
        return;
    }
    if (bPaperCopy && PS->ActiveTicket.Status != EGCTicketStatus::CheckedIn
        && PS->ActiveTicket.Status != EGCTicketStatus::Boarded)
    {
        if (AGCCharacter* Character = Cast<AGCCharacter>(GetPawn()))
        {
            Character->ShowSubtitle(TEXT("Check in first; a boarding pass needs an assigned seat and gate."));
        }
        return;
    }
    if (bPaperCopy && !PS->ActiveTicket.bPaperCopyPrinted)
    {
        FGCFlightTicket PrintedTicket = PS->ActiveTicket;
        PrintedTicket.bPaperCopyPrinted = true;
        PS->SetTicket(PrintedTicket);
    }
    if (!BoardingPassWidget)
    {
        BoardingPassWidget = CreateWidget<UGCBoardingPassWidget>(this, UGCBoardingPassWidget::StaticClass());
        if (BoardingPassWidget) BoardingPassWidget->AddToViewport(80);
    }
    if (BoardingPassWidget)
    {
        BoardingPassWidget->ConfigurePass(PS->ActiveTicket, bPaperCopy);
        BoardingPassWidget->SetVisibility(ESlateVisibility::Visible);
        SetPause(false);
        SetMenuInput(true);
    }
}

void AGCPlayerController::OpenPhoneBoardingPass()
{
    ShowBoardingPass(false);
}

void AGCPlayerController::PrintBoardingPass()
{
    ShowBoardingPass(true);
}

void AGCPlayerController::ToggleIdentityCard()
{
    if (!IsLocalController() || (IdentitySetupWidget && IdentitySetupWidget->GetVisibility() == ESlateVisibility::Visible))
    {
        return;
    }
    if (IdentityCardWidget && IdentityCardWidget->GetVisibility() == ESlateVisibility::Visible)
    {
        IdentityCardWidget->SetVisibility(ESlateVisibility::Collapsed);
        SetPause(false);
        SetMenuInput(false);
        return;
    }
    if ((MainMenuWidget && MainMenuWidget->GetVisibility() == ESlateVisibility::Visible)
        || (SettingsWidget && SettingsWidget->GetVisibility() == ESlateVisibility::Visible))
    {
        return;
    }
    if (UGCIdentityCardWidget* Card = EnsureIdentityCard())
    {
        Card->SetVisibility(ESlateVisibility::Visible);
        SetPause(true);
        SetMenuInput(true);
    }
}

UGCIdentitySetupWidget* AGCPlayerController::EnsureIdentitySetup()
{
	if (!IdentitySetupWidget)
	{
		if (!IdentitySetupClass)
		{
			IdentitySetupClass = UGCIdentitySetupWidget::StaticClass();
		}
		IdentitySetupWidget = CreateWidget<UGCIdentitySetupWidget>(this, IdentitySetupClass);
		if (IdentitySetupWidget)
		{
			IdentitySetupWidget->OnIdentitySetupComplete.AddDynamic(this, &AGCPlayerController::HandleIdentitySetupComplete);
			IdentitySetupWidget->AddToViewport(50);
			IdentitySetupWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return IdentitySetupWidget;
}

void AGCPlayerController::ShowIdentitySetup()
{
	UGCIdentitySetupWidget* Setup = EnsureIdentitySetup();
	if (!Setup)
	{
		return;
	}
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	Setup->SetVisibility(ESlateVisibility::Visible);
	SetMenuInput(true);
	SetPause(true);
}

UGCMainMenuWidget* AGCPlayerController::EnsureMainMenu()
{
	if (!MainMenuWidget)
	{
		if (!MainMenuClass)
		{
			MainMenuClass = UGCMainMenuWidget::StaticClass();
		}
		MainMenuWidget = CreateWidget<UGCMainMenuWidget>(this, MainMenuClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->OnPlayRequested.AddDynamic(this, &AGCPlayerController::HandleMenuPlay);
			MainMenuWidget->OnSettingsRequested.AddDynamic(this, &AGCPlayerController::HandleMenuSettings);
			MainMenuWidget->AddToViewport(10);
			MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return MainMenuWidget;
}

UGCSettingsWidget* AGCPlayerController::EnsureSettings()
{
	if (!SettingsWidget)
	{
		if (!SettingsClass)
		{
			SettingsClass = UGCSettingsWidget::StaticClass();
		}
		SettingsWidget = CreateWidget<UGCSettingsWidget>(this, SettingsClass);
		if (SettingsWidget)
		{
			SettingsWidget->OnClosed.AddDynamic(this, &AGCPlayerController::HandleSettingsClosed);
			SettingsWidget->AddToViewport(20);
			SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return SettingsWidget;
}

UGCLoadingScreenWidget* AGCPlayerController::EnsureLoading()
{
	if (!LoadingWidget)
	{
		if (!LoadingClass)
		{
			LoadingClass = UGCLoadingScreenWidget::StaticClass();
		}
		LoadingWidget = CreateWidget<UGCLoadingScreenWidget>(this, LoadingClass);
		if (LoadingWidget)
		{
			// The controller drives progress for the timed test screen.
			LoadingWidget->bSimulateProgress = false;
			LoadingWidget->AddToViewport(30);
			LoadingWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return LoadingWidget;
}

void AGCPlayerController::SetMenuInput(bool bUI)
{
	bShowMouseCursor = bUI;
	if (bUI)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AGCPlayerController::ShowMainMenu()
{
	UGCMainMenuWidget* Menu = EnsureMainMenu();
	if (!Menu)
	{
		return;
	}
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	bPauseMenuOpen = false;
	Menu->SetPauseMode(false);
	Menu->SetVisibility(ESlateVisibility::Visible);
	SetMenuInput(true);
	SetPause(true);
}

void AGCPlayerController::ShowPauseMenu()
{
	UGCMainMenuWidget* Menu = EnsureMainMenu();
	if (!Menu)
	{
		return;
	}
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
    if (IdentityCardWidget)
    {
        IdentityCardWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
	bPauseMenuOpen = true;
	Menu->SetPauseMode(true);
	Menu->SetVisibility(ESlateVisibility::Visible);
	SetMenuInput(true);
	SetPause(true);
}

void AGCPlayerController::TogglePauseMenu()
{
    if (BoardingPassWidget && BoardingPassWidget->GetVisibility() == ESlateVisibility::Visible)
    {
        BoardingPassWidget->SetVisibility(ESlateVisibility::Collapsed);
        SetPause(false);
        SetMenuInput(false);
        return;
    }
    if (PoliceActionWheelWidget && PoliceActionWheelWidget->GetVisibility() == ESlateVisibility::Visible)
    {
        HidePoliceActionWheel();
        return;
    }
    if (IdentityCardWidget && IdentityCardWidget->GetVisibility() == ESlateVisibility::Visible)
    {
        IdentityCardWidget->SetVisibility(ESlateVisibility::Collapsed);
        SetPause(false);
        SetMenuInput(false);
        return;
    }
	if (!IsLocalController() || (IdentitySetupWidget && IdentitySetupWidget->GetVisibility() == ESlateVisibility::Visible))
	{
		return;
	}
	if ((MainMenuWidget && MainMenuWidget->GetVisibility() == ESlateVisibility::Visible)
		|| (SettingsWidget && SettingsWidget->GetVisibility() == ESlateVisibility::Visible))
	{
		HideMenus();
	}
	else
	{
		ShowPauseMenu();
	}
}

void AGCPlayerController::ShowSettingsMenu()
{
	UGCSettingsWidget* Settings = EnsureSettings();
	if (!Settings)
	{
		return;
	}
	bReturnToMainOnSettingsClose = MainMenuWidget && MainMenuWidget->GetVisibility() == ESlateVisibility::Visible;
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	Settings->SetVisibility(ESlateVisibility::Visible);
	Settings->LoadFromSettings();
	SetMenuInput(true);
	SetPause(true);
}

void AGCPlayerController::ShowLoadingScreen(float Seconds)
{
	UGCLoadingScreenWidget* Loading = EnsureLoading();
	if (!Loading || !GetWorld())
	{
		return;
	}
	LoadingDuration = FMath::Max(0.5f, Seconds);
	LoadingElapsed = 0.0f;
	Loading->SetProgress(0.0f);
	Loading->SetVisibility(ESlateVisibility::Visible);
	GetWorld()->GetTimerManager().SetTimer(LoadingTimer, this, &AGCPlayerController::TickLoading, 0.1f, true);
}

void AGCPlayerController::HideMenus()
{
    if (BoardingPassWidget)
    {
        BoardingPassWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (PoliceActionWheelWidget)
    {
        PoliceActionWheelWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LoadingTimer);
	}
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (LoadingWidget)
	{
		LoadingWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
    if (IdentityCardWidget)
    {
        IdentityCardWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
	if (IdentitySetupWidget)
	{
		IdentitySetupWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	bReturnToMainOnSettingsClose = false;
	bPauseMenuOpen = false;
	SetPause(false);
	SetMenuInput(false);
}

void AGCPlayerController::HandleMenuPlay()
{
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>(); !PS || PS->CitizenID.IsEmpty())
	{
		ShowIdentitySetup();
		return;
	}
	HideMenus();
}

void AGCPlayerController::HandleIdentitySetupComplete()
{
    EnsurePersonalVehicles();
    if (AGCCharacter* Character = Cast<AGCCharacter>(GetPawn()))
    {
        Character->RefreshOutfitForProfile();
    }
	if (IdentitySetupWidget)
	{
		IdentitySetupWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (bShowMainMenuOnStart)
	{
		ShowMainMenu();
	}
	else
	{
		bPauseMenuOpen = false;
		SetPause(false);
		SetMenuInput(false);
	}
}

void AGCPlayerController::HandleMenuSettings()
{
	ShowSettingsMenu();
}

void AGCPlayerController::HandleSettingsClosed()
{
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (bReturnToMainOnSettingsClose)
	{
		bReturnToMainOnSettingsClose = false;
		if (bPauseMenuOpen)
		{
			ShowPauseMenu();
		}
		else
		{
			bReturnToMainOnSettingsClose = false;
			ShowMainMenu();
		}
	}
	else
	{
		HideMenus();
	}
}

void AGCPlayerController::TickLoading()
{
	LoadingElapsed += 0.1f;
	if (LoadingWidget)
	{
		LoadingWidget->SetProgress(LoadingDuration > 0.0f ? LoadingElapsed / LoadingDuration : 1.0f);
	}
	if (LoadingElapsed >= LoadingDuration)
	{
		HideLoadingScreen();
	}
}

void AGCPlayerController::HideLoadingScreen()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LoadingTimer);
	}
	if (LoadingWidget)
	{
		LoadingWidget->SetProgress(1.0f);
		LoadingWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AGCPlayerController::GCMenu()
{
	ShowMainMenu();
}

void AGCPlayerController::GCSettingsMenu()
{
	ShowSettingsMenu();
}

void AGCPlayerController::GCLoadingTest(float Seconds)
{
	ShowLoadingScreen(Seconds);
}

void AGCPlayerController::GCHideMenus()
{
	HideMenus();
}

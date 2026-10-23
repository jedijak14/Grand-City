#include "Core/GCProfileSaveSubsystem.h"

#include "Core/GCPlayerState.h"
#include "Core/GCProfileSaveGame.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "World/GCDayNightManager.h"

namespace
{
    constexpr int32 LocalUserIndex = 0;

    UGCProfileSaveGame* LoadOrCreateSave()
    {
        if (UGameplayStatics::DoesSaveGameExist(UGCProfileSaveSubsystem::GetSlotName(), LocalUserIndex))
        {
            UGCProfileSaveGame* Existing = Cast<UGCProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(
                UGCProfileSaveSubsystem::GetSlotName(), LocalUserIndex));
            if (Existing && Existing->SaveVersion <= UGCProfileSaveGame::CurrentSaveVersion)
            {
                return Existing;
            }
        }
        return Cast<UGCProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UGCProfileSaveGame::StaticClass()));
    }
}

const FString& UGCProfileSaveSubsystem::GetSlotName()
{
    static const FString SlotName(TEXT("Gridlock_CitizenProfile_0"));
    return SlotName;
}

bool UGCProfileSaveSubsystem::LoadPlayerState(AGCPlayerState* PlayerState, bool& bOutHadSave)
{
    bOutHadSave = false;
    if (!IsValid(PlayerState) || !GetGameInstance())
    {
        return false;
    }
    if (!UGameplayStatics::DoesSaveGameExist(GetSlotName(), LocalUserIndex))
    {
        return true;
    }

    UGCProfileSaveGame* Save = Cast<UGCProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(), LocalUserIndex));
    if (!Save || Save->SaveVersion > UGCProfileSaveGame::CurrentSaveVersion || Save->CitizenID.IsEmpty())
    {
        return false;
    }

    PlayerState->CitizenID = Save->CitizenID;
    PlayerState->CitizenName = Save->CitizenName;
    PlayerState->CitizenDateOfBirth = Save->CitizenDateOfBirth;
    PlayerState->CitizenSex = Save->CitizenSex;
    PlayerState->DriverLicenseType = Save->DriverLicenseType;
    PlayerState->DriverLicenseNumber = Save->DriverLicenseNumber;
    PlayerState->LicenseIssuedUtc = Save->LicenseIssuedUtc;
    PlayerState->LicenseExpiryUtc = Save->LicenseExpiryUtc;
    PlayerState->CitizenCreatedUtc = Save->CitizenCreatedUtc;
    if (PlayerState->DriverLicenseNumber.IsEmpty())
    {
        PlayerState->DriverLicenseNumber = FString::Printf(TEXT("CA-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(10).ToUpper());
    }
    if (PlayerState->CitizenCreatedUtc.GetYear() < 2)
    {
        PlayerState->CitizenCreatedUtc = Save->LastSavedUtc.GetYear() >= 2 ? Save->LastSavedUtc : FDateTime::UtcNow();
    }
    if (PlayerState->LicenseIssuedUtc.GetYear() < 2)
    {
        PlayerState->LicenseIssuedUtc = PlayerState->CitizenCreatedUtc;
    }
    if (PlayerState->LicenseExpiryUtc.GetYear() < 2)
    {
        PlayerState->LicenseExpiryUtc = PlayerState->LicenseIssuedUtc.AddYears(5);
    }
    PlayerState->CurrentAddress = Save->CurrentAddress;
    PlayerState->CurrentOutfitID = Save->CurrentOutfitID;
    PlayerState->Money = FMath::Max(0, Save->Money);
    PlayerState->DigitalBalance = FMath::Max(0, Save->DigitalBalance);
    const uint8 PaymentMethodValue = static_cast<uint8>(Save->SelectedPaymentMethod);
    PlayerState->SelectedPaymentMethod = PaymentMethodValue <= static_cast<uint8>(EGCPaymentMethod::BaylineCredit)
        ? Save->SelectedPaymentMethod : EGCPaymentMethod::Cash;
    PlayerState->CreditUsed = FMath::Clamp(Save->CreditUsed, 0, PlayerState->CreditLimit);
    const uint8 JobValue = static_cast<uint8>(Save->CurrentJob);
    PlayerState->CurrentJob = JobValue <= static_cast<uint8>(EGCJob::President) ? Save->CurrentJob : EGCJob::Civilian;
    PlayerState->ConductPoints = FMath::Clamp(Save->ConductPoints, 0, 100);
    PlayerState->ShiftPoints = FMath::Max(0, Save->ShiftPoints);
    PlayerState->bPoliceShiftActive = Save->bPoliceShiftActive && PlayerState->CurrentJob == EGCJob::Police;
    PlayerState->bPoliceShiftFailed = Save->bPoliceShiftFailed;
    PlayerState->PoliceBadgeNumber = Save->PoliceBadgeNumber;
    PlayerState->bHasAlcoholTestKit = Save->bHasAlcoholTestKit || PlayerState->bPoliceShiftActive;
    PlayerState->bHasDrugTestKit = Save->bHasDrugTestKit || PlayerState->bPoliceShiftActive;
    PlayerState->PoliceCitationRecords = Save->PoliceCitationRecords;
    PlayerState->ActiveTicket = Save->ActiveTicket;
    PlayerState->StealthSkill = FMath::Clamp(Save->StealthSkill, 0.0f, 1.0f);
    PlayerState->PropertyPortfolio.Reset();
    PlayerState->OwnedPropertyIDs.Reset();
    PlayerState->StolenPropertyIDs.Reset();
    TSet<FString> SeenPropertyIDs;
    for (FGCPropertyRecord Property : Save->Properties)
    {
        if (!Property.IsValidRecord() || SeenPropertyIDs.Contains(Property.PropertyID))
        {
            continue;
        }
        if (!Property.OwnerCitizenID.IsEmpty() && !Property.OwnerCitizenID.Equals(Save->CitizenID, ESearchCase::IgnoreCase))
        {
            continue;
        }
        Property.OwnerCitizenID = Save->CitizenID;
        Property.Condition = FMath::Clamp(Property.Condition, 0.0f, 1.0f);
        Property.OutstandingTax = FMath::Max<int64>(0, Property.OutstandingTax);
        Property.OutstandingMaintenance = FMath::Max<int64>(0, Property.OutstandingMaintenance);
        SeenPropertyIDs.Add(Property.PropertyID);
        PlayerState->PropertyPortfolio.Add(Property);
        PlayerState->OwnedPropertyIDs.Add(Property.PropertyID);
        if (Property.DeedStatus == EGCPropertyDeedStatus::Stolen)
        {
            PlayerState->StolenPropertyIDs.Add(Property.PropertyID);
        }
    }
    PlayerState->PrimaryResidenceID = Save->PrimaryResidenceID;
    const FGCPropertyRecord* Primary = PlayerState->PropertyPortfolio.FindByPredicate([PlayerState](const FGCPropertyRecord& Property)
    {
        return Property.PropertyID.Equals(PlayerState->PrimaryResidenceID, ESearchCase::IgnoreCase);
    });
    if (Primary)
    {
        PlayerState->CurrentAddress = Primary->StreetAddress;
    }
    else
    {
        PlayerState->PrimaryResidenceID.Empty();
        PlayerState->CurrentAddress.Empty();
    }
    PlayerState->OwnedVehicleRecords.Reset();
    TSet<FString> SeenVehicleIDs;
    for (FGCOwnedVehicleRecord Vehicle : Save->OwnedVehicles)
    {
        if (!Vehicle.IsValidRecord() || SeenVehicleIDs.Contains(Vehicle.VehicleID))
        {
            continue;
        }
        if (!Vehicle.OwnerCitizenID.IsEmpty() && !Vehicle.OwnerCitizenID.Equals(Save->CitizenID, ESearchCase::IgnoreCase))
        {
            continue;
        }
        Vehicle.OwnerCitizenID = Save->CitizenID;
        SeenVehicleIDs.Add(Vehicle.VehicleID);
        PlayerState->OwnedVehicleRecords.Add(Vehicle);
    }
    PlayerState->OwnedDeviceIDs = Save->OwnedDeviceIDs;
    PlayerState->ActivePhoneModel = Save->ActivePhoneModel;
    PlayerState->ActivePhoneOS = Save->ActivePhoneOS;
    PlayerState->TotalPlaytimeSeconds = FMath::Max(0.0, Save->TotalPlaytimeSeconds);
    PlayerState->LastProcessedGameDay = FMath::Max<int64>(0, Save->LastProcessedGameDay);
    PlayerState->LastSavedGameDayIndex = FMath::Max<int64>(0, Save->GameDayIndex);
    PlayerState->LastSavedTimeOfDay = FMath::Fmod(FMath::Max(0.0f, Save->TimeOfDay), 24.0f);
    PlayerState->LastSavedMapName = Save->LastMapName;
    PlayerState->LastSavedPlayerTransform = Save->LastPlayerTransform;
    PlayerState->bHasSavedReturnLocation = Save->bHasReturnLocation;
    PlayerState->ClearWanted();
    bOutHadSave = true;
    return true;
}

bool UGCProfileSaveSubsystem::SavePlayerState(const AGCPlayerState* PlayerState)
{
    if (!IsValid(PlayerState) || PlayerState->CitizenID.IsEmpty() || !GetGameInstance())
    {
        return false;
    }

    UGCProfileSaveGame* Save = LoadOrCreateSave();
    if (!Save)
    {
        return false;
    }
    Save->SaveVersion = UGCProfileSaveGame::CurrentSaveVersion;
    Save->LastSavedUtc = FDateTime::UtcNow();
    Save->CitizenID = PlayerState->CitizenID;
    Save->CitizenName = PlayerState->CitizenName;
    Save->CitizenDateOfBirth = PlayerState->CitizenDateOfBirth;
    Save->CitizenSex = PlayerState->CitizenSex;
    Save->DriverLicenseType = PlayerState->DriverLicenseType;
    Save->DriverLicenseNumber = PlayerState->DriverLicenseNumber;
    Save->LicenseIssuedUtc = PlayerState->LicenseIssuedUtc;
    Save->LicenseExpiryUtc = PlayerState->LicenseExpiryUtc;
    Save->CitizenCreatedUtc = PlayerState->CitizenCreatedUtc;
    Save->CurrentAddress = PlayerState->CurrentAddress;
    Save->CurrentOutfitID = PlayerState->CurrentOutfitID;
    Save->Money = PlayerState->Money;
    Save->DigitalBalance = PlayerState->DigitalBalance;
    Save->SelectedPaymentMethod = PlayerState->SelectedPaymentMethod;
    Save->CreditUsed = PlayerState->CreditUsed;
    Save->CurrentJob = PlayerState->CurrentJob;
    Save->ConductPoints = PlayerState->ConductPoints;
    Save->ShiftPoints = PlayerState->ShiftPoints;
    Save->bPoliceShiftActive = PlayerState->bPoliceShiftActive;
    Save->bPoliceShiftFailed = PlayerState->bPoliceShiftFailed;
    Save->PoliceBadgeNumber = PlayerState->PoliceBadgeNumber;
    Save->bHasAlcoholTestKit = PlayerState->bHasAlcoholTestKit;
    Save->bHasDrugTestKit = PlayerState->bHasDrugTestKit;
    Save->PoliceCitationRecords = PlayerState->PoliceCitationRecords;
    Save->ActiveTicket = PlayerState->ActiveTicket;
    Save->StealthSkill = PlayerState->StealthSkill;
    Save->Properties = PlayerState->PropertyPortfolio;
    Save->PrimaryResidenceID = PlayerState->PrimaryResidenceID;
    Save->OwnedVehicles = PlayerState->OwnedVehicleRecords;
    Save->OwnedDeviceIDs = PlayerState->OwnedDeviceIDs;
    Save->ActivePhoneModel = PlayerState->ActivePhoneModel;
    Save->ActivePhoneOS = PlayerState->ActivePhoneOS;
    Save->TotalPlaytimeSeconds = PlayerState->TotalPlaytimeSeconds;
    Save->LastProcessedGameDay = PlayerState->LastProcessedGameDay;

    if (UWorld* World = PlayerState->GetWorld())
    {
        const FString CurrentMap = UGameplayStatics::GetCurrentLevelName(World, true);
        if (!Save->LastMapName.IsEmpty() && !Save->LastMapName.Equals(CurrentMap, ESearchCase::IgnoreCase))
        {
            Save->bHasReturnLocation = false;
        }
        Save->LastMapName = CurrentMap;
        for (TActorIterator<AGCDayNightManager> It(World); It; ++It)
        {
            Save->GameDayIndex = It->GameDayIndex;
            Save->TimeOfDay = It->TimeOfDay;
            break;
        }
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
        {
            if (ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
            {
                Save->LastPlayerTransform = Character->GetActorTransform();
                Save->bHasReturnLocation = true;
            }
        }
    }
    return UGameplayStatics::SaveGameToSlot(Save, GetSlotName(), LocalUserIndex);
}

bool UGCProfileSaveSubsystem::DeleteProfileSave()
{
    return UGameplayStatics::DeleteGameInSlot(GetSlotName(), LocalUserIndex);
}

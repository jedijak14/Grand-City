#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/GCTypes.h"
#include "GCProfileSaveGame.generated.h"

/** Versioned single-player slot: citizen identity, finances, inventory, deeds, residence, and return location. */
UCLASS()
class GRANDCITY_API UGCProfileSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSaveVersion = 8;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    int32 SaveVersion = CurrentSaveVersion;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FDateTime LastSavedUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FString CitizenID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FString CitizenName;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FDateTime CitizenDateOfBirth;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    EGCCitizenSex CitizenSex = EGCCitizenSex::PreferNotToSay;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FString DriverLicenseType = TEXT("Class C");

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FString DriverLicenseNumber;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FDateTime LicenseIssuedUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FDateTime LicenseExpiryUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FDateTime CitizenCreatedUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FString CurrentAddress;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Appearance")
    FName CurrentOutfitID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 Money = 1500;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 DigitalBalance = 2500;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    EGCPaymentMethod SelectedPaymentMethod = EGCPaymentMethod::Cash;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 CreditUsed = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Career")
    EGCJob CurrentJob = EGCJob::Civilian;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Career")
    int32 ConductPoints = 100;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Career")
    int32 ShiftPoints = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Career")
    bool bPoliceShiftActive = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Career")
    bool bPoliceShiftFailed = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Identity")
    FString PoliceBadgeNumber;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Equipment")
    bool bHasAlcoholTestKit = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Equipment")
    bool bHasDrugTestKit = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police|Records")
    TArray<FGCPoliceCitationRecord> PoliceCitationRecords;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Progression")
    float StealthSkill = 0.15f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Properties")
    TArray<FGCPropertyRecord> Properties;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Properties")
    FString PrimaryResidenceID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle Ownership")
    TArray<FGCOwnedVehicleRecord> OwnedVehicles;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Flights")
    FGCFlightTicket ActiveTicket;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Inventory")
    TArray<FString> OwnedDeviceIDs;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Inventory")
    FString ActivePhoneModel;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Inventory")
    FString ActivePhoneOS;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World")
    FString LastMapName;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World")
    FTransform LastPlayerTransform = FTransform::Identity;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World")
    bool bHasReturnLocation = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World")
    int64 GameDayIndex = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World")
    float TimeOfDay = 10.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Progression")
    int64 LastProcessedGameDay = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Playtime")
    double TotalPlaytimeSeconds = 0.0;
};

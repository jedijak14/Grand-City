#pragma once

#include "CoreMinimal.h"
#include "GCTypes.generated.h"

class USkeletalMesh;

/** Which wheels receive engine torque. */
UENUM(BlueprintType)
enum class EGCVehicleDriveType : uint8
{
	RWD UMETA(DisplayName = "Rear Wheel Drive"),
	FWD UMETA(DisplayName = "Front Wheel Drive"),
	AWD UMETA(DisplayName = "All Wheel Drive")
};

UENUM(BlueprintType)
enum class EGCCameraMode : uint8
{
	Chase,
	Cockpit,
	Hood
};

/** Interactable world locations. Jobs/life-sim is intentionally milestone 2. */
UENUM(BlueprintType)
enum class EGCLocationType : uint8
{
	Hospital,
	Shop,
	PoliceStation,
	Airport,
	Marina,
	SkyscraperElevator
};

/** Placeholder roles - settable by dev command now, full job loops are milestone 3. */
UENUM(BlueprintType)
enum class EGCJob : uint8
{
	Civilian,
	Police,
	Medic,
	Taxi,
	ShopOwner,
	BorderPatrol,
	Hitman,
	President
};

/** Fictional in-world tender types; deliberately distinct from real payment brands. */
UENUM(BlueprintType)
enum class EGCPaymentMethod : uint8
{
	Cash,
	BaylineTap,
	CoastPass,
	HarborCard,
	BaylineCredit
};

UENUM(BlueprintType)
enum class EGCWeaponType : uint8
{
	Pistol,
	Rifle,
	SMG,
	Sniper,
    Taser
};

UENUM(BlueprintType)
enum class EGCCitizenSex : uint8
{
    Unspecified,
    Female,
    Male,
    NonBinary,
    PreferNotToSay
};

UENUM(BlueprintType)
enum class EGCPropertyType : uint8
{
    Apartment,
    Condominium,
    Townhouse,
    SingleFamilyHome,
    Commercial
};

UENUM(BlueprintType)
enum class EGCPropertyDeedStatus : uint8
{
    Purchased,
    Stolen
};

/** Persistent deed and valuation record. A property is keyed by a stable PropertyID, never an actor pointer. */
USTRUCT(BlueprintType)
struct FGCPropertyRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FString PropertyID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FString PropertyName;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FString StreetAddress;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    EGCPropertyType PropertyType = EGCPropertyType::SingleFamilyHome;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    EGCPropertyDeedStatus DeedStatus = EGCPropertyDeedStatus::Purchased;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FString OwnerCitizenID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FDateTime AcquiredUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int64 PurchasePrice = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int64 AssessedValue = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int32 Bedrooms = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int32 Bathrooms = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int32 FloorAreaSquareFeet = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Condition = 1.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int64 AnnualPropertyTax = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int64 OutstandingTax = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int64 MonthlyMaintenance = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property", meta = (ClampMin = "0"))
    int64 OutstandingMaintenance = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FDateTime LastTaxPaymentUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    FDateTime LastMaintenanceUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    int64 LastTaxAccrualGameDay = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    int64 LastMaintenanceAccrualGameDay = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Property")
    bool bOccupied = false;

    bool IsValidRecord() const { return !PropertyID.IsEmpty() && !PropertyName.IsEmpty(); }
};

UENUM(BlueprintType)
enum class EGCVehicleTitleStatus : uint8
{
    Clear,
    Stolen
};

USTRUCT(BlueprintType)
struct FGCOwnedVehicleRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FString VehicleID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FString DisplayName;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FString LicensePlate;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FString OwnerCitizenID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FString VehicleClassPath;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FString LastKnownMap;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    FTransform LastKnownTransform = FTransform::Identity;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    EGCVehicleTitleStatus RegistrationStatus = EGCVehicleTitleStatus::Clear;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Ownership")
    int64 PurchasePrice = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Condition")
    float HealthFraction = 1.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Condition")
    bool bWrecked = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Condition")
    float EngineTemperatureC = 88.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Condition")
    float RadiatorIntegrity = 1.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Vehicle|Condition")
    TArray<float> TireIntegrity;

    bool IsValidRecord() const { return !VehicleID.IsEmpty() && !LicensePlate.IsEmpty(); }
};

UENUM(BlueprintType)
enum class EGCOutfitCategory : uint8
{
    Casual,
    Formal,
    Swimwear,
    Sportswear,
    Uniform
};

UENUM(BlueprintType)
enum class EGCPoliceViolation : uint8
{
    Jaywalking,
    Littering,
    Loitering,
    ExpiredMeter,
    SidewalkParking,
    BlockingFireHydrant,
    AgainstTraffic,
    Speeding,
    IllegalTurn,
    BrokenTailLight,
    SmokeExhaust,
    RecklessDriving,
    DUI,
    UnlicensedDriving,
    FakeIdentification,
    ExpiredLicense
};

USTRUCT(BlueprintType)
struct FGCPoliceCitationRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    FString CitationID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    FString CitizenID;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    FString CitizenName;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    EGCPoliceViolation Violation = EGCPoliceViolation::Jaywalking;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    int32 FineAmount = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    FDateTime IssuedUtc;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    FString EvidenceNote;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Police")
    bool bPaid = false;
};

USTRUCT(BlueprintType)
struct FGCOutfitPreset
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    FName OutfitID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    EGCCitizenSex ApplicableSex = EGCCitizenSex::Unspecified;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    EGCOutfitCategory Category = EGCOutfitCategory::Casual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    TObjectPtr<USkeletalMesh> BodyMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    TObjectPtr<USkeletalMesh> ClothingMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit")
    bool bDefaultForSex = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outfit", meta = (EditCondition = "Category == EGCOutfitCategory::Swimwear"))
    bool bAppropriateCoverage = true;
};

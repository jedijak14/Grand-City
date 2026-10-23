#include "Core/GCPlayerState.h"
#include "Core/GCProfileSaveSubsystem.h"
#include "Components/GCHealthComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "World/GCDayNightManager.h"
#include "Vehicle/GCVehiclePawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    int64 GetCurrentGameDay(UWorld* World)
    {
        if (!World)
        {
            return 0;
        }
        for (TActorIterator<AGCDayNightManager> It(World); It; ++It)
        {
            return It->GameDayIndex;
        }
        return 0;
    }
}

AGCPlayerState::AGCPlayerState()
{
}

void AGCPlayerState::AddMoney(int32 Amount)
{
	Money = FMath::Max(0, Money + Amount);
    RequestPersistentSave();
}

bool AGCPlayerState::SpendMoney(int32 Amount)
{
	if (Amount < 0)
	{
		return false;
	}
	if (SelectedPaymentMethod == EGCPaymentMethod::Cash)
	{
		if (Money < Amount)
		{
			return false;
		}
		Money -= Amount;
		return true;
	}
	if (SelectedPaymentMethod == EGCPaymentMethod::BaylineCredit)
	{
		if (CreditUsed + Amount > CreditLimit)
		{
			return false;
		}
		CreditUsed += Amount;
		return true;
	}
	if (DigitalBalance < Amount)
	{
		return false;
	}
	DigitalBalance -= Amount;
	return true;
}

void AGCPlayerState::RefundMoney(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	if (SelectedPaymentMethod == EGCPaymentMethod::Cash)
	{
        Money = FMath::Max(0, Money + Amount);
	}
	else if (SelectedPaymentMethod == EGCPaymentMethod::BaylineCredit)
	{
		CreditUsed = FMath::Max(0, CreditUsed - Amount);
	}
	else
	{
		DigitalBalance = FMath::Max(0, DigitalBalance + Amount);
	}
    RequestPersistentSave();
}

bool AGCPlayerState::RepayCreditCard(int32 Amount)
{
	const int32 Payment = FMath::Min(FMath::Max(0, Amount), CreditUsed);
	if (Payment == 0 || DigitalBalance < Payment)
	{
		return false;
	}
	DigitalBalance -= Payment;
	CreditUsed -= Payment;
    RequestPersistentSave();
	return true;
}

void AGCPlayerState::CyclePaymentMethod()
{
	const uint8 MethodCount = 5;
	SelectedPaymentMethod = static_cast<EGCPaymentMethod>((static_cast<uint8>(SelectedPaymentMethod) + 1) % MethodCount);
    RequestPersistentSave();
}

FString AGCPlayerState::GetPaymentMethodLabel() const
{
	switch (SelectedPaymentMethod)
	{
	case EGCPaymentMethod::BaylineTap: return TEXT("Bayline Tap");
	case EGCPaymentMethod::CoastPass: return TEXT("CoastPass");
	case EGCPaymentMethod::HarborCard: return TEXT("Harbor Card");
	case EGCPaymentMethod::BaylineCredit: return TEXT("Bayline Credit");
	case EGCPaymentMethod::Cash:
	default: return TEXT("Cash");
	}
}

bool AGCPlayerState::CreateCitizenIdentity(const FString& DisplayName)
{
    return CreateCitizenProfile(DisplayName, FDateTime(1995, 1, 1), EGCCitizenSex::Unspecified, TEXT("Class C"));
}

bool AGCPlayerState::CreateCitizenProfile(const FString& DisplayName, const FDateTime& DateOfBirth,
    EGCCitizenSex Sex, const FString& LicenseType)
{
    FString CleanName = DisplayName;
    CleanName.TrimStartAndEndInline();
    FString CleanLicense = LicenseType;
    CleanLicense.TrimStartAndEndInline();
    if (CleanName.Len() < 2 || CleanLicense.IsEmpty() || DateOfBirth >= FDateTime::UtcNow())
    {
        return false;
    }
    const FDateTime Now = FDateTime::UtcNow();
    int32 Age = Now.GetYear() - DateOfBirth.GetYear();
    if (Now.GetMonth() < DateOfBirth.GetMonth()
        || (Now.GetMonth() == DateOfBirth.GetMonth() && Now.GetDay() < DateOfBirth.GetDay()))
    {
        --Age;
    }
    if (Age < 16 || Age > 110)
    {
        return false;
    }
    CitizenName = CleanName.Left(24);
    CitizenID = FString::Printf(TEXT("GL-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper());
    CitizenDateOfBirth = DateOfBirth;
    CitizenSex = Sex;
    DriverLicenseType = CleanLicense.Left(24);
    DriverLicenseNumber = FString::Printf(TEXT("CA-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(10).ToUpper());
    LicenseIssuedUtc = Now;
    LicenseExpiryUtc = Now.AddYears(5);
    CitizenCreatedUtc = Now;
    RequestPersistentSave();
    return true;
}

int32 AGCPlayerState::TickJobIncome(float DeltaSeconds)
{
	if (CurrentJob != LastPaidJob)
	{
		LastPaidJob = CurrentJob;
		JobPayTimer = 0.0f;
	}
	if (CurrentJob == EGCJob::Civilian)
	{
		return 0;
	}
	JobPayTimer += FMath::Max(0.0f, DeltaSeconds);
	if (JobPayTimer < 120.0f)
	{
		return 0;
	}
	JobPayTimer = FMath::Fmod(JobPayTimer, 120.0f);
	int32 Wage = 200;
	switch (CurrentJob)
	{
	case EGCJob::Police: Wage = 350; break;
	case EGCJob::Medic: Wage = 300; break;
	case EGCJob::Taxi: Wage = 220; break;
	case EGCJob::ShopOwner: Wage = 260; break;
	case EGCJob::BorderPatrol: Wage = 400; break;
	case EGCJob::Hitman: Wage = 280; break;
	case EGCJob::President: Wage = 600; break;
	default: return 0;
	}
	DigitalBalance += Wage;
	return Wage;
}

void AGCPlayerState::AddWantedStars(int32 Count)
{
	const int32 Old = WantedStars;
	WantedStars = FMath::Clamp(WantedStars + Count, 0, 5);
	HiddenTime = 0.0f;
	if (WantedStars != Old)
	{
		OnWantedChanged.Broadcast(WantedStars);
	}
}

bool AGCPlayerState::OwnsProperty(const FString& PropertyID) const
{
	return !PropertyID.IsEmpty() && OwnedPropertyIDs.ContainsByPredicate([&PropertyID](const FString& OwnedID)
	{
		return OwnedID.Equals(PropertyID, ESearchCase::IgnoreCase);
	});
}

bool AGCPlayerState::AddOwnedProperty(const FString& PropertyID, bool bStolen)
{
    FGCPropertyRecord Record;
    Record.PropertyID = PropertyID;
    Record.PropertyName = PropertyID;
    Record.OwnerCitizenID = CitizenID;
    Record.DeedStatus = bStolen ? EGCPropertyDeedStatus::Stolen : EGCPropertyDeedStatus::Purchased;
    Record.AcquiredUtc = FDateTime::UtcNow();
    Record.LastTaxPaymentUtc = Record.AcquiredUtc;
    Record.LastMaintenanceUtc = Record.AcquiredUtc;
    Record.LastTaxAccrualGameDay = GetCurrentGameDay(GetWorld());
    Record.LastMaintenanceAccrualGameDay = Record.LastTaxAccrualGameDay;
    return AcquireProperty(Record);
}

bool AGCPlayerState::AcquireProperty(const FGCPropertyRecord& Property)
{
    if (!Property.IsValidRecord() || OwnsProperty(Property.PropertyID))
    {
        return false;
    }
    FGCPropertyRecord NewRecord = Property;
    NewRecord.OwnerCitizenID = CitizenID;
    if (NewRecord.AcquiredUtc.GetYear() < 2)
    {
        NewRecord.AcquiredUtc = FDateTime::UtcNow();
    }
    if (NewRecord.LastTaxPaymentUtc.GetYear() < 2)
    {
        NewRecord.LastTaxPaymentUtc = NewRecord.AcquiredUtc;
    }
    if (NewRecord.LastMaintenanceUtc.GetYear() < 2)
    {
        NewRecord.LastMaintenanceUtc = NewRecord.AcquiredUtc;
    }
    NewRecord.Condition = FMath::Clamp(NewRecord.Condition, 0.0f, 1.0f);
    const int64 CurrentGameDay = GetCurrentGameDay(GetWorld());
    if (NewRecord.LastTaxAccrualGameDay == 0 && CurrentGameDay > 0)
    {
        NewRecord.LastTaxAccrualGameDay = CurrentGameDay;
    }
    if (NewRecord.LastMaintenanceAccrualGameDay == 0 && CurrentGameDay > 0)
    {
        NewRecord.LastMaintenanceAccrualGameDay = CurrentGameDay;
    }
    OwnedPropertyIDs.Add(NewRecord.PropertyID);
    if (NewRecord.DeedStatus == EGCPropertyDeedStatus::Stolen)
    {
        StolenPropertyIDs.Add(NewRecord.PropertyID);
    }
    PropertyPortfolio.Add(NewRecord);
    RequestPersistentSave();
    return true;
}

bool AGCPlayerState::SetPrimaryResidence(const FString& PropertyID)
{
    const FGCPropertyRecord* Found = PropertyPortfolio.FindByPredicate([this, &PropertyID](const FGCPropertyRecord& Record)
    {
        return Record.PropertyID.Equals(PropertyID, ESearchCase::IgnoreCase) && Record.OwnerCitizenID == CitizenID;
    });
    if (!Found)
    {
        return false;
    }
    PrimaryResidenceID = Found->PropertyID;
    CurrentAddress = Found->StreetAddress;
    for (FGCPropertyRecord& Record : PropertyPortfolio)
    {
        Record.bOccupied = Record.PropertyID.Equals(PrimaryResidenceID, ESearchCase::IgnoreCase);
    }
    RequestPersistentSave();
    return true;
}

int64 AGCPlayerState::GetPropertySaleOffer(const FString& PropertyID) const
{
    const FGCPropertyRecord* Found = PropertyPortfolio.FindByPredicate([&PropertyID](const FGCPropertyRecord& Record)
    {
        return Record.PropertyID.Equals(PropertyID, ESearchCase::IgnoreCase);
    });
    if (!Found)
    {
        return 0;
    }
    const int64 BaseValue = Found->AssessedValue > 0 ? Found->AssessedValue : Found->PurchasePrice;
    const double DeedFactor = Found->DeedStatus == EGCPropertyDeedStatus::Stolen ? 0.20 : 0.72;
    const double ConditionFactor = FMath::Clamp(static_cast<double>(Found->Condition), 0.1, 1.0);
    const int64 GrossOffer = FMath::RoundToInt64(static_cast<double>(BaseValue) * DeedFactor * ConditionFactor);
    return FMath::Max<int64>(0, GrossOffer - Found->OutstandingTax - Found->OutstandingMaintenance);
}

bool AGCPlayerState::SellProperty(const FString& PropertyID)
{
    const int64 SaleOffer = GetPropertySaleOffer(PropertyID);
    if (SaleOffer <= 0)
    {
        return false;
    }
    Money = FMath::Max(0, Money + static_cast<int32>(FMath::Min<int64>(SaleOffer, MAX_int32)));
    PropertyPortfolio.RemoveAll([&PropertyID](const FGCPropertyRecord& Record)
    {
        return Record.PropertyID.Equals(PropertyID, ESearchCase::IgnoreCase);
    });
    OwnedPropertyIDs.RemoveAll([&PropertyID](const FString& OwnedID)
    {
        return OwnedID.Equals(PropertyID, ESearchCase::IgnoreCase);
    });
    StolenPropertyIDs.RemoveAll([&PropertyID](const FString& OwnedID)
    {
        return OwnedID.Equals(PropertyID, ESearchCase::IgnoreCase);
    });
    if (PrimaryResidenceID.Equals(PropertyID, ESearchCase::IgnoreCase))
    {
        PrimaryResidenceID.Empty();
        CurrentAddress.Empty();
    }
    RequestPersistentSave();
    return true;
}

bool AGCPlayerState::PayPropertyTax(const FString& PropertyID)
{
    FGCPropertyRecord* Found = PropertyPortfolio.FindByPredicate([&PropertyID](const FGCPropertyRecord& Record)
    {
        return Record.PropertyID.Equals(PropertyID, ESearchCase::IgnoreCase);
    });
    if (!Found || Found->OutstandingTax <= 0 || Found->OutstandingTax > MAX_int32
        || !SpendMoney(static_cast<int32>(Found->OutstandingTax)))
    {
        return false;
    }
    Found->OutstandingTax = 0;
    Found->LastTaxPaymentUtc = FDateTime::UtcNow();
    RequestPersistentSave();
    return true;
}

bool AGCPlayerState::PayPropertyMaintenance(const FString& PropertyID)
{
    FGCPropertyRecord* Found = PropertyPortfolio.FindByPredicate([&PropertyID](const FGCPropertyRecord& Record)
    {
        return Record.PropertyID.Equals(PropertyID, ESearchCase::IgnoreCase);
    });
    if (!Found)
    {
        return false;
    }
    const int64 ServiceCost = FMath::Max<int64>(Found->OutstandingMaintenance, Found->MonthlyMaintenance);
    if (ServiceCost <= 0 || ServiceCost > MAX_int32 || !SpendMoney(static_cast<int32>(ServiceCost)))
    {
        return false;
    }
    Found->OutstandingMaintenance = 0;
    Found->Condition = FMath::Clamp(Found->Condition + 0.2f, 0.0f, 1.0f);
    Found->LastMaintenanceUtc = FDateTime::UtcNow();
    RequestPersistentSave();
    return true;
}

bool AGCPlayerState::ProcessPropertyDay(int64 NewGameDayIndex)
{
    if (NewGameDayIndex <= LastProcessedGameDay)
    {
        return false;
    }
    bool bChanged = false;
    for (FGCPropertyRecord& Record : PropertyPortfolio)
    {
        const int64 TaxMonths = FMath::Max<int64>(0, (NewGameDayIndex - Record.LastTaxAccrualGameDay) / 30);
        if (TaxMonths > 0)
        {
            const int64 MonthlyTax = Record.AnnualPropertyTax > 0 ? FMath::Max<int64>(1, Record.AnnualPropertyTax / 12) : 0;
            if (MonthlyTax > 0)
            {
                const int64 Room = MAX_int64 - Record.OutstandingTax;
                Record.OutstandingTax += MonthlyTax > Room / TaxMonths ? Room : MonthlyTax * TaxMonths;
            }
            Record.LastTaxAccrualGameDay += TaxMonths * 30;
            bChanged = true;
        }

        const int64 MaintenanceMonths = FMath::Max<int64>(0, (NewGameDayIndex - Record.LastMaintenanceAccrualGameDay) / 30);
        if (MaintenanceMonths > 0)
        {
            if (Record.MonthlyMaintenance > 0)
            {
                const int64 Room = MAX_int64 - Record.OutstandingMaintenance;
                Record.OutstandingMaintenance += Record.MonthlyMaintenance > Room / MaintenanceMonths
                    ? Room : Record.MonthlyMaintenance * MaintenanceMonths;
            }
            Record.Condition = FMath::Max(0.05f, Record.Condition - static_cast<float>(MaintenanceMonths) * 0.015f);
            Record.LastMaintenanceAccrualGameDay += MaintenanceMonths * 30;
            bChanged = true;
        }
    }
    LastProcessedGameDay = NewGameDayIndex;
    if (bChanged)
    {
        RequestPersistentSave();
    }
    return bChanged;
}

void AGCPlayerState::RegisterOwnedVehicle(AGCVehiclePawn* Vehicle, bool bStolen)
{
    if (!IsValid(Vehicle))
    {
        return;
    }
    if (Vehicle->VehicleID.IsEmpty())
    {
        Vehicle->VehicleID = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToUpper();
    }
    if (Vehicle->LicensePlate.IsEmpty())
    {
        Vehicle->LicensePlate = FString::Printf(TEXT("GC-%04d"), FMath::RandRange(0, 9999));
    }
    Vehicle->bIsPlayerOwned = true;
    Vehicle->bWasStolen |= bStolen;

    FGCOwnedVehicleRecord* Record = OwnedVehicleRecords.FindByPredicate([Vehicle](const FGCOwnedVehicleRecord& Existing)
    {
        return Existing.VehicleID.Equals(Vehicle->VehicleID, ESearchCase::IgnoreCase);
    });
    if (!Record)
    {
        Record = &OwnedVehicleRecords.AddDefaulted_GetRef();
    }
    Record->VehicleID = Vehicle->VehicleID;
    Record->DisplayName = Vehicle->DisplayName;
    Record->LicensePlate = Vehicle->LicensePlate;
    Record->OwnerCitizenID = CitizenID;
    Record->VehicleClassPath = Vehicle->GetClass()->GetPathName();
    Record->RegistrationStatus = Vehicle->bWasStolen ? EGCVehicleTitleStatus::Stolen : EGCVehicleTitleStatus::Clear;
    Record->HealthFraction = Vehicle->Health && Vehicle->Health->MaxHealth > 0.0f
        ? FMath::Clamp(Vehicle->Health->Health / Vehicle->Health->MaxHealth, 0.0f, 1.0f) : 1.0f;
    Record->bWrecked = Vehicle->bWrecked;
    Record->EngineTemperatureC = Vehicle->EngineTemperatureC;
    Record->RadiatorIntegrity = FMath::Clamp(Vehicle->RadiatorIntegrity, 0.0f, 1.0f);
    Record->TireIntegrity.Reset();
    for (const FGCVehicleTireCondition& Tire : Vehicle->TireConditions)
    {
        Record->TireIntegrity.Add(FMath::Clamp(Tire.Integrity, 0.0f, 1.0f));
    }
    Record->LastKnownTransform = Vehicle->GetActorTransform();
    Record->LastKnownMap = GetWorld() ? UGameplayStatics::GetCurrentLevelName(GetWorld(), true) : FString();
    RequestPersistentSave();
}

void AGCPlayerState::SyncOwnedVehicles()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    const FString CurrentMap = UGameplayStatics::GetCurrentLevelName(World, true);
    bool bChanged = false;
    for (TActorIterator<AGCVehiclePawn> It(World); It; ++It)
    {
        AGCVehiclePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->bIsPlayerOwned)
        {
            continue;
        }
        FGCOwnedVehicleRecord* Record = OwnedVehicleRecords.FindByPredicate([Vehicle](const FGCOwnedVehicleRecord& Existing)
        {
            return Existing.VehicleID.Equals(Vehicle->VehicleID, ESearchCase::IgnoreCase);
        });
        if (!Record)
        {
            Record = &OwnedVehicleRecords.AddDefaulted_GetRef();
        }
        Record->VehicleID = Vehicle->VehicleID;
        Record->DisplayName = Vehicle->DisplayName;
        Record->LicensePlate = Vehicle->LicensePlate;
        Record->OwnerCitizenID = CitizenID;
        Record->VehicleClassPath = Vehicle->GetClass()->GetPathName();
        Record->RegistrationStatus = Vehicle->bWasStolen ? EGCVehicleTitleStatus::Stolen : EGCVehicleTitleStatus::Clear;
        Record->HealthFraction = Vehicle->Health && Vehicle->Health->MaxHealth > 0.0f
            ? FMath::Clamp(Vehicle->Health->Health / Vehicle->Health->MaxHealth, 0.0f, 1.0f) : 1.0f;
        Record->bWrecked = Vehicle->bWrecked;
        Record->EngineTemperatureC = Vehicle->EngineTemperatureC;
        Record->RadiatorIntegrity = FMath::Clamp(Vehicle->RadiatorIntegrity, 0.0f, 1.0f);
        Record->TireIntegrity.Reset();
        for (const FGCVehicleTireCondition& Tire : Vehicle->TireConditions)
        {
            Record->TireIntegrity.Add(FMath::Clamp(Tire.Integrity, 0.0f, 1.0f));
        }
        Record->LastKnownTransform = Vehicle->GetActorTransform();
        Record->LastKnownMap = CurrentMap;
        bChanged = true;
    }
    if (bChanged)
    {
        RequestPersistentSave();
    }
}

void AGCPlayerState::SetTicket(const FGCFlightTicket& NewTicket)
{
    ActiveTicket = NewTicket;
    RequestPersistentSave();
}

void AGCPlayerState::ClearTicket()
{
    ActiveTicket = FGCFlightTicket();
    RequestPersistentSave();
}

void AGCPlayerState::RequestPersistentSave()
{
    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UGCProfileSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UGCProfileSaveSubsystem>() : nullptr;
    if (SaveSubsystem)
    {
        SaveSubsystem->SavePlayerState(this);
    }
}

void AGCPlayerState::StartPoliceShift()
{
    if (CurrentJob != EGCJob::Police)
    {
        return;
    }
    ConductPoints = 100;
    ShiftPoints = 0;
    bPoliceShiftActive = true;
    bPoliceShiftFailed = false;
    bHasAlcoholTestKit = true;
    bHasDrugTestKit = true;
    if (PoliceBadgeNumber.IsEmpty())
    {
        PoliceBadgeNumber = FString::Printf(TEXT("GC-%05d"), FMath::RandRange(1, 99999));
    }
    RequestPersistentSave();
}

bool AGCPlayerState::IsPoliceOnDuty() const
{
    return CurrentJob == EGCJob::Police && bPoliceShiftActive && !bPoliceShiftFailed && ConductPoints > 0;
}

void AGCPlayerState::ApplyConductPenalty(int32 Points)
{
    if (!IsPoliceOnDuty() || Points <= 0)
    {
        return;
    }
    ConductPoints = FMath::Max(0, ConductPoints - Points);
    if (ConductPoints == 0)
    {
        bPoliceShiftActive = false;
        bPoliceShiftFailed = true;
    }
    RequestPersistentSave();
}

void AGCPlayerState::AwardShiftPoints(int32 Points)
{
    if (!IsPoliceOnDuty() || Points <= 0)
    {
        return;
    }
    ShiftPoints = FMath::Clamp(ShiftPoints + Points, 0, MAX_int32);
    RequestPersistentSave();
}

void AGCPlayerState::RecordPoliceCitation(const FGCPoliceCitationRecord& Citation)
{
    if (Citation.CitationID.IsEmpty())
    {
        return;
    }
    PoliceCitationRecords.Add(Citation);
    RequestPersistentSave();
}

bool AGCPlayerState::OwnsDevice(const FString& DeviceID) const
{
	return !DeviceID.IsEmpty() && OwnedDeviceIDs.ContainsByPredicate([&DeviceID](const FString& OwnedID)
	{
		return OwnedID.Equals(DeviceID, ESearchCase::IgnoreCase);
	});
}

void AGCPlayerState::AddOwnedDevice(const FString& DeviceID, bool bPhone, const FString& PhoneModel, const FString& PhoneOS)
{
	if (DeviceID.IsEmpty())
	{
		return;
	}
	if (!OwnsDevice(DeviceID))
	{
		OwnedDeviceIDs.Add(DeviceID);
	}
	if (bPhone)
	{
		ActivePhoneModel = PhoneModel;
		ActivePhoneOS = PhoneOS;
	}
    RequestPersistentSave();
}

void AGCPlayerState::ClearWanted()
{
	if (WantedStars != 0)
	{
		WantedStars = 0;
		OnWantedChanged.Broadcast(0);
	}
	HiddenTime = 0.0f;
}

void AGCPlayerState::TickWantedDecay(float DeltaSeconds, bool bSeenByPolice)
{
	if (WantedStars <= 0)
	{
		return;
	}
	if (bSeenByPolice)
	{
		HiddenTime = 0.0f;
		return;
	}
	HiddenTime += DeltaSeconds;
	if (HiddenTime >= WantedDecaySeconds)
	{
		HiddenTime = 0.0f;
		WantedStars = FMath::Max(0, WantedStars - 1);
		OnWantedChanged.Broadcast(WantedStars);
	}
}

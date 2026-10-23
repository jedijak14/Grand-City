#include "World/GCRealEstateActor.h"

#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "Core/GCCrimeSystemSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "World/GCDayNightManager.h"
#include "Kismet/GameplayStatics.h"

AGCRealEstateActor::AGCRealEstateActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ListingTrigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(380.0f, 380.0f, 220.0f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	SignMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ListingSign"));
	SignMesh->SetupAttachment(Trigger);
	SignMesh->SetRelativeLocation(FVector(0.0f, 230.0f, 110.0f));
	SignMesh->SetRelativeScale3D(FVector(1.5f, 0.12f, 0.72f));
	SignMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGCRealEstateActor::BeginPlay()
{
	Super::BeginPlay();
	if (PropertyID.IsEmpty())
	{
		PropertyID = GetFName().ToString();
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGCRealEstateActor::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AGCRealEstateActor::OnTriggerEnd);
}

void AGCRealEstateActor::Interact(AGCCharacter* Character)
{
	if (!Character)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PlayerState = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PlayerState)
	{
		Character->ShowSubtitle(TEXT("The property office cannot verify your account."), 4.0f);
		return;
	}
    if (PlayerState->OwnsProperty(PropertyID))
    {
        if (!PlayerState->PrimaryResidenceID.Equals(PropertyID, ESearchCase::IgnoreCase))
        {
            if (PlayerState->SetPrimaryResidence(PropertyID))
            {
                Character->ShowSubtitle(FString::Printf(TEXT("%s is now your registered primary residence at %s."),
                    *PropertyName, *StreetAddress), 5.0f);
            }
            return;
        }
        const FGCPropertyRecord* OwnedRecord = PlayerState->PropertyPortfolio.FindByPredicate([this](const FGCPropertyRecord& Record)
        {
            return Record.PropertyID.Equals(PropertyID, ESearchCase::IgnoreCase);
        });
        if (OwnedRecord)
        {
            const int64 SaleOffer = PlayerState->GetPropertySaleOffer(PropertyID);
            Character->ShowSubtitle(FString::Printf(
                TEXT("%s | %s | %d bd / %d ba | %d sq ft | condition %.0f%% | bills $%s | sale offer $%s. V sell, T tax, M maintenance."),
                *PropertyName, *StreetAddress, OwnedRecord->Bedrooms, OwnedRecord->Bathrooms,
                OwnedRecord->FloorAreaSquareFeet, OwnedRecord->Condition * 100.0f,
                *LexToString(OwnedRecord->OutstandingTax + OwnedRecord->OutstandingMaintenance), *LexToString(SaleOffer)), 8.0f);
        }
        return;
    }

    if (IsValid(Realtor) && Realtor->bDead)
    {
        FGCPropertyRecord Record;
        Record.PropertyID = PropertyID;
        Record.PropertyName = PropertyName;
        Record.StreetAddress = StreetAddress;
        Record.PropertyType = PropertyType;
        Record.DeedStatus = EGCPropertyDeedStatus::Stolen;
        Record.AcquiredUtc = FDateTime::UtcNow();
        Record.PurchasePrice = 0;
        Record.AssessedValue = AssessedValue;
        Record.Bedrooms = Bedrooms;
        Record.Bathrooms = Bathrooms;
        Record.FloorAreaSquareFeet = FloorAreaSquareFeet;
        Record.Condition = FMath::Clamp(InitialCondition, 0.0f, 1.0f);
        Record.AnnualPropertyTax = AnnualPropertyTax;
        Record.MonthlyMaintenance = MonthlyMaintenance;
        Record.LastTaxPaymentUtc = Record.AcquiredUtc;
        Record.LastMaintenanceUtc = Record.AcquiredUtc;
        Record.OutstandingTax = FMath::Max<int64>(0, AnnualPropertyTax / 12);
        Record.OutstandingMaintenance = FMath::Max<int64>(0, MonthlyMaintenance);
        if (PlayerState->AcquireProperty(Record))
        {
            UGCCrimeSystemSubsystem* CrimeSystem = GetWorld()->GetSubsystem<UGCCrimeSystemSubsystem>();
            const bool bWitnessed = CrimeSystem
                ? CrimeSystem->ReportCrime(Character, GetActorLocation(), EGCCrimeType::IllegalPropertyClaim)
                : false;
            Character->ShowSubtitle(FString::Printf(
                TEXT("You seized %s without a deed. %s"), *PropertyName,
                bWitnessed ? TEXT("A witness raised the alarm.") : TEXT("No one saw the seizure.")), 5.0f);
        }
        return;
    }

	if (!IsValid(Realtor))
	{
		Character->ShowSubtitle(TEXT("No realtor is available. This listing cannot be transferred right now."), 4.0f);
		return;
	}
	if (!PlayerState->SpendMoney(PurchasePrice))
	{
		Character->ShowSubtitle(FString::Printf(TEXT("%s costs $%d. You don't have enough money."), *PropertyName, PurchasePrice), 5.0f);
		return;
	}
    FGCPropertyRecord Record;
    Record.PropertyID = PropertyID;
    Record.PropertyName = PropertyName;
    Record.StreetAddress = StreetAddress;
    Record.PropertyType = PropertyType;
    Record.DeedStatus = EGCPropertyDeedStatus::Purchased;
    Record.AcquiredUtc = FDateTime::UtcNow();
    Record.PurchasePrice = PurchasePrice;
    Record.AssessedValue = AssessedValue;
    Record.Bedrooms = Bedrooms;
    Record.Bathrooms = Bathrooms;
    Record.FloorAreaSquareFeet = FloorAreaSquareFeet;
    Record.Condition = FMath::Clamp(InitialCondition, 0.0f, 1.0f);
    Record.AnnualPropertyTax = AnnualPropertyTax;
    Record.MonthlyMaintenance = MonthlyMaintenance;
    Record.LastTaxPaymentUtc = Record.AcquiredUtc;
    Record.LastMaintenanceUtc = Record.AcquiredUtc;
    Record.OutstandingTax = FMath::Max<int64>(0, AnnualPropertyTax / 12);
    Record.OutstandingMaintenance = FMath::Max<int64>(0, MonthlyMaintenance);
    if (!PlayerState->AcquireProperty(Record))
    {
        PlayerState->RefundMoney(PurchasePrice);
        Character->ShowSubtitle(TEXT("The deed could not be recorded; your money was refunded."), 4.0f);
        return;
    }
    Character->ShowSubtitle(FString::Printf(TEXT("Deed recorded for %s, %s. Use E here to register it as your primary residence."),
        *PropertyName, *StreetAddress), 5.0f);
}

void AGCRealEstateActor::SellOwnedProperty(AGCCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    AGCPlayerState* PlayerState = Character->GetPlayerState<AGCPlayerState>();
    const int64 Offer = PlayerState ? PlayerState->GetPropertySaleOffer(PropertyID) : 0;
    if (PlayerState && Offer > 0 && PlayerState->SellProperty(PropertyID))
    {
        Character->ShowSubtitle(FString::Printf(TEXT("Property sold for $%s after outstanding bills."), *LexToString(Offer)), 5.0f);
    }
    else
    {
        Character->ShowSubtitle(TEXT("The property cannot be sold; verify ownership and outstanding bills."), 4.0f);
    }
}

void AGCRealEstateActor::PayPropertyTax(AGCCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    AGCPlayerState* PlayerState = Character->GetPlayerState<AGCPlayerState>();
    if (PlayerState && PlayerState->PayPropertyTax(PropertyID))
    {
        Character->ShowSubtitle(FString::Printf(TEXT("Property tax paid for %s."), *PropertyName), 4.0f);
    }
    else
    {
        Character->ShowSubtitle(TEXT("No property tax was paid. Check ownership, balance, and the outstanding bill."), 4.0f);
    }
}

void AGCRealEstateActor::MaintainOwnedProperty(AGCCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    AGCPlayerState* PlayerState = Character->GetPlayerState<AGCPlayerState>();
    if (PlayerState && PlayerState->PayPropertyMaintenance(PropertyID))
    {
        Character->ShowSubtitle(FString::Printf(TEXT("Maintenance completed at %s; condition improved."), *PropertyName), 4.0f);
    }
    else
    {
        Character->ShowSubtitle(TEXT("Maintenance was not completed. Check ownership, balance, and service cost."), 4.0f);
    }
}

void AGCRealEstateActor::OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		Character->SetCurrentProperty(this);
        const AGCPlayerState* PlayerState = Character->GetPlayerState<AGCPlayerState>();
        if (PlayerState && PlayerState->OwnsProperty(PropertyID))
        {
            Character->ShowSubtitle(FString::Printf(TEXT("%s | %s | Press E to manage your deed."), *PropertyName, *StreetAddress), 5.0f);
        }
        else if (IsValid(Realtor) && Realtor->bDead)
        {
            Character->ShowSubtitle(FString::Printf(TEXT("%s — Press E to seize the unguarded listing (illegal)."), *PropertyName), 5.0f);
        }
        else
        {
            Character->ShowSubtitle(FString::Printf(TEXT("%s | %s | %d bd / %d ba | $%d — Press E to buy."),
                *PropertyName, *StreetAddress, Bedrooms, Bathrooms, PurchasePrice), 5.0f);
        }
	}
}

void AGCRealEstateActor::OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		if (Character->CurrentProperty == this)
		{
			Character->SetCurrentProperty(nullptr);
		}
	}
}

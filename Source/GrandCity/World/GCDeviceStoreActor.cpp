#include "World/GCDeviceStoreActor.h"

#include "Character/GCCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"

AGCDeviceStoreActor::AGCDeviceStoreActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StoreTrigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(340.0f, 340.0f, 220.0f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);

	StoreSign = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StoreSign"));
	StoreSign->SetupAttachment(Trigger);
	StoreSign->SetRelativeLocation(FVector(0.0f, 220.0f, 180.0f));
	StoreSign->SetRelativeScale3D(FVector(2.2f, 0.12f, 0.55f));
	StoreSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGCDeviceStoreActor::BeginPlay()
{
	Super::BeginPlay();
	if (Offers.IsEmpty())
	{
		AddDefaultOffers();
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGCDeviceStoreActor::OnTriggerBegin);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AGCDeviceStoreActor::OnTriggerEnd);
}

void AGCDeviceStoreActor::AddDefaultOffers()
{
	auto AddOffer = [this](const TCHAR* ID, const TCHAR* Name, const TCHAR* OS, EGCDeviceCategory Category, int32 Price)
	{
		FGCDeviceOffer Offer;
		Offer.DeviceID = ID;
		Offer.DisplayName = Name;
		Offer.OperatingSystem = OS;
		Offer.Category = Category;
		Offer.Price = Price;
		Offers.Add(MoveTemp(Offer));
	};
	AddOffer(TEXT("PHONE-ASTER-S9"), TEXT("Aster S9"), TEXT("AsterOS"), EGCDeviceCategory::Phone, 899);
	AddOffer(TEXT("PHONE-NORTHSTAR-FOLD"), TEXT("Northstar Fold"), TEXT("LumaOS"), EGCDeviceCategory::Phone, 1199);
	AddOffer(TEXT("TABLET-TIDE-SLATE"), TEXT("Tide Slate 12"), TEXT("AsterOS"), EGCDeviceCategory::Tablet, 699);
	AddOffer(TEXT("DESKTOP-HARBOR-TOWER"), TEXT("Harbor Tower Desktop"), TEXT("HarborOS"), EGCDeviceCategory::Desktop, 1599);
}

FString AGCDeviceStoreActor::DescribeSelectedOffer() const
{
	if (!Offers.IsValidIndex(SelectedOfferIndex))
	{
		return TEXT("No devices are available.");
	}
	const FGCDeviceOffer& Offer = Offers[SelectedOfferIndex];
	const TCHAR* Category = Offer.Category == EGCDeviceCategory::Phone ? TEXT("PHONE")
		: (Offer.Category == EGCDeviceCategory::Tablet ? TEXT("TABLET") : TEXT("DESKTOP"));
	return FString::Printf(TEXT("%s  /  %s  /  $%d"), Category, *Offer.DisplayName, Offer.Price);
}

void AGCDeviceStoreActor::CycleOffer()
{
	if (Offers.IsEmpty())
	{
		return;
	}
	SelectedOfferIndex = (SelectedOfferIndex + 1) % Offers.Num();
	if (AGCCharacter* Character = CurrentCustomer.Get())
	{
		Character->ShowSubtitle(FString::Printf(TEXT("Device selected: %s. Press E to purchase."), *DescribeSelectedOffer()), 4.0f);
	}
}

void AGCDeviceStoreActor::PurchaseSelected(AGCCharacter* Character)
{
	if (!Character || !Offers.IsValidIndex(SelectedOfferIndex))
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	AGCPlayerState* PlayerState = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
	if (!PlayerState)
	{
		Character->ShowSubtitle(TEXT("The store cannot verify your account."), 4.0f);
		return;
	}
	const FGCDeviceOffer& Offer = Offers[SelectedOfferIndex];
	if (PlayerState->OwnsDevice(Offer.DeviceID))
	{
		Character->ShowSubtitle(TEXT("You already own this device."), 4.0f);
		return;
	}
	if (!PlayerState->SpendMoney(Offer.Price))
	{
		Character->ShowSubtitle(FString::Printf(TEXT("%s costs $%d. You don't have enough money."), *Offer.DisplayName, Offer.Price), 4.0f);
		return;
	}
	const bool bIsPhone = Offer.Category == EGCDeviceCategory::Phone;
	PlayerState->AddOwnedDevice(Offer.DeviceID, bIsPhone, Offer.DisplayName, Offer.OperatingSystem);
	Character->ShowSubtitle(FString::Printf(TEXT("Purchased %s (%s). %s"), *Offer.DisplayName, *Offer.OperatingSystem,
		bIsPhone ? TEXT("It is now your active phone.") : TEXT("It has been added to your device inventory.")), 5.0f);
}

void AGCDeviceStoreActor::OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		CurrentCustomer = Character;
		Character->SetCurrentDeviceStore(this);
		Character->ShowSubtitle(FString::Printf(TEXT("Device shop: %s. Tab to browse, E to purchase."), *DescribeSelectedOffer()), 5.0f);
	}
}

void AGCDeviceStoreActor::OnTriggerEnd(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		if (Character->CurrentDeviceStore == this)
		{
			Character->SetCurrentDeviceStore(nullptr);
		}
		if (CurrentCustomer.Get() == Character)
		{
			CurrentCustomer.Reset();
		}
	}
}

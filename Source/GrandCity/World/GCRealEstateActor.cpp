#include "World/GCRealEstateActor.h"

#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"
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
		Character->ShowSubtitle(FString::Printf(TEXT("You already own %s."), *PropertyName), 4.0f);
		return;
	}

	if (IsValid(Realtor) && Realtor->bDead)
	{
		if (PlayerState->AddOwnedProperty(PropertyID, true))
		{
			bool bWitnessed = false;
			bool bScreamPlayed = false;
			TArray<AActor*> Nearby;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGCPedestrian::StaticClass(), Nearby);
			for (AActor* Actor : Nearby)
			{
				AGCPedestrian* Witness = Cast<AGCPedestrian>(Actor);
				if (Witness && Witness != Realtor && Witness->CanWitnessCrimeAt(GetActorLocation()))
				{
					bWitnessed = true;
					const bool bPlayScream = !Witness->bIsPolice && !bScreamPlayed;
					Witness->ReactToWitnessedCrime(Character->GetActorLocation(), bPlayScream);
					bScreamPlayed |= bPlayScream;
				}
			}
			if (bWitnessed)
			{
				PlayerState->AddWantedStars(3);
			}
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
	if (!PlayerState->AddOwnedProperty(PropertyID, false))
	{
		PlayerState->RefundMoney(PurchasePrice);
		Character->ShowSubtitle(TEXT("The deed could not be recorded; your money was refunded."), 4.0f);
		return;
	}
	Character->ShowSubtitle(FString::Printf(TEXT("Deed transferred: %s is now yours."), *PropertyName), 5.0f);
}

void AGCRealEstateActor::OnTriggerBegin(UPrimitiveComponent* Overlapped, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AGCCharacter* Character = Cast<AGCCharacter>(Other))
	{
		Character->SetCurrentProperty(this);
		if (IsValid(Realtor) && Realtor->bDead)
		{
			Character->ShowSubtitle(FString::Printf(TEXT("%s — Press E to claim the unguarded listing (illegal)."), *PropertyName), 5.0f);
		}
		else
		{
			Character->ShowSubtitle(FString::Printf(TEXT("%s — Press E to buy for $%d."), *PropertyName, PurchasePrice), 5.0f);
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

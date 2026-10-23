#include "Character/GCPedestrian.h"
#include "AI/GCPedAIController.h"
#include "Character/GCCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/GCHealthComponent.h"
#include "Components/GCVoiceLineComponent.h"
#include "Components/GCWeaponComponent.h"
#include "Components/GCClothPhysicsLibrary.h"
#include "Core/GCGameUserSettings.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "World/GCDayNightManager.h"
#include "World/GCDroppedItemActor.h"
#include "Core/GCPlayerState.h"
#include "Core/GCCrimeSystemSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicle/GCVehiclePawn.h"

AGCPedestrian::AGCPedestrian()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AGCPedAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);

	Health = CreateDefaultSubobject<UGCHealthComponent>(TEXT("Health"));
	Weapon = CreateDefaultSubobject<UGCWeaponComponent>(TEXT("Weapon"));
	Weapon->Damage = 9.0f; // police sidearm - dangerous in numbers, survivable solo
	Weapon->FireRateRPM = 240.0f;
	Weapon->SpreadDegrees = 2.6f;

	Voice = CreateDefaultSubobject<UGCVoiceLineComponent>(TEXT("Voice"));
    CarriedItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarriedItemMesh"));
    CarriedItemMesh->SetupAttachment(GetMesh());
    CarriedItemMesh->SetRelativeLocation(FVector(18.0f, 14.0f, 82.0f));
    CarriedItemMesh->SetRelativeScale3D(FVector(0.18f));
    CarriedItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CarriedItemMesh->SetHiddenInGame(true);
    CarriedItemMesh->SetVisibility(false);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
	Move->NavAgentProps.AgentRadius = 38.0f;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 420.0f;
}

void AGCPedestrian::BeginPlay()
{
	Super::BeginPlay();
    const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings();
    UGCClothPhysicsLibrary::ApplyClothSimulation(this,
        !Settings || Settings->bClothAndHairPhysics, 1.0f);
    if (CitizenID.IsEmpty())
    {
        static const TCHAR* FirstNames[] = { TEXT("Avery"), TEXT("Jordan"), TEXT("Riley"), TEXT("Morgan"), TEXT("Casey"), TEXT("Taylor"), TEXT("Sam"), TEXT("Drew"), TEXT("Alex"), TEXT("Cameron"), TEXT("Jamie"), TEXT("Quinn"), TEXT("Parker"), TEXT("Reese"), TEXT("Skyler"), TEXT("Robin") };
        static const TCHAR* LastNames[] = { TEXT("Reyes"), TEXT("Kim"), TEXT("Patel"), TEXT("Rivera"), TEXT("Chen"), TEXT("Brooks"), TEXT("Miller"), TEXT("Nguyen"), TEXT("Garcia"), TEXT("Price"), TEXT("Diaz"), TEXT("Cooper"), TEXT("Bennett"), TEXT("Morgan"), TEXT("Foster"), TEXT("Wong") };
        CitizenSex = FMath::RandRange(0, 1) == 0 ? EGCCitizenSex::Female : EGCCitizenSex::Male;
        CitizenName = FString::Printf(TEXT("%s %s"), FirstNames[FMath::RandRange(0, UE_ARRAY_COUNT(FirstNames) - 1)],
            LastNames[FMath::RandRange(0, UE_ARRAY_COUNT(LastNames) - 1)]);
        CitizenID = FString::Printf(TEXT("GC-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper());
        bIDIsFake = FMath::FRand() < 0.015f;
        const FDateTime Now = FDateTime::UtcNow();
        CitizenDateOfBirth = Now.AddYears(-FMath::RandRange(18, 76)).AddDays(-FMath::RandRange(0, 364));
        DriverLicenseType = FMath::FRand() < 0.14f ? TEXT("Class M") : TEXT("Class C");
        DriverLicenseNumber = FString::Printf(TEXT("CA-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(10).ToUpper());
        DriverLicenseExpiryUtc = FMath::FRand() < 0.08f
            ? Now.AddYears(-FMath::RandRange(1, 3))
            : Now.AddYears(FMath::RandRange(1, 5));
        bHasOpenWarrant = FMath::FRand() < 0.055f;
        bHasPriorCriminalRecord = FMath::FRand() < 0.16f;
        if (bHasPriorCriminalRecord)
        {
            static const TCHAR* Records[] = { TEXT("petty theft"), TEXT("disorderly conduct"), TEXT("trespassing"), TEXT("unpaid citation") };
            PriorCriminalRecordSummary = Records[FMath::RandRange(0, UE_ARRAY_COUNT(Records) - 1)];
        }
        bHasWeapon = FMath::FRand() < 0.025f;
        bHasContraband = FMath::FRand() < 0.075f;
        bNervous = FMath::FRand() < 0.18f;
        bSmellsOfCannabis = FMath::FRand() < 0.06f;
        bDilatedPupils = FMath::FRand() < 0.035f;
        bIntoxicatedByAlcohol = FMath::FRand() < 0.035f;
        bIntoxicatedByDrugs = FMath::FRand() < 0.025f;
        bSlurredSpeech = bIntoxicatedByAlcohol && FMath::FRand() < 0.82f;
        bUnsteadyGait = (bIntoxicatedByAlcohol || bIntoxicatedByDrugs) && FMath::FRand() < 0.72f;
    }
	if (WalletCash <= 0)
	{
		WalletCash = FMath::RandRange(35, 220);
	}
    if (!bIsPolice && HeldItem == EGCHandheldItem::None)
    {
        const float ItemRoll = FMath::FRand();
        if (bHoldingCoffee || ItemRoll < 0.12f)
        {
            HeldItem = EGCHandheldItem::Coffee;
            bHoldingCoffee = true;
        }
        else if (ItemRoll < 0.20f)
        {
            HeldItem = EGCHandheldItem::Phone;
        }
        else if (ItemRoll < 0.27f)
        {
            HeldItem = EGCHandheldItem::Bag;
        }
    }
    if (IsValid(CarriedItemMesh))
    {
        UStaticMesh* ItemAsset = nullptr;
        switch (HeldItem)
        {
        case EGCHandheldItem::Coffee: ItemAsset = CoffeeCupMesh; break;
        case EGCHandheldItem::Phone: ItemAsset = PhonePropMesh; break;
        case EGCHandheldItem::Bag: ItemAsset = HandbagMesh; break;
        default: break;
        }
        if (ItemAsset)
        {
            CarriedItemMesh->SetStaticMesh(ItemAsset);
            CarriedItemMesh->SetVisibility(true);
            CarriedItemMesh->SetHiddenInGame(false);
        }
    }
    if (bUnsteadyGait && GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.62f;
    }
    if (Voice && bSlurredSpeech)
    {
        Voice->PitchMultiplier = 0.79f;
    }
    if (Health)
	{
		Health->OnDeath.AddDynamic(this, &AGCPedestrian::OnDeath);
	}
}

void AGCPedestrian::ShowSubtitleToNearbyPlayer(const FString& Text) const
{
	if (Text.IsEmpty())
	{
		return;
	}
	if (AGCCharacter* PlayerChar = Cast<AGCCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		if (FVector::Distance(PlayerChar->GetActorLocation(), GetActorLocation()) < 1100.0f)
		{
			PlayerChar->ShowSubtitle(Text, 4.0f);
		}
	}
}

void AGCPedestrian::SayLine(FName LineID)
{
	if (bDead || !Voice)
	{
		return;
	}
	FString Subtitle;
	if (Voice->PlayLine(LineID, Subtitle))
	{
		ShowSubtitleToNearbyPlayer(Subtitle);
	}
}

bool AGCPedestrian::HasProbableCause() const
{
    return bHasOpenWarrant || bHasWeapon || bHasContraband || bSmellsOfCannabis || bNervous || bDilatedPupils;
}

void AGCPedestrian::StopForPolice()
{
    bStoppedForPolice = true;
    if (AGCPedAIController* PedController = Cast<AGCPedAIController>(GetController()))
    {
        PedController->StopMovement();
    }
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->MaxWalkSpeed = 0.0f;
    }
}

void AGCPedestrian::InteractWithPlayer(AGCCharacter* Character)
{
    if (!IsValid(Character) || bDead)
    {
        return;
    }
    const AGCPlayerState* PlayerState = Character->GetPlayerState<AGCPlayerState>();
    if (PlayerState && PlayerState->IsPoliceOnDuty())
    {
        RequestIdentification(Character);
        return;
    }
    FString Subtitle;
    if ((!Voice || !Voice->PlayRandomWithPrefix(TEXT("AmbientGreeting"), Subtitle))
        && (!Voice || !Voice->PlayRandomWithPrefix(TEXT("Ambient"), Subtitle)))
    {
        Subtitle = FString::Printf(TEXT("%s: Hey there. Hope you're having a good day."), *CitizenName);
    }
    Character->ShowSubtitle(Subtitle, 4.0f);
}

bool AGCPedestrian::InitiatePoliceStop(AGCCharacter* Officer)
{
    const AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || bDead || bArrested)
    {
        return false;
    }
    StopForPolice();
    Officer->ShowSubtitle(FString::Printf(TEXT("%s stopped for officer contact. Ensure reasonable suspicion before a frisk or search."), *CitizenName), 4.0f);
    return true;
}

bool AGCPedestrian::RequestIdentification(AGCCharacter* Officer)
{
    const AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || bDead || bArrested)
    {
        if (Officer)
        {
            Officer->ShowSubtitle(TEXT("ID checks require an active police shift."));
        }
        return false;
    }
    if (!bStoppedForPolice && bNervous && FMath::FRand() < 0.18f)
    {
        FleeFrom(Officer->GetActorLocation());
        Officer->ShowSubtitle(FString::Printf(TEXT("%s fled during an ID check. Pursuit initiated."), *CitizenName), 4.0f);
        OfficerState->AwardShiftPoints(2);
        return false;
    }
    StopForPolice();
    const FDateTime Now = FDateTime::UtcNow();
    const bool bLicenseValid = !DriverLicenseNumber.IsEmpty() && DriverLicenseExpiryUtc > Now;
    const FString Warrant = bHasOpenWarrant ? TEXT("OPEN WARRANT") : TEXT("NO WARRANT");
    const FString Record = bHasPriorCriminalRecord ? PriorCriminalRecordSummary : TEXT("no prior record");
    const FString IDStatus = bIDIsFake ? TEXT("ID NOT FOUND — POSSIBLE COUNTERFEIT") : TEXT("ID VERIFIED");
    Officer->ShowSubtitle(FString::Printf(TEXT("DATABASE: %s | ID %s (%s) | DOB %s | DL %s (%s) | %s | Record: %s"),
        *CitizenName, *CitizenID, *IDStatus, *CitizenDateOfBirth.ToString(TEXT("%Y-%m-%d")), *DriverLicenseNumber,
        bLicenseValid ? TEXT("VALID") : TEXT("EXPIRED"), *Warrant, *Record), 9.0f);
    const_cast<AGCPlayerState*>(OfficerState)->AwardShiftPoints(3);
    return true;
}

bool AGCPedestrian::Frisk(AGCCharacter* Officer)
{
    AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || bDead || bArrested)
    {
        return false;
    }
    if (!bStoppedForPolice)
    {
        OfficerState->ApplyConductPenalty(8);
        Officer->ShowSubtitle(TEXT("Frisk cancelled: the person has not been stopped."));
        return false;
    }
    if (!HasProbableCause())
    {
        OfficerState->ApplyConductPenalty(20);
        Officer->ShowSubtitle(TEXT("Unjustified frisk. Conduct points deducted."));
        return false;
    }
    if (bHasWeapon)
    {
        Officer->ShowSubtitle(FString::Printf(TEXT("Frisk found a concealed weapon on %s. You have grounds to arrest."), *CitizenName), 5.0f);
        OfficerState->AwardShiftPoints(12);
    }
    else
    {
        Officer->ShowSubtitle(FString::Printf(TEXT("Frisk complete: no weapon found on %s."), *CitizenName), 4.0f);
        OfficerState->AwardShiftPoints(4);
    }
    return true;
}

bool AGCPedestrian::SearchPersonAndBag(AGCCharacter* Officer)
{
    AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || bDead || bArrested)
    {
        return false;
    }
    if (!bStoppedForPolice || !HasProbableCause())
    {
        OfficerState->ApplyConductPenalty(25);
        Officer->ShowSubtitle(TEXT("Unlawful search. Establish probable cause and stop the person first."));
        return false;
    }
    const bool bFoundEvidence = bHasWeapon || bHasContraband;
    if (bHasWeapon)
    {
        bHasWeapon = false;
    }
    if (bHasContraband)
    {
        bHasContraband = false;
    }
    if (bFoundEvidence)
    {
        Officer->ShowSubtitle(FString::Printf(TEXT("Search found and seized contraband/evidence from %s."), *CitizenName), 5.0f);
        OfficerState->AwardShiftPoints(15);
    }
    else
    {
        Officer->ShowSubtitle(FString::Printf(TEXT("Search complete: no contraband found on %s."), *CitizenName), 4.0f);
        OfficerState->AwardShiftPoints(5);
    }
    return true;
}

bool AGCPedestrian::Arrest(AGCCharacter* Officer)
{
    AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || bDead || bArrested)
    {
        return false;
    }
    const bool bLawfulBasis = bHasOpenWarrant || bHasWeapon || bHasContraband || bDUIConfirmed;
    if (!bStoppedForPolice || !bLawfulBasis)
    {
        OfficerState->ApplyConductPenalty(35);
        Officer->ShowSubtitle(TEXT("Unlawful arrest attempt. A stop and arrestable legal basis are required."));
        return false;
    }
    bArrested = true;
    StopForPolice();
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->DisableMovement();
    }
    OfficerState->AwardShiftPoints(25);
    Officer->ShowSubtitle(FString::Printf(TEXT("%s arrested on a verified warrant or evidence-based charge. Request transport."), *CitizenName), 6.0f);
    return true;
}

bool AGCPedestrian::ReleaseFromPoliceStop(AGCCharacter* Officer)
{
    const AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || !bStoppedForPolice || bArrested)
    {
        return false;
    }
    bStoppedForPolice = false;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = WalkSpeed;
        Movement->SetMovementMode(MOVE_Walking);
    }
    if (AGCPedAIController* PedController = Cast<AGCPedAIController>(GetController()))
    {
        PedController->ResumeFromPoliceStop();
    }
    Officer->ShowSubtitle(FString::Printf(TEXT("%s released from the stop."), *CitizenName), 3.0f);
    return true;
}

void AGCPedestrian::InterviewAboutCrime(AGCCharacter* Officer)
{
    const AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty())
    {
        if (Officer)
        {
            Officer->ShowSubtitle(TEXT("Witness interviews require an active police shift."));
        }
        return;
    }
    const FString Statement = LastWitnessStatement.IsEmpty()
        ? FString::Printf(TEXT("%s says they did not see anything relevant."), *CitizenName)
        : FString::Printf(TEXT("%s says: %s"), *CitizenName, *LastWitnessStatement);
    const FString ObservedSpeech = bSlurredSpeech
        ? TEXT(" Speech is slurred; consider a lawful impairment assessment.") : TEXT("");
    Officer->ShowSubtitle(Statement + ObservedSpeech, 7.0f);
    const_cast<AGCPlayerState*>(OfficerState)->AwardShiftPoints(LastWitnessStatement.IsEmpty() ? 1 : 5);
}

bool AGCPedestrian::RunAlcoholTest(AGCCharacter* Officer)
{
    AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || !OfficerState->bHasAlcoholTestKit
        || bDead || bArrested || !bStoppedForPolice)
    {
        if (Officer) Officer->ShowSubtitle(TEXT("Alcohol test requires an active shift, test kit, and stopped subject."));
        return false;
    }
    if (!bSlurredSpeech && !bUnsteadyGait)
    {
        OfficerState->ApplyConductPenalty(8);
        Officer->ShowSubtitle(TEXT("No articulable alcohol-impairment cues. Test declined; conduct points deducted."));
        return false;
    }
    if (bIntoxicatedByAlcohol)
    {
        bDUIConfirmed = true;
        FGCPoliceCitationRecord Citation;
        Citation.CitationID = FString::Printf(TEXT("CIT-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper());
        Citation.CitizenID = CitizenID;
        Citation.CitizenName = CitizenName;
        Citation.Violation = EGCPoliceViolation::DUI;
        Citation.FineAmount = 750;
        Citation.IssuedUtc = FDateTime::UtcNow();
        Citation.EvidenceNote = TEXT("Alcohol screening positive; officer documented slurred speech or unsteady gait.");
        OfficerState->RecordPoliceCitation(Citation);
        OfficerState->AwardShiftPoints(15);
        Officer->ShowSubtitle(FString::Printf(TEXT("Alcohol test POSITIVE for %s. DUI evidence recorded; request transport."), *CitizenName), 6.0f);
        return true;
    }
    OfficerState->AwardShiftPoints(4);
    Officer->ShowSubtitle(FString::Printf(TEXT("Alcohol test negative for %s. Documented observations; release or continue lawful inquiry."), *CitizenName), 6.0f);
    return true;
}

bool AGCPedestrian::RunDrugTest(AGCCharacter* Officer)
{
    AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || !OfficerState->bHasDrugTestKit
        || bDead || bArrested || !bStoppedForPolice)
    {
        if (Officer) Officer->ShowSubtitle(TEXT("Drug test requires an active shift, test kit, and stopped subject."));
        return false;
    }
    if (!bSmellsOfCannabis && !bDilatedPupils && !bUnsteadyGait)
    {
        OfficerState->ApplyConductPenalty(8);
        Officer->ShowSubtitle(TEXT("No articulable drug-impairment cues. Test declined; conduct points deducted."));
        return false;
    }
    if (bIntoxicatedByDrugs)
    {
        bDUIConfirmed = true;
        FGCPoliceCitationRecord Citation;
        Citation.CitationID = FString::Printf(TEXT("CIT-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper());
        Citation.CitizenID = CitizenID;
        Citation.CitizenName = CitizenName;
        Citation.Violation = EGCPoliceViolation::DUI;
        Citation.FineAmount = 900;
        Citation.IssuedUtc = FDateTime::UtcNow();
        Citation.EvidenceNote = TEXT("Drug impairment test positive; officer documented observable indicators.");
        OfficerState->RecordPoliceCitation(Citation);
        OfficerState->AwardShiftPoints(15);
        Officer->ShowSubtitle(FString::Printf(TEXT("Drug test POSITIVE for %s. Impaired-driving evidence recorded; request transport."), *CitizenName), 6.0f);
        return true;
    }
    OfficerState->AwardShiftPoints(4);
    Officer->ShowSubtitle(FString::Printf(TEXT("Drug test negative for %s. Document the result and continue lawfully."), *CitizenName), 6.0f);
    return true;
}

bool AGCPedestrian::IssueCitation(AGCCharacter* Officer, EGCPoliceViolation Violation)
{
    AGCPlayerState* OfficerState = Officer ? Officer->GetPlayerState<AGCPlayerState>() : nullptr;
    if (!Officer || !OfficerState || !OfficerState->IsPoliceOnDuty() || bDead || bArrested)
    {
        return false;
    }
    bool bObserved = false;
    int32 Fine = 0;
    const TCHAR* ViolationName = TEXT("unknown offense");
    switch (Violation)
    {
    case EGCPoliceViolation::Jaywalking: bObserved = bObservedJaywalking; Fine = 75; ViolationName = TEXT("jaywalking"); break;
    case EGCPoliceViolation::Littering: bObserved = bObservedLittering; Fine = 100; ViolationName = TEXT("littering"); break;
    case EGCPoliceViolation::Loitering: bObserved = bObservedLoitering; Fine = 50; ViolationName = TEXT("loitering"); break;
    case EGCPoliceViolation::FakeIdentification: bObserved = bIDIsFake; Fine = 350; ViolationName = TEXT("presenting false identification"); break;
    case EGCPoliceViolation::ExpiredLicense: bObserved = DriverLicenseExpiryUtc <= FDateTime::UtcNow(); Fine = 180; ViolationName = TEXT("expired license"); break;
    default: break;
    }
    if (!bObserved)
    {
        OfficerState->ApplyConductPenalty(12);
        Officer->ShowSubtitle(TEXT("Citation rejected: there is no observed evidence for that violation."));
        return false;
    }
    FGCPoliceCitationRecord Citation;
    Citation.CitationID = FString::Printf(TEXT("CIT-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8).ToUpper());
    Citation.CitizenID = CitizenID;
    Citation.CitizenName = CitizenName;
    Citation.Violation = Violation;
    Citation.FineAmount = Fine;
    Citation.IssuedUtc = FDateTime::UtcNow();
    Citation.EvidenceNote = FString::Printf(TEXT("Observed %s by officer %s."), ViolationName, *OfficerState->CitizenID);
    if (WalletCash >= Fine)
    {
        WalletCash -= Fine;
        Citation.bPaid = true;
    }
    OfficerState->RecordPoliceCitation(Citation);
    OfficerState->AwardShiftPoints(10);
    Officer->ShowSubtitle(FString::Printf(TEXT("Citation issued to %s: %s, $%d%s."), *CitizenName, ViolationName, Fine,
        Citation.bPaid ? TEXT(" paid") : TEXT(" unpaid")), 5.0f);
    return true;
}

void AGCPedestrian::MaybeSayAmbient()
{
	if (bDead || !Voice || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextAmbientVoiceTime)
	{
		return;
	}
	NextAmbientVoiceTime = Now + FMath::FRandRange(10.0f, 22.0f);
	if (FMath::FRand() > 0.35f)
	{
		return;
	}
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn || FVector::Distance(PlayerPawn->GetActorLocation(), GetActorLocation()) > 1100.0f)
	{
		return;
	}
	FString Subtitle;
	if (Voice->PlayRandomWithPrefix(bIsPolice ? TEXT("Police") : TEXT("Ambient"), Subtitle))
	{
		ShowSubtitleToNearbyPlayer(Subtitle);
	}
}

void AGCPedestrian::FleeFrom(const FVector& DangerLocation)
{
	if (AGCPedAIController* AI = Cast<AGCPedAIController>(GetController()))
	{
		AI->StartFlee(DangerLocation);
	}
}

void AGCPedestrian::FireAtActor(AActor* Target)
{
	if (!Target || bDead || !bIsPolice)
	{
		return;
	}
	const FVector Start = GetActorLocation() + FVector(0, 0, 145.0f) + GetActorForwardVector() * 40.0f;
	const FVector TargetPoint = Target->GetActorLocation() + FVector(0, 0, 110.0f);
	const FVector Dir = (TargetPoint - Start).GetSafeNormal();
	SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
	FVector HitPoint;
	Weapon->FireOnce(Start, Dir, true, HitPoint);
}

bool AGCPedestrian::CanWitnessCrimeAt(const FVector& IncidentLocation) const
{
	if (bDead || !GetWorld() || FVector::Distance(GetActorLocation(), IncidentLocation) > 2400.0f)
	{
		return false;
	}
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 145.0f);
	const FVector End = IncidentLocation + FVector(0.0f, 0.0f, 85.0f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CivilianWitnessLOS), false, this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
		|| FVector::DistSquared(Hit.ImpactPoint, End) < FMath::Square(130.0f);
}

void AGCPedestrian::ReactToWitnessedCrime(const FVector& IncidentLocation, bool bScream)
{
	if (bDead)
	{
		return;
	}
    if (bScream && !bIsPolice && PanicScreamSound)
    {
        if (IsValid(PanicScreamAudio))
        {
            PanicScreamAudio->Stop();
        }
        PanicScreamAudio = UGameplayStatics::SpawnSoundAtLocation(this, PanicScreamSound, GetActorLocation());
        if (IsValid(PanicScreamAudio))
        {
            UpdatePanicAudioOcclusion();
            GetWorldTimerManager().SetTimer(PanicAudioOcclusionTimer, this,
                &AGCPedestrian::UpdatePanicAudioOcclusion, 0.15f, true);
        }
    }
	FString Subtitle;
	const FName VoiceLine = bIsPolice ? FName(TEXT("PoliceCrimeWitness")) : FName(TEXT("CivilianScream"));
	if (!Voice || !Voice->PlayLine(VoiceLine, Subtitle))
	{
		Subtitle = bIsPolice ? TEXT("Police! Stop! Call it in!") : TEXT("Help! Somebody call the police!");
	}
    LastWitnessStatement = FString::Printf(TEXT("I %s an incident near coordinates %s."),
        bScream ? TEXT("saw") : TEXT("heard"), *IncidentLocation.ToCompactString());
	ShowSubtitleToNearbyPlayer(Subtitle);
	FleeFrom(IncidentLocation);
}

void AGCPedestrian::RecordVehicleCrashWitness(const FVector& CrashLocation, bool bWasVictim, const FString& VehicleDescription)
{
    if (bDead || FVector::DistSquared(GetActorLocation(), CrashLocation) > FMath::Square(2800.0f))
    {
        return;
    }
    LastIncidentLocation = CrashLocation;
    bWasVehicleCrashVictim = bWasVehicleCrashVictim || bWasVictim;
    LastWitnessStatement = bWasVictim
        ? FString::Printf(TEXT("I was struck by %s near %s."), *VehicleDescription, *CrashLocation.ToCompactString())
        : FString::Printf(TEXT("I witnessed a vehicle collision involving %s near %s."), *VehicleDescription, *CrashLocation.ToCompactString());
}

void AGCPedestrian::UpdatePanicAudioOcclusion()
{
    if (!IsValid(PanicScreamAudio) || !PanicScreamAudio->IsPlaying() || !GetWorld())
    {
        GetWorldTimerManager().ClearTimer(PanicAudioOcclusionTimer);
        PanicScreamAudio = nullptr;
        return;
    }
    APawn* Listener = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!Listener)
    {
        return;
    }
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 130.0f);
    const FVector End = Listener->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianVoiceOcclusion), false, this);
    Params.AddIgnoredActor(Listener);
    const bool bOccluded = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    PanicScreamAudio->SetLowPassFilterEnabled(true);
    PanicScreamAudio->SetLowPassFilterFrequency(bOccluded ? 1100.0f : 18000.0f);
    const float Distance = FVector::Distance(GetActorLocation(), Listener->GetActorLocation());
    PanicScreamAudio->SetVolumeMultiplier(FMath::GetMappedRangeValueClamped(
        FVector2D(400.0f, 2600.0f), FVector2D(1.0f, 0.08f), Distance));
}

void AGCPedestrian::QueuePoliceReport(int32 WantedStars, float DelaySeconds)
{
    if (bDead || bIsPolice || WantedStars <= 0 || !GetWorld())
    {
        return;
    }
    PendingWantedStars = FMath::Max(PendingWantedStars, FMath::Clamp(WantedStars, 1, 5));
    GetWorldTimerManager().ClearTimer(CrimeReportTimer);
    GetWorldTimerManager().SetTimer(CrimeReportTimer, this, &AGCPedestrian::CompletePoliceReport,
        FMath::Clamp(DelaySeconds, 1.0f, 20.0f), false);
}

void AGCPedestrian::CompletePoliceReport()
{
    if (bDead || PendingWantedStars <= 0)
    {
        return;
    }
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (AGCPlayerState* PS = PC->GetPlayerState<AGCPlayerState>())
        {
            PS->AddWantedStars(PendingWantedStars);
            if (AGCCharacter* PlayerCharacter = Cast<AGCCharacter>(PC->GetPawn()))
            {
                PlayerCharacter->ShowSubtitle(TEXT("A witness called police. Your wanted level increased."), 4.0f);
            }
        }
    }
    PendingWantedStars = 0;
}

bool AGCPedestrian::AttemptPickpocket(AGCCharacter* Thief)
{
    if (!Thief || bDead || bIsPolice || bHasBeenPickpocketed)
    {
        return false;
    }
    const float Distance = FVector::Distance(Thief->GetActorLocation(), GetActorLocation());
    if (Distance > 190.0f)
    {
        Thief->ShowSubtitle(TEXT("Get within arm's reach to try pickpocketing."), 3.0f);
        return false;
    }

    const FVector TowardThief = (Thief->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    const bool bFacingThief = FVector::DotProduct(GetActorForwardVector(), TowardThief) > 0.25f;
    const bool bBehindVictim = FVector::DotProduct(GetActorForwardVector(), TowardThief) < -0.25f;
    const float ApproachSpeed = Thief->GetVelocity().Size2D();
    const bool bQuietApproach = Thief->bIsCrouched && ApproachSpeed < 150.0f && bBehindVictim;

    float NightFactor = 0.0f;
    for (TActorIterator<AGCDayNightManager> It(GetWorld()); It; ++It)
    {
        NightFactor = FMath::Clamp(It->NightFactor, 0.0f, 1.0f);
        break;
    }
    float Skill = 0.0f;
    if (AGCPlayerState* PlayerState = Thief->GetPlayerState<AGCPlayerState>())
    {
        Skill = FMath::Clamp(PlayerState->StealthSkill, 0.0f, 1.0f);
    }

    const float SpeedExposure = FMath::GetMappedRangeValueClamped(FVector2D(100.0f, 500.0f), FVector2D(0.0f, 0.30f), ApproachSpeed);
    const float DistanceExposure = FMath::GetMappedRangeValueClamped(FVector2D(35.0f, 190.0f), FVector2D(0.18f, 0.0f), Distance);
    float DetectionChance = 0.12f + DistanceExposure + SpeedExposure + (bFacingThief ? 0.48f : 0.0f)
        + (Thief->bIsCrouched ? 0.0f : 0.28f) + (bBehindVictim ? -0.12f : 0.20f)
        - NightFactor * 0.12f - Skill * 0.34f;
    if (bQuietApproach)
    {
        DetectionChance -= 0.10f;
    }
    DetectionChance = FMath::Clamp(DetectionChance, 0.03f, 0.97f);

    if (FMath::FRand() < DetectionChance)
    {
        bHasBeenPickpocketed = true;
        ReactToWitnessedCrime(Thief->GetActorLocation(), true);
        if (UGCCrimeSystemSubsystem* CrimeSystem = GetWorld()->GetSubsystem<UGCCrimeSystemSubsystem>())
        {
            CrimeSystem->ReportCrime(Thief, GetActorLocation(), EGCCrimeType::Pickpocket, 700.0f);
        }
        Thief->ShowSubtitle(TEXT("They caught your hand. The victim shouted and nearby witnesses may report you."), 5.0f);
        return false;
    }

    bHasBeenPickpocketed = true;
    const int32 StolenCash = FMath::Max(0, WalletCash);
    if (AGCPlayerState* PlayerState = Thief->GetPlayerState<AGCPlayerState>())
    {
        PlayerState->AddMoney(StolenCash);
        PlayerState->StealthSkill = FMath::Clamp(PlayerState->StealthSkill + 0.015f, 0.0f, 1.0f);
    }
    Thief->ShowSubtitle(FString::Printf(TEXT("Quietly lifted $%d. Stealth improved slightly."), StolenCash), 4.0f);
    return true;
}

void AGCPedestrian::KnockDown(float DurationSeconds)
{
	if (bDead || bKnockedDown)
	{
		return;
	}
	bKnockedDown = true;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();

    const bool bSpillCoffee = bHoldingCoffee || HeldItem == EGCHandheldItem::Coffee;
    DropCarriedItem(bSpillCoffee);
    bHoldingCoffee = false;
    HeldItem = EGCHandheldItem::None;
	GetWorldTimerManager().SetTimer(KnockdownTimer, this, &AGCPedestrian::RecoverFromKnockdown,
		FMath::Clamp(DurationSeconds, 0.8f, 6.0f), false);
}

void AGCPedestrian::DropCarriedItem(bool bSpillCoffee)
{
    if (bSpillCoffee)
    {
        FHitResult GroundHit;
        const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
        const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 500.0f);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CoffeeStainGround), false, this);
        const bool bFoundGround = GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params);
        const FVector SpillLocation = bFoundGround ? GroundHit.ImpactPoint : GetActorLocation();
        if (CoffeeSpillFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CoffeeSpillFX,
                SpillLocation + FVector(0.0f, 0.0f, 4.0f));
        }
        if (CoffeeSpillSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, CoffeeSpillSound, SpillLocation);
        }
        if (bFoundGround && CoffeeSpillStainMaterial)
        {
            UGameplayStatics::SpawnDecalAtLocation(this, CoffeeSpillStainMaterial,
                FVector(100.0f, 100.0f, 4.0f), GroundHit.ImpactPoint + GroundHit.ImpactNormal * 2.0f,
                GroundHit.ImpactNormal.Rotation(), 50.0f);
        }
    }

    if (IsValid(CarriedItemMesh) && CarriedItemMesh->GetStaticMesh() && GetWorld())
    {
        const FVector DropLocation = CarriedItemMesh->GetComponentLocation();
        const FRotator DropRotation = CarriedItemMesh->GetComponentRotation();
        AGCDroppedItemActor* DroppedItem = GetWorld()->SpawnActor<AGCDroppedItemActor>(
            AGCDroppedItemActor::StaticClass(), DropLocation, DropRotation);
        if (IsValid(DroppedItem))
        {
            const FVector TossImpulse = GetVelocity() * 0.45f + GetActorForwardVector() * 180.0f + FVector(0.0f, 0.0f, 260.0f);
            DroppedItem->InitializeDroppedItem(CarriedItemMesh->GetStaticMesh(), TossImpulse);
        }
        CarriedItemMesh->SetVisibility(false);
        CarriedItemMesh->SetHiddenInGame(true);
    }
}

void AGCPedestrian::RecoverFromKnockdown()
{
	if (bDead)
	{
		return;
	}
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bKnockedDown = false;
}

void AGCPedestrian::OnDeath(AActor* Killer)
{
	bDead = true;
    GetWorldTimerManager().ClearTimer(PanicAudioOcclusionTimer);
    GetWorldTimerManager().ClearTimer(CrimeReportTimer);
	StartRagdoll();

	// Homicides only raise wanted level when a live witness has line of sight.
    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    const APawn* SuspectPawn = Cast<APawn>(Killer);
    const bool bKilledByPlayer = Killer && (Killer == PlayerPawn
        || (SuspectPawn && SuspectPawn->GetController() && SuspectPawn->GetController()->IsPlayerController()));
    if (bKilledByPlayer)
    {
        if (UGCCrimeSystemSubsystem* CrimeSystem = GetWorld()->GetSubsystem<UGCCrimeSystemSubsystem>())
        {
            CrimeSystem->ReportCrime(const_cast<APawn*>(SuspectPawn), GetActorLocation(), EGCCrimeType::Homicide);
        }
    }

	SetLifeSpan(25.0f); // body cleanup; spawner will replace the population
}

void AGCPedestrian::StartRagdoll()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	if (AController* C = GetController())
	{
		C->UnPossess();
	}
}

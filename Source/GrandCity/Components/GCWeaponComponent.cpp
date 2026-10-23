#include "Components/GCWeaponComponent.h"
#include "Character/GCPedestrian.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UGCWeaponComponent::UGCWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	AmmoInMagazine = MagazineSize;
}

bool UGCWeaponComponent::CanFire() const
{
	if (bReloading || AmmoInMagazine <= 0 || !GetWorld())
	{
		return false;
	}
	const double Interval = 60.0 / FMath::Max(30.0f, FireRateRPM);
	return (GetWorld()->GetTimeSeconds() - LastFireTime) >= Interval;
}

float UGCWeaponComponent::FireOnce(const FVector& Start, const FVector& Direction, bool bAiming, FVector& OutHitPoint)
{
	OutHitPoint = Start + Direction * Range;
	if (!CanFire())
	{
		if (AmmoInMagazine <= 0 && !bReloading)
		{
			StartReload();
		}
		return 0.0f;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	LastFireTime = World->GetTimeSeconds();
	--AmmoInMagazine;

	const float SpreadRad = FMath::DegreesToRadians(SpreadDegrees * (bAiming ? 0.25f : 1.0f));
	const FVector ShotDir = FMath::VRandCone(Direction.GetSafeNormal(), SpreadRad);
	const FVector End = Start + ShotDir * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponTrace), false, Owner);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	if (bHit)
	{
		OutHitPoint = Hit.ImpactPoint;
		AController* InstigatorController = nullptr;
		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			InstigatorController = OwnerPawn->GetController();
		}
        const float AppliedDamage = WeaponType == EGCWeaponType::Taser ? 4.0f : Damage;
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), AppliedDamage, ShotDir, Hit,
			InstigatorController, Owner, UDamageType::StaticClass());
        if (WeaponType == EGCWeaponType::Taser)
        {
            if (AGCPedestrian* Pedestrian = Cast<AGCPedestrian>(Hit.GetActor()); Pedestrian && !Pedestrian->bDead)
            {
                Pedestrian->KnockDown(4.0f);
            }
        }

		if (ImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ImpactFX, Hit.ImpactPoint,
				Hit.ImpactNormal.Rotation());
		}
	}

	if (MuzzleFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, MuzzleFX, Start, Direction.Rotation());
	}
	if (Owner)
	{
        USoundBase* ShotSound = bSuppressed && SuppressedFireSound ? SuppressedFireSound.Get() : FireSound;
        if (ShotSound)
        {
            UGameplayStatics::PlaySoundAtLocation(World, ShotSound, Owner->GetActorLocation(), bSuppressed ? 0.28f : 1.0f);
        }
	}
	if (bDrawDebugTracer)
	{
		DrawDebugLine(World, Start, OutHitPoint, FColor::Yellow, false, 0.06f, 0, 0.7f);
	}

	return RecoilPitchDegrees;
}

void UGCWeaponComponent::StartReload()
{
	if (bReloading || AmmoInMagazine == MagazineSize || !GetWorld())
	{
		return;
	}
	bReloading = true;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UGCWeaponComponent::FinishReload, ReloadSeconds, false);
}

void UGCWeaponComponent::FinishReload()
{
	bReloading = false;
	AmmoInMagazine = MagazineSize;
}

void UGCWeaponComponent::ConfigureForType(EGCWeaponType NewType)
{
	WeaponType = NewType;
	switch (NewType)
	{
	case EGCWeaponType::Pistol:
		Damage = 22.0f; FireRateRPM = 450.0f; MagazineSize = 17; bAutomatic = false;
		SpreadDegrees = 1.2f; RecoilPitchDegrees = 0.55f; ReloadSeconds = 1.4f;
		break;
	case EGCWeaponType::Rifle:
		Damage = 30.0f; FireRateRPM = 700.0f; MagazineSize = 30; bAutomatic = true;
		SpreadDegrees = 1.6f; RecoilPitchDegrees = 0.75f; ReloadSeconds = 2.0f;
		break;
	case EGCWeaponType::SMG:
		Damage = 16.0f; FireRateRPM = 950.0f; MagazineSize = 32; bAutomatic = true;
		SpreadDegrees = 2.4f; RecoilPitchDegrees = 0.4f; ReloadSeconds = 1.8f;
		break;
	case EGCWeaponType::Sniper:
		Damage = 120.0f; FireRateRPM = 45.0f; MagazineSize = 5; bAutomatic = false;
		SpreadDegrees = 0.15f; RecoilPitchDegrees = 2.4f; ReloadSeconds = 3.0f;
		break;
    case EGCWeaponType::Taser:
        Damage = 4.0f; FireRateRPM = 60.0f; MagazineSize = 2; bAutomatic = false;
        SpreadDegrees = 0.55f; RecoilPitchDegrees = 0.1f; ReloadSeconds = 2.2f;
        break;
	}
	AmmoInMagazine = MagazineSize;
	bReloading = false;
}

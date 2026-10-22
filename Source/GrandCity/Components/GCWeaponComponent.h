#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GCTypes.h"
#include "GCWeaponComponent.generated.h"

class UNiagaraSystem;
class USoundBase;

/** Hitscan weapon: spread, recoil value, tracer/muzzle FX hooks, ammo & reload. */
UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCWeaponComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EGCWeaponType WeaponType = EGCWeaponType::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Range = 15000.0f;

	/** Rounds per minute. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRateRPM = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MagazineSize = 17;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 AmmoInMagazine = 17;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float ReloadSeconds = 1.4f;

	/** Half-angle spread in degrees (hip fire). Aiming divides this by 4. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float SpreadDegrees = 1.2f;

	/** Camera pitch kick (degrees) returned to the shooter per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilPitchDegrees = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FX")
	UNiagaraSystem* MuzzleFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FX")
	UNiagaraSystem* ImpactFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FX")
	USoundBase* FireSound = nullptr;

	/** Debug tracer line while developing (replace with a Niagara beam later). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FX")
	bool bDrawDebugTracer = true;

	/**
	 * Fire one shot. Returns recoil pitch to apply to camera, or 0 if it could not fire.
	 * OutHitPoint receives the end of the trace (for tracers/FX).
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float FireOnce(const FVector& Start, const FVector& Direction, bool bAiming, FVector& OutHitPoint);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartReload();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ConfigureForType(EGCWeaponType NewType);

private:
	double LastFireTime = -100.0;
	bool bReloading = false;
	FTimerHandle ReloadTimer;

	void FinishReload();
};

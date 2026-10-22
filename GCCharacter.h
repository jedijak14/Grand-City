#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GCCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGCWeaponComponent;
class UGCHealthComponent;
class UGCParachuteComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class AGCLocationActor;
class AGCComputerTerminalActor;

/**
 * Player character: third/first person, sprint, jump, swim, aim & shoot,
 * vehicle/plane/boat entry transitions, parachute, ragdoll death + hospital respawn.
 * Hoodie/hair skeletal attachments are provided for Chaos cloth assets.
 */
UCLASS(Blueprintable)
class GRANDCITY_API AGCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGCCharacter();

	/** Applies the Cloth-and-hair user setting to HoodieMesh/HairMesh visibility. */
	UFUNCTION(BlueprintCallable, Category = "Character|Clothing")
	void RefreshClothAndHairVisibility();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Camera")
	USpringArmComponent* CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	UGCWeaponComponent* Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	UGCHealthComponent* Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	UGCParachuteComponent* Parachute;

	/** Canopy mesh shown while parachuting (assign a canopy static mesh in a BP child). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	UStaticMeshComponent* ParachuteCanopy;

	/** Optional Chaos-cloth hoodie, pose-led by the main mesh (wind/cloth physics in the asset). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Clothing")
	USkeletalMeshComponent* HoodieMesh;

	/** Optional hair groom/card component placeholder (leader pose from main mesh). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Clothing")
	USkeletalMeshComponent* HairMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement")
	float WalkSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement")
	float SprintSpeed = 740.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement")
	float AimWalkSpeed = 240.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	bool bSeated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	bool bAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	APawn* NearestEnterable = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	AGCLocationActor* CurrentLocation = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	AGCComputerTerminalActor* CurrentComputer = nullptr;

	/** Voice-line subtitle currently on screen (set by terminals/NPCs/airliner). */
	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	FString CurrentSubtitle;

	UPROPERTY(BlueprintReadOnly, Category = "Character|State")
	float SubtitleExpiryTime = 0.0f;

	// Vehicle transition API (called by vehicle/plane/boat pawns)
	void BeginVehicleTransition();
	void CompleteEnterVehicle(APawn* Vehicle, USceneComponent* Seat);
	void CompleteExitVehicle(const FVector& WorldLocation);
	void SetFirstPersonHidden(bool bHidden);

	void SetCurrentLocation(AGCLocationActor* Location) { CurrentLocation = Location; }
	void SetCurrentComputer(AGCComputerTerminalActor* Computer) { CurrentComputer = Computer; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	void ShowSubtitle(const FString& Text, float DurationSeconds = 4.0f);

	/** Adds wanted stars through the player state (witnessed crime calls this). */
	void ReportCrime(int32 Stars);

	UFUNCTION(BlueprintCallable, Category = "Character")
	bool IsSwimmingNow() const { return bSwimming; }

protected:
	UFUNCTION()
	void OnDeath(AActor* Killer);

private:
	float ClothCheckTimer = 0.0f;
	bool bSprinting = false;
	bool bFiring = false;
	bool bSwimming = false;
	bool bFirstPerson = false;
	float LastFallSpeed = 0.0f;
	float ScanTimer = 0.0f;
	FVector2D MoveInput = FVector2D::ZeroVector;
	FTimerHandle RespawnTimer;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartSprint();
	void StopSprint();
	void StartCrouch();
	void StopCrouch();
	void StartAim();
	void StopAim();
	void StartFire();
	void StopFire();
	void FireWeapon();
	void Reload();
	void Interact();
	void CycleComputerOffer();
	void CycleComputerClass();
	void TryEnterVehicle();
	void DeployParachute();
	void ToggleFirstPerson();
	void EquipPistol();
	void EquipRifle();
	void EquipSMG();
	void EquipSniper();

	void ScanForInteractables();
	void UpdateSwimming();
	void StartRagdoll();
	void RespawnAtHospital();
};

#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Components/GCHealthComponent.h"
#include "Components/GCParachuteComponent.h"
#include "Components/GCWeaponComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GCPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Vehicle/GCAirlinerPawn.h"
#include "Vehicle/GCSurfboardPawn.h"
#include "Vehicle/GCBoatPawn.h"
#include "Vehicle/GCPlanePawn.h"
#include "Vehicle/GCVehiclePawn.h"
#include "World/GCComputerTerminalActor.h"
#include "World/GCRealEstateActor.h"
#include "World/GCDeviceStoreActor.h"
#include "World/GCBusStopActor.h"
#include "Vehicle/GCBusPawn.h"
#include "World/GCLocationActor.h"
#include "World/GCOceanManager.h"

AGCCharacter::AGCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(RootComponent);
	CameraArm->TargetArmLength = 420.0f;
	CameraArm->bUsePawnControlRotation = true;
	CameraArm->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraArm);
	FollowCamera->FieldOfView = 70.0f;

	BodycamCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BodycamCamera"));
	BodycamCamera->SetupAttachment(RootComponent);
	BodycamCamera->SetRelativeLocation(FVector(18.0f, 0.0f, 78.0f));
	BodycamCamera->FieldOfView = 96.0f;
	BodycamCamera->bUsePawnControlRotation = true;
	BodycamCamera->SetAutoActivate(false);

	Weapon = CreateDefaultSubobject<UGCWeaponComponent>(TEXT("Weapon"));
	Health = CreateDefaultSubobject<UGCHealthComponent>(TEXT("Health"));
	BreathingAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HeavyBreathingAudio"));
	BreathingAudio->SetupAttachment(RootComponent);
	BreathingAudio->bAutoActivate = false;
	Parachute = CreateDefaultSubobject<UGCParachuteComponent>(TEXT("Parachute"));

	ParachuteCanopy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParachuteCanopy"));
	ParachuteCanopy->SetupAttachment(RootComponent);
	ParachuteCanopy->SetRelativeLocation(FVector(0.0f, 0.0f, 320.0f));
	ParachuteCanopy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ParachuteCanopy->SetVisibility(false);

	HoodieMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HoodieMesh"));
	HoodieMesh->SetupAttachment(GetMesh());
	HairMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMesh"));
	HairMesh->SetupAttachment(GetMesh());

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = 220.0f;
	Move->JumpZVelocity = 620.0f;
	Move->AirControl = 0.45f;
	Move->MaxSwimSpeed = 260.0f;
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	Move->NavAgentProps.AgentRadius = 38.0f;
	Move->NavAgentProps.bCanCrouch = true;
}

void AGCCharacter::BeginPlay()
{
	Super::BeginPlay();
	float NearestOceanDistanceSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGCOceanManager> It(GetWorld()); It; ++It)
	{
		const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), It->GetActorLocation());
		if (DistanceSq < NearestOceanDistanceSq)
		{
			NearestOceanDistanceSq = DistanceSq;
			OceanManager = *It;
		}
	}
	if (Health)
	{
		Health->OnDeath.AddDynamic(this, &AGCCharacter::OnDeath);
	}
	// Pose-lead the clothing attachments from the main body mesh.
	HoodieMesh->SetLeaderPoseComponent(GetMesh());
	HairMesh->SetLeaderPoseComponent(GetMesh());
	RefreshClothAndHairVisibility();
}

void AGCCharacter::RefreshClothAndHairVisibility()
{
	const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings();
	const bool bShow = !Settings || Settings->bClothAndHairPhysics;
	if (HoodieMesh)
	{
		HoodieMesh->SetVisibility(bShow, false);
	}
	if (HairMesh)
	{
		HairMesh->SetVisibility(bShow, false);
	}
}

void AGCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGCCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGCCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AGCCharacter::StartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AGCCharacter::StopSprint);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AGCCharacter::StartCrouch);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &AGCCharacter::StopCrouch);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AGCCharacter::StartAim);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AGCCharacter::StopAim);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AGCCharacter::StartFire);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AGCCharacter::StopFire);
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AGCCharacter::Reload);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AGCCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("EnterVehicle"), IE_Pressed, this, &AGCCharacter::TryEnterVehicle);
	PlayerInputComponent->BindAction(TEXT("DeployParachute"), IE_Pressed, this, &AGCCharacter::DeployParachute);
	PlayerInputComponent->BindAction(TEXT("ToggleCamera"), IE_Pressed, this, &AGCCharacter::ToggleFirstPerson);
	PlayerInputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AGCCharacter::ToggleBodycam);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AGCCharacter::CyclePaymentMethod);
	PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &AGCCharacter::RepayCreditCard);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AGCCharacter::TryPickpocket);

	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AGCCharacter::CycleComputerOffer);
	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AGCCharacter::CycleComputerClass);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AGCCharacter::EquipPistol);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AGCCharacter::EquipRifle);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AGCCharacter::EquipSMG);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AGCCharacter::EquipSniper);
}

void AGCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bBodycamActive)
	{
		BodycamElapsedSeconds += DeltaSeconds;
	}
	if (bSeated || Health->IsDead())
	{
		return;
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = bAiming ? AimWalkSpeed : (bSprinting ? SprintSpeed : WalkSpeed);
	if (bSprinting && Move->Velocity.Size2D() > 220.0f)
	{
		BreathingRecoverySeconds = 3.5f;
	}
	else
	{
		BreathingRecoverySeconds = FMath::Max(0.0f, BreathingRecoverySeconds - DeltaSeconds);
	}
	const bool bNeedsHeavyBreathing = bSprinting || BreathingRecoverySeconds > 0.0f || (Health && Health->Health <= 25.0f);
	if (BreathingAudio && HeavyBreathingSound)
	{
		if (bNeedsHeavyBreathing && !BreathingAudio->IsPlaying())
		{
			BreathingAudio->SetSound(HeavyBreathingSound);
			BreathingAudio->Play();
		}
		else if (!bNeedsHeavyBreathing && BreathingAudio->IsPlaying())
		{
			BreathingAudio->Stop();
		}
	}

	if (Move->IsFalling())
	{
		LastFallSpeed = -Move->Velocity.Z;
		if (Parachute->bDeployed)
		{
			Parachute->SetSteerInput(FVector2D(MoveInput.X, MoveInput.Y));
		}
		// Space also deploys while in freefall (handled in DeployParachute via key X; see docs).
	}

	// Parachute canopy visual + cloth-like sway.
	const bool bChute = Parachute->bDeployed;
	ParachuteCanopy->SetVisibility(bChute);
	if (bChute)
	{
		const float Sway = FMath::Sin(Parachute->SwayPhase * 1.7f) * 6.0f;
		const float Breathe = 1.0f + FMath::Sin(Parachute->SwayPhase * 2.3f) * 0.04f;
		ParachuteCanopy->SetRelativeRotation(FRotator(Sway, 0.0f, Sway * 0.7f));
		ParachuteCanopy->SetRelativeScale3D(FVector(Breathe, Breathe, 1.0f));
	}

	if (bFiring && Weapon->bAutomatic)
	{
		FireWeapon();
	}

	// Aim camera behaviour.
	bUseControllerRotationYaw = bAiming;
	Move->bOrientRotationToMovement = !bAiming;
	const float TargetArm = bFirstPerson ? 0.0f : (bAiming ? 170.0f : 420.0f);
	CameraArm->TargetArmLength = FMath::FInterpTo(CameraArm->TargetArmLength, TargetArm, DeltaSeconds, 8.0f);
	FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, bAiming ? 48.0f : 70.0f, DeltaSeconds, 8.0f);

	UpdateSwimming();

	ScanTimer += DeltaSeconds;
	if (ScanTimer > 0.15f)
	{
		ScanTimer = 0.0f;
		ScanForInteractables();
	}
}

// ---------------------------------------------------------------- movement input
void AGCCharacter::MoveForward(float Value)
{
	MoveInput.Y = Value;
	if (Controller && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(YawRot.Vector(), Value);
	}
}

void AGCCharacter::MoveRight(float Value)
{
	MoveInput.X = Value;
	if (Controller && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Value);
	}
}

void AGCCharacter::StartSprint() { bSprinting = true; }
void AGCCharacter::StopSprint() { bSprinting = false; }
void AGCCharacter::StartCrouch() { Crouch(); }
void AGCCharacter::StopCrouch() { UnCrouch(); }
void AGCCharacter::StartAim() { bAiming = true; }
void AGCCharacter::StopAim() { bAiming = false; }

void AGCCharacter::StartFire()
{
	bFiring = true;
	FireWeapon();
}

void AGCCharacter::StopFire() { bFiring = false; }

void AGCCharacter::FireWeapon()
{
	if (!FollowCamera || Health->IsDead())
	{
		return;
	}
	FVector HitPoint;
	const float Recoil = Weapon->FireOnce(
		FollowCamera->GetComponentLocation(), FollowCamera->GetForwardVector(), bAiming, HitPoint);
	if (Recoil > 0.0f && Controller)
	{
		FRotator Control = Controller->GetControlRotation();
		Control.Pitch = FMath::ClampAngle(Control.Pitch + Recoil, -89.0f, 89.0f);
		Controller->SetControlRotation(Control);
		// Gunfire in earshot of the world attracts attention. Witness checks refine this later.
		ReportCrime(0); // marks "noise" only - stars are added by damage/witness systems
	}
}

void AGCCharacter::Reload() { Weapon->StartReload(); }
void AGCCharacter::EquipPistol() { Weapon->ConfigureForType(EGCWeaponType::Pistol); }
void AGCCharacter::EquipRifle() { Weapon->ConfigureForType(EGCWeaponType::Rifle); }
void AGCCharacter::EquipSMG() { Weapon->ConfigureForType(EGCWeaponType::SMG); }
void AGCCharacter::EquipSniper() { Weapon->ConfigureForType(EGCWeaponType::Sniper); }

void AGCCharacter::DeployParachute()
{
	Parachute->TryDeploy();
}

void AGCCharacter::ToggleFirstPerson()
{
	bFirstPerson = !bFirstPerson;
	if (bFirstPerson)
	{
		CameraArm->SetRelativeLocation(FVector(18.0f, 0.0f, 72.0f));
	}
	else
	{
		CameraArm->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	}
	GetMesh()->SetOwnerNoSee(bFirstPerson || bBodycamActive);
}

void AGCCharacter::ToggleBodycam()
{
	AGCPlayerState* PlayerState = GetPlayerState<AGCPlayerState>();
	if (!PlayerState || PlayerState->CurrentJob != EGCJob::Police)
	{
		ShowSubtitle(TEXT("Bodycam access is restricted to on-duty police."), 4.0f);
		return;
	}

	bBodycamActive = !bBodycamActive;
	if (bBodycamActive)
	{
		BodycamElapsedSeconds = 0.0f;
		FollowCamera->SetActive(false);
		BodycamCamera->SetActive(true);
		GetMesh()->SetOwnerNoSee(true);
		ShowSubtitle(TEXT("BODYCAM RECORDING — F6 to return to standard view."), 3.0f);
	}
	else
	{
		BodycamCamera->SetActive(false);
		FollowCamera->SetActive(true);
		GetMesh()->SetOwnerNoSee(bFirstPerson);
		ShowSubtitle(TEXT("Bodycam view ended."), 2.0f);
	}
}

void AGCCharacter::CyclePaymentMethod()
{
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
		PS->CyclePaymentMethod();
		ShowSubtitle(FString::Printf(TEXT("Payment set to %s. Press P to cycle."), *PS->GetPaymentMethodLabel()), 3.0f);
	}
}

void AGCCharacter::RepayCreditCard()
{
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
		const int32 Payment = FMath::Min(500, PS->CreditUsed);
		if (PS->RepayCreditCard(Payment))
		{
			ShowSubtitle(FString::Printf(TEXT("Bayline Credit payment: $%d from digital balance."), Payment), 3.0f);
		}
		else
		{
			ShowSubtitle(TEXT("No credit balance is due, or your digital balance is too low."), 3.0f);
		}
	}
}

void AGCCharacter::TryPickpocket()
{
	if (bSeated || Health->IsDead())
	{
		return;
	}
	TArray<AActor*> Pedestrians;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGCPedestrian::StaticClass(), Pedestrians);
	AGCPedestrian* Target = nullptr;
	float BestDistance = 200.0f;
	for (AActor* Actor : Pedestrians)
	{
		AGCPedestrian* Pedestrian = Cast<AGCPedestrian>(Actor);
		if (!Pedestrian || Pedestrian->bDead || Pedestrian->bIsPolice || Pedestrian->bHasBeenPickpocketed)
		{
			continue;
		}
		const float Distance = FVector::DistSquared2D(GetActorLocation(), Pedestrian->GetActorLocation());
		if (Distance < FMath::Square(BestDistance))
		{
			BestDistance = FMath::Sqrt(Distance);
			Target = Pedestrian;
		}
	}
	if (!Target)
	{
		ShowSubtitle(TEXT("No one is close enough to pickpocket. Crouch and approach from behind."), 4.0f);
		return;
	}
	Target->AttemptPickpocket(this);
}

void AGCCharacter::Interact()
{
	if (CurrentBus)
	{
		CurrentBus->DisembarkPassenger();
		return;
	}
	// Airport computers take priority over the generic location action.
	if (CurrentComputer)
	{
		CurrentComputer->UseComputer(this);
		return;
	}
	if (CurrentDeviceStore)
	{
		CurrentDeviceStore->PurchaseSelected(this);
		return;
	}
	if (CurrentBusStop)
	{
		CurrentBusStop->Ride(this);
		return;
	}
	if (CurrentProperty)
	{
		CurrentProperty->Interact(this);
		return;
	}
	if (CurrentLocation)
	{
		CurrentLocation->Interact(this);
	}
}

void AGCCharacter::CycleComputerOffer()
{
	if (CurrentComputer)
	{
		CurrentComputer->CycleOffer();
	}
	else if (CurrentDeviceStore)
	{
		CurrentDeviceStore->CycleOffer();
	}
	else if (CurrentBusStop)
	{
		CurrentBusStop->CycleDestination();
	}
}

void AGCCharacter::CycleComputerClass()
{
	if (CurrentComputer)
	{
		CurrentComputer->CycleClass();
	}
}

void AGCCharacter::ShowSubtitle(const FString& Text, float DurationSeconds)
{
	CurrentSubtitle = Text;
	SubtitleExpiryTime = GetWorld() ? GetWorld()->GetTimeSeconds() + DurationSeconds : DurationSeconds;
}

void AGCCharacter::TryEnterVehicle()
{
	if (!NearestEnterable)
	{
		return;
	}
	if (AGCVehiclePawn* Vehicle = Cast<AGCVehiclePawn>(NearestEnterable))
	{
		Vehicle->BeginEnter(this);
	}
	else if (AGCPlanePawn* Plane = Cast<AGCPlanePawn>(NearestEnterable))
	{
		Plane->BeginEnter(this);
	}
	else if (AGCBoatPawn* Boat = Cast<AGCBoatPawn>(NearestEnterable))
	{
		Boat->BeginEnter(this);
	}
	else if (AGCAirlinerPawn* Airliner = Cast<AGCAirlinerPawn>(NearestEnterable))
	{
		Airliner->BoardPassenger(this);
	}
	else if (AGCSurfboardPawn* Surf = Cast<AGCSurfboardPawn>(NearestEnterable))
	{
		Surf->BeginRide(this);
	}
}

void AGCCharacter::ScanForInteractables()
{
	NearestEnterable = nullptr;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	TArray<AActor*> Ignored = { this };
	TArray<FHitResult> Hits;
	UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), GetActorLocation(), GetActorLocation(),
		380.0f, ObjectTypes, false, Ignored, EDrawDebugTrace::None, Hits, true);

	float BestDist = TNumericLimits<float>::Max();
	for (const FHitResult& Hit : Hits)
	{
		APawn* Pawn = Cast<APawn>(Hit.GetActor());
		if (!Pawn || Pawn == this)
		{
			continue;
		}
		if (Pawn->IsA<AGCVehiclePawn>() || Pawn->IsA<AGCPlanePawn>() || Pawn->IsA<AGCBoatPawn>() || Pawn->IsA<AGCAirlinerPawn>() || Pawn->IsA<AGCSurfboardPawn>())
		{
			const float D = FVector::Distance(GetActorLocation(), Pawn->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				NearestEnterable = Pawn;
			}
		}
	}
}

// ---------------------------------------------------------------- swimming
void AGCCharacter::UpdateSwimming()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FGCOceanSample WaterSample = OceanManager
		? OceanManager->SampleOcean(GetActorLocation())
		: AGCOceanManager::SampleOceanStatic(FVector2D(GetActorLocation().X, GetActorLocation().Y),
			GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f, 1.0f);
	const float WaveZ = WaterSample.Height;

	const bool bBelowSurface = GetActorLocation().Z < WaveZ + 30.0f;
	if (bBelowSurface && Move->MovementMode != MOVE_Swimming)
	{
		// Only treat as water if there is no walkable ground close under us.
		FHitResult GroundHit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SwimGroundCheck), false, this);
		const bool bGroundClose = GetWorld()->LineTraceSingleByChannel(GroundHit,
			GetActorLocation(), GetActorLocation() - FVector(0, 0, 160.0f), ECC_Visibility, Params)
			&& !GroundHit.GetActor()->ActorHasTag(TEXT("Water"));
		if (!bGroundClose)
		{
			Move->SetMovementMode(MOVE_Swimming);
			bSwimming = true;
		}
	}
	else if (!bBelowSurface && Move->MovementMode == MOVE_Swimming)
	{
		Move->SetMovementMode(MOVE_Walking);
		bSwimming = false;
	}

	if (bSwimming)
	{
		// Bob with the same wave field the boats use.
		FVector Vel = Move->Velocity;
		Vel.Z = FMath::Clamp((WaveZ + 40.0f - GetActorLocation().Z) * 2.0f
			+ WaterSample.VerticalVelocity, -120.0f, 120.0f);
		Move->Velocity.Z = Vel.Z;
		LastFallSpeed = 0.0f; // water breaks your fall
	}
}

// ---------------------------------------------------------------- damage/death
void AGCCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (!Parachute->bDeployed && LastFallSpeed > 950.0f && !bSwimming)
	{
		const float Damage = (LastFallSpeed - 950.0f) * 0.09f;
		TakeDamage(Damage, FDamageEvent(), nullptr, this);
	}
	LastFallSpeed = 0.0f;
}

void AGCCharacter::OnDeath(AActor* Killer)
{
	if (BreathingAudio)
	{
		BreathingAudio->Stop();
	}
	if (bBodycamActive)
	{
		bBodycamActive = false;
		BodycamCamera->SetActive(false);
		FollowCamera->SetActive(true);
		GetMesh()->SetOwnerNoSee(bFirstPerson);
	}
	StartRagdoll();
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AGCCharacter::RespawnAtHospital, 3.0f, false);
}

void AGCCharacter::StartRagdoll()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	Parachute->CutAway();
}

void AGCCharacter::RespawnAtHospital()
{
	FVector RespawnLoc = GetActorLocation();
	FRotator RespawnRot = FRotator::ZeroRotator;
	bool bFound = false;

	for (TActorIterator<AGCLocationActor> It(GetWorld()); It; ++It)
	{
		if (It->LocationType == EGCLocationType::Hospital)
		{
			RespawnLoc = It->GetActorLocation() + It->GetActorForwardVector() * 350.0f + FVector(0, 0, 120.0f);
			RespawnRot = It->GetActorRotation();
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		{
			RespawnLoc = Start->GetActorLocation();
			RespawnRot = Start->GetActorRotation();
		}
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetAllBodiesSimulatePhysics(false);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));

	SetActorLocationAndRotation(RespawnLoc, RespawnRot, false, nullptr, ETeleportType::TeleportPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	Health->Revive();
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
		PS->SpendMoney(250); // hospital bill
		PS->ClearWanted();
	}
}

void AGCCharacter::ReportCrime(int32 Stars)
{
	if (Stars <= 0)
	{
		return;
	}
	if (AGCPlayerState* PS = GetPlayerState<AGCPlayerState>())
	{
		PS->AddWantedStars(Stars);
	}
}

// ---------------------------------------------------------------- vehicle transitions
void AGCCharacter::BeginVehicleTransition()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parachute->CutAway();
	bFiring = false;
	bAiming = false;
}

void AGCCharacter::CompleteEnterVehicle(APawn* Vehicle, USceneComponent* Seat)
{
	if (Seat)
	{
		AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	bSeated = true;
}

void AGCCharacter::CompleteExitVehicle(const FVector& WorldLocation)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(WorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bSeated = false;
}

void AGCCharacter::SetFirstPersonHidden(bool bHidden)
{
	GetMesh()->SetVisibility(!bHidden, true);
}

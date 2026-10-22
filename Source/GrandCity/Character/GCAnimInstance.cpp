#include "Character/GCAnimInstance.h"
#include "Character/GCCharacter.h"
#include "Character/GCPedestrian.h"
#include "Components/GCHealthComponent.h"
#include "Components/GCParachuteComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Vehicle/GCVehiclePawn.h"

void UGCAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		return;
	}

	const FVector Vel = Pawn->GetVelocity();
	GroundSpeed = FVector(Vel.X, Vel.Y, 0.0f).Size();
	Direction = UKismetMathLibrary::CalculateDirection(Vel, Pawn->GetActorRotation());

	if (DeltaSeconds > 0.0f)
	{
		TurnYawRate = FMath::Clamp(
			FMath::FindDeltaAngleDegrees(LastDirection, Direction) / DeltaSeconds, -540.0f, 540.0f);
	}
	LastDirection = Direction;

	const FVector LocalVel = Pawn->GetActorTransform().InverseTransformVectorNoScale(Vel);
	LocalVelocity2D = FVector2D(LocalVel.X, LocalVel.Y);

	bFalling = false;
	bSwimming = false;
	bCrouching = false;
	AccelerationSize = 0.0f;
	BodyLean = 0.0f;
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		bCrouching = Character->bIsCrouched;
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			bFalling = Move->IsFalling();
			bSwimming = Move->IsSwimming();
			const FVector Accel = Move->GetCurrentAcceleration();
			AccelerationSize = Accel.Size();
			const FVector LocalAccel = Pawn->GetActorTransform().InverseTransformVectorNoScale(Accel);
			BodyLean = FMath::Clamp(-LocalAccel.Y / 1400.0f, -1.0f, 1.0f);
		}
	}

	// Aim offset between control facing and body facing.
	AimPitch = 0.0f;
	AimYaw = 0.0f;
	if (Pawn->GetController())
	{
		const FRotator Delta = (Pawn->GetControlRotation() - Pawn->GetActorRotation()).GetNormalized();
		AimYaw = FMath::Clamp(Delta.Yaw, -90.0f, 90.0f);
		AimPitch = FMath::Clamp(Delta.Pitch, -80.0f, 80.0f);
	}

	bDead = false;
	if (UGCHealthComponent* Health = Pawn->FindComponentByClass<UGCHealthComponent>())
	{
		bDead = Health->IsDead();
	}
	if (AGCPedestrian* Ped = Cast<AGCPedestrian>(Pawn))
	{
		bDead = bDead || Ped->bDead;
	}

	bAiming = false;
	bSeated = false;
	bParachuting = false;
	VehicleSteering = 0.0f;
	if (AGCCharacter* GCChar = Cast<AGCCharacter>(Pawn))
	{
		bAiming = GCChar->bAiming;
		bSeated = GCChar->bSeated;
		bParachuting = GCChar->Parachute && GCChar->Parachute->bDeployed;

		if (AActor* ParentVehicle = GCChar->GetAttachParentActor())
		{
			if (AGCVehiclePawn* Vehicle = Cast<AGCVehiclePawn>(ParentVehicle))
			{
				VehicleSteering = Vehicle->GetSteeringNormalized();
			}
		}
	}
}

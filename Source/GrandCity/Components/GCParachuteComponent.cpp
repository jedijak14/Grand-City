#include "Components/GCParachuteComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

UGCParachuteComponent::UGCParachuteComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UGCParachuteComponent::TryDeploy()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || bDeployed)
	{
		return false;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!Move || !Move->IsFalling() || Move->Velocity.Z > -MinDeployFallSpeed)
	{
		return false;
	}
	bDeployed = true;
	OnDeployed.Broadcast();
	return true;
}

void UGCParachuteComponent::CutAway()
{
	bDeployed = false;
}

void UGCParachuteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bDeployed)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		bDeployed = false;
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	if (!Move->IsFalling())
	{
		bDeployed = false;
		OnLanded.Broadcast();
		return;
	}

	SwayPhase += DeltaTime;

	FVector Vel = Move->Velocity;

	// Flare: pulling back (negative forward input) near touchdown bleeds descent briefly.
	const float Flare = SteerInput.Y < -0.4f ? 0.55f : 1.0f;
	Vel.Z = FMath::Max(Vel.Z, -DescentSpeed * Flare);

	// Glide toward where the player is looking, steered by input.
	FRotator ControlRot = Character->GetActorRotation();
	if (Character->GetController())
	{
		ControlRot = Character->GetController()->GetControlRotation();
	}
	const FVector Forward = FRotator(0.0f, ControlRot.Yaw, 0.0f).Vector();
	const FVector Right = FRotator(0.0f, ControlRot.Yaw + 90.0f, 0.0f).Vector();
	FVector Desired = (Forward * (0.6f + SteerInput.Y * 0.4f) + Right * SteerInput.X * 0.7f) * ForwardGlideSpeed;
	Desired.Z = Vel.Z;

	Vel.X = FMath::FInterpTo(Vel.X, Desired.X, DeltaTime, 1.6f);
	Vel.Y = FMath::FInterpTo(Vel.Y, Desired.Y, DeltaTime, 1.6f);
	Move->Velocity = Vel;
}

#include "AI/GCVehicleAIController.h"
#include "Core/GCPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Vehicle/GCVehiclePawn.h"
#include "Vehicle/GCBusPawn.h"

AGCVehicleAIController::AGCVehicleAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGCVehicleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Vehicle = Cast<AGCVehiclePawn>(InPawn);
	if (Vehicle && Vehicle->bIsPoliceVehicle)
	{
		Mode = EGCVehicleAIMode::PolicePursuit;
	}
	if (Mode == EGCVehicleAIMode::BusRoute && BusRoutePoints.IsValidIndex(BusRouteIndex))
	{
		CurrentTarget = BusRoutePoints[BusRouteIndex];
		bHasTarget = true;
	}
	else
	{
		PickTrafficTarget();
	}
}

void AGCVehicleAIController::SetMode(EGCVehicleAIMode NewMode)
{
	Mode = NewMode;
	if (Vehicle)
	{
		Vehicle->SetSiren(NewMode == EGCVehicleAIMode::PolicePursuit);
		if (NewMode == EGCVehicleAIMode::Parked)
		{
			Vehicle->SetAIInputs(0.0f, 0.0f, true);
		}
	}
}

void AGCVehicleAIController::SetBusRoute(const TArray<FVector>& RoutePoints, float StopDwellSeconds)
{
	BusRoutePoints = RoutePoints;
	BusRouteIndex = 0;
	BusStopDwellSeconds = FMath::Clamp(StopDwellSeconds, 1.0f, 30.0f);
	BusDwellRemaining = 0.0f;
	Mode = EGCVehicleAIMode::BusRoute;
	if (Vehicle && BusRoutePoints.IsValidIndex(BusRouteIndex))
	{
		CurrentTarget = BusRoutePoints[BusRouteIndex];
		bHasTarget = true;
	}
}

void AGCVehicleAIController::PickTrafficTarget()
{
	if (!Vehicle)
	{
		return;
	}
	// Prefer a point generally ahead of the car so traffic flows instead of u-turning constantly.
	const FVector Origin = Vehicle->GetActorLocation() + Vehicle->GetActorForwardVector() * 4000.0f;
	FVector Dest;
	if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), Origin, Dest, 3500.0f))
	{
		CurrentTarget = Dest;
		bHasTarget = true;
	}
	else
	{
		CurrentTarget = Origin;
		bHasTarget = true;
	}
	TargetTimer = 20.0f;
}

void AGCVehicleAIController::DriveToward(const FVector& TargetLocation, float DesiredKmh, float DeltaSeconds)
{
	if (!Vehicle)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector ToTarget = TargetLocation - Vehicle->GetActorLocation();
	const FVector LocalDir = Vehicle->GetActorTransform().InverseTransformVectorNoScale(ToTarget).GetSafeNormal();
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(LocalDir.Y, LocalDir.X));
	float Steer = FMath::Clamp(AngleDeg / 28.0f, -1.0f, 1.0f);

	const float SpeedKmh = Vehicle->GetSpeedKmh();
	const float ForwardSpeedKmh = Vehicle->GetForwardSpeedCms() * 0.036f;

	// Front obstacle probe.
	bool bObstacle = false;
	{
		const FVector Start = Vehicle->GetActorLocation() + FVector(0, 0, 80.0f);
		const FVector End = Start + Vehicle->GetActorForwardVector() * 620.0f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleObstacle), false, Vehicle);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			// The ground plane in front is not an obstacle; something tall in the probe is.
			if (Hit.ImpactNormal.Z < 0.6f)
			{
				bObstacle = true;
			}
		}
	}

	float Throttle = 0.0f;
	bool bHandbrake = false;

	if (Now < ReverseUntilTime)
	{
		Throttle = -0.7f;
		Steer = -Steer;
	}
	else if (bObstacle && ForwardSpeedKmh > 2.0f)
	{
		Throttle = -0.65f; // vehicle logic treats pedals-back at speed as braking
	}
	else
	{
		const float SpeedError = DesiredKmh - ForwardSpeedKmh;
		Throttle = FMath::Clamp(SpeedError / 25.0f, -0.8f, 1.0f);
		if (FMath::Abs(AngleDeg) > 55.0f)
		{
			Throttle *= 0.35f; // slow for sharp turns
		}
	}

	// Stuck detection -> reverse out.
	if (SpeedKmh < 3.0f && FMath::Abs(Throttle) > 0.4f && Now >= ReverseUntilTime)
	{
		StuckTimer += DeltaSeconds;
		if (StuckTimer > 1.8f)
		{
			ReverseUntilTime = Now + 1.1f;
			StuckTimer = 0.0f;
		}
	}
	else
	{
		StuckTimer = 0.0f;
	}

	Vehicle->SetAIInputs(Throttle, Steer, bHandbrake);
}

void AGCVehicleAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Vehicle || Vehicle->bWrecked)
	{
		return;
	}

	switch (Mode)
	{
	case EGCVehicleAIMode::Parked:
		Vehicle->SetAIInputs(0.0f, 0.0f, true);
		return;

	case EGCVehicleAIMode::PolicePursuit:
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		AGCPlayerState* PS = PC ? PC->GetPlayerState<AGCPlayerState>() : nullptr;
		const bool bWanted = PS && PS->WantedStars > 0;
		Vehicle->SetSiren(bWanted);
		if (!PlayerPawn || !bWanted)
		{
			// Drop back to patrolling like traffic.
			if (!bHasTarget) { PickTrafficTarget(); }
			DriveToward(CurrentTarget, TrafficCruiseKmh, DeltaSeconds);
			return;
		}
		// Intercept: aim where the player will be, scaled by distance. Ram at high stars.
		const float Dist = FVector::Distance(Vehicle->GetActorLocation(), PlayerPawn->GetActorLocation());
		const FVector Predicted = PlayerPawn->GetActorLocation()
			+ PlayerPawn->GetVelocity() * FMath::Clamp(Dist / 2500.0f, 0.15f, 0.85f);
		const float Aggression = PS->WantedStars >= 4 ? 190.0f : 130.0f;
		DriveToward(Predicted, Aggression, DeltaSeconds);
		return;
	}

	case EGCVehicleAIMode::BusRoute:
	{
		if (BusRoutePoints.IsEmpty())
		{
			Vehicle->SetAIInputs(0.0f, 0.0f, true);
			return;
		}
		if (BusDwellRemaining > 0.0f)
		{
			BusDwellRemaining = FMath::Max(0.0f, BusDwellRemaining - DeltaSeconds);
			Vehicle->SetAIInputs(0.0f, 0.0f, true);
			return;
		}
		if (!BusRoutePoints.IsValidIndex(BusRouteIndex))
		{
			BusRouteIndex = 0;
		}
		if (FVector::Dist2D(Vehicle->GetActorLocation(), BusRoutePoints[BusRouteIndex]) <= WaypointAcceptance)
		{
			if (AGCBusPawn* Bus = Cast<AGCBusPawn>(Vehicle))
			{
				Bus->NotifyRouteStopReached(BusRouteIndex);
			}
			BusDwellRemaining = BusStopDwellSeconds;
			BusRouteIndex = (BusRouteIndex + 1) % BusRoutePoints.Num();
			CurrentTarget = BusRoutePoints[BusRouteIndex];
			bHasTarget = true;
			Vehicle->SetAIInputs(0.0f, 0.0f, true);
			return;
		}
		CurrentTarget = BusRoutePoints[BusRouteIndex];
		bHasTarget = true;
		DriveToward(CurrentTarget, TrafficCruiseKmh, DeltaSeconds);
		return;
	}

	default: // Traffic
		TargetTimer -= DeltaSeconds;
		if (!bHasTarget || TargetTimer <= 0.0f
			|| FVector::Dist2D(Vehicle->GetActorLocation(), CurrentTarget) < WaypointAcceptance)
		{
			PickTrafficTarget();
		}
		DriveToward(CurrentTarget, TrafficCruiseKmh, DeltaSeconds);
		return;
	}
}

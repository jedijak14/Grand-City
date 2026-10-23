#include "Vehicle/GCHelicopterPawn.h"

AGCHelicopterPawn::AGCHelicopterPawn()
{
	bHelicopterMode = true;
	MassKg = 2450.0f;
	FuelPercent = 92.0f;
	Body->SetBoxExtent(FVector(260.0f, 165.0f, 105.0f));
	FuselageMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	FuselageMesh->SetRelativeScale3D(FVector(2.3f, 0.65f, 0.72f));
	PropellerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	PropellerMesh->SetRelativeScale3D(FVector(4.8f, 0.12f, 0.06f));
	PilotSeat->SetRelativeLocation(FVector(30.0f, -35.0f, 15.0f));
	ExitPoint->SetRelativeLocation(FVector(0.0f, -240.0f, -20.0f));
	ChaseArm->SetRelativeLocation(FVector(-100.0f, 0.0f, 220.0f));
	ChaseArm->TargetArmLength = 900.0f;
}

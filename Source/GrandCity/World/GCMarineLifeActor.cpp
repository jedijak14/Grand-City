#include "World/GCMarineLifeActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GCOceanManager.h"

AGCMarineLifeActor::AGCMarineLifeActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Fish = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Fish"));
	Fish->SetupAttachment(Root);
	Fish->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Fish->CastShadow = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Fish->SetStaticMesh(SphereMesh.Object);
	}

	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* Dolphin = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Dolphin%d"), i));
		Dolphin->SetupAttachment(Root);
		Dolphin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dolphin->SetRelativeScale3D(FVector(2.8f, 0.55f, 0.60f));
		if (SphereMesh.Succeeded())
		{
			Dolphin->SetStaticMesh(SphereMesh.Object);
		}
		Dolphins.Add(Dolphin);
	}
}

void AGCMarineLifeActor::BeginPlay()
{
	Super::BeginPlay();

	FRandomStream Rand(777);
	FishStates.Reset();
	Fish->ClearInstances();
	for (int32 i = 0; i < FishCount; ++i)
	{
		FFishState State;
		State.OrbitRadius = Rand.FRandRange(SchoolRadius * 0.25f, SchoolRadius);
		State.OrbitPhase = Rand.FRandRange(0.0f, 360.0f);
		State.OrbitSpeed = Rand.FRandRange(8.0f, 22.0f) * (Rand.FRand() > 0.5f ? 1.0f : -1.0f);
		State.DepthOffset = Rand.FRandRange(-70.0f, 70.0f);
		State.WigglePhase = Rand.FRandRange(0.0f, 6.28f);
		State.LastPosition = GetActorLocation();
		FishStates.Add(State);
		Fish->AddInstance(FTransform(State.LastPosition), true);
	}

	DolphinPhases.Reset();
	DolphinLastPositions.Reset();
	for (int32 i = 0; i < Dolphins.Num(); ++i)
	{
		DolphinPhases.Add(Rand.FRandRange(0.0f, 12.0f));
		DolphinLastPositions.Add(GetActorLocation());
	}
}

void AGCMarineLifeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld() || DeltaSeconds <= 0.0f)
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();
	const FVector Origin = GetActorLocation();
	const FVector SchoolCenter = Origin + FVector(
		FMath::Cos(Time * 0.07f) * SchoolRadius * 0.35f,
		FMath::Sin(Time * 0.09f) * SchoolRadius * 0.35f,
		0.0f);

	for (int32 i = 0; i < FishStates.Num(); ++i)
	{
		FFishState& State = FishStates[i];
		State.OrbitPhase += State.OrbitSpeed * DeltaSeconds;
		const float Rad = FMath::DegreesToRadians(State.OrbitPhase);
		FVector Pos = SchoolCenter + FVector(FMath::Cos(Rad) * State.OrbitRadius, FMath::Sin(Rad) * State.OrbitRadius, 0.0f);
		const float Surface = WaterBaseLevelZ + AGCOceanManager::GetWaveHeightAt(FVector2D(Pos.X, Pos.Y), Time, 1.0f);
		Pos.Z = Surface - SwimDepthCm + State.DepthOffset + FMath::Sin(Time * 2.0f + State.WigglePhase) * 12.0f;

		FVector Heading = Pos - State.LastPosition;
		Heading.Z *= 0.35f;
		const FRotator Rot = Heading.IsNearlyZero() ? FRotator::ZeroRotator : Heading.GetSafeNormal().Rotation();
		Fish->UpdateInstanceTransform(i, FTransform(Rot, Pos, FishInstanceScale), true, false, true);
		State.LastPosition = Pos;
	}
	Fish->MarkRenderStateDirty();

	for (int32 i = 0; i < Dolphins.Num(); ++i)
	{
		if (!Dolphins[i])
		{
			continue;
		}
		const float Phase = Time * 0.55f + DolphinPhases[i];
		const float OrbitRad = Phase * 0.42f;
		FVector Pos = Origin + FVector(FMath::Cos(OrbitRad) * (SchoolRadius + 900.0f), FMath::Sin(OrbitRad) * (SchoolRadius + 900.0f), 0.0f);
		const float Surface = WaterBaseLevelZ + AGCOceanManager::GetWaveHeightAt(FVector2D(Pos.X, Pos.Y), Time, 1.0f);
		// Periodic leap arc; otherwise cruising just below the surface.
		const float LeapCycle = FMath::Frac(Phase / 7.0f);
		float Z = Surface - 55.0f;
		if (LeapCycle < 0.22f)
		{
			const float LeapT = LeapCycle / 0.22f;
			Z = Surface - 30.0f + FMath::Sin(LeapT * PI) * 260.0f;
		}
		Pos.Z = Z;
		FVector Heading = Pos - DolphinLastPositions[i];
		if (!Heading.IsNearlyZero())
		{
			Dolphins[i]->SetWorldLocationAndRotation(Pos, Heading.GetSafeNormal().Rotation());
		}
		else
		{
			Dolphins[i]->SetWorldLocation(Pos);
		}
		DolphinLastPositions[i] = Pos;
	}
}

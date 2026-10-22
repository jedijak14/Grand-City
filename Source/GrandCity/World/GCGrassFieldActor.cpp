#include "World/GCGrassFieldActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Core/GCGameUserSettings.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGCGrassFieldActor::AGCGrassFieldActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Grass = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Grass"));
	Grass->SetupAttachment(Root);
	Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Grass->CastShadow = false;
	Grass->SetNumCustomDataFloats(1);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Grass->SetStaticMesh(CubeMesh.Object); // fallback blade volume; assign real grass in a BP child
	}
}

void AGCGrassFieldActor::BeginPlay()
{
	Super::BeginPlay();
	Regenerate();
}

void AGCGrassFieldActor::Regenerate()
{
	if (!Grass || !Grass->GetStaticMesh() || !GetWorld())
	{
		return;
	}
	if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
	{
		if (!Settings->bGrassEnabled)
		{
			Grass->ClearInstances();
			return;
		}
	}

	Grass->ClearInstances();
	FRandomStream Rand(RandomSeed);
	const FVector Origin = GetActorLocation();

	for (int32 i = 0; i < InstanceCount; ++i)
	{
		FVector Pos(
			Origin.X + Rand.FRandRange(-FieldSize.X * 0.5f, FieldSize.X * 0.5f),
			Origin.Y + Rand.FRandRange(-FieldSize.Y * 0.5f, FieldSize.Y * 0.5f),
			Origin.Z + 600.0f);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrassGround), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Pos, Pos - FVector(0, 0, 1400.0f), ECC_Visibility, Params))
		{
			if (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Water")))
			{
				continue;
			}
			Pos = Hit.ImpactPoint;
		}
		else
		{
			Pos.Z = Origin.Z;
		}

		const float S = Rand.FRandRange(0.65f, 1.35f);
		const FTransform Instance(
			FRotator(Rand.FRandRange(-5.0f, 5.0f), Rand.FRandRange(0.0f, 360.0f), Rand.FRandRange(-5.0f, 5.0f)),
			Pos,
			BaseInstanceScale * S);
		const int32 Index = Grass->AddInstance(Instance, true);
		Grass->SetCustomDataValue(Index, 0, Rand.FRand(), false);
	}
	Grass->MarkRenderStateDirty();
}

#include "Components/GCSkidmarkComponent.h"

UGCSkidmarkComponent::UGCSkidmarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CastShadow = false;
	// Vertices are stored in world space, so the component itself must not move with the car.
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);
}

void UGCSkidmarkComponent::BeginPlay()
{
	Super::BeginPlay();
	SetWorldTransform(FTransform::Identity);
}

void UGCSkidmarkComponent::AddSkidPoint(int32 WheelId, const FVector& WorldLocation, const FVector& GroundNormal, float Width, float Alpha)
{
	if (Vertices.Num() >= MaxVertices)
	{
		ClearAll();
	}

	const FVector Loc = WorldLocation + GroundNormal * 1.2f; // avoid z-fighting with road
	FLastPoint& Last = LastPoints.FindOrAdd(WheelId);

	FVector Forward;
	if (Last.bValid)
	{
		Forward = (Loc - Last.Center);
		Forward.Z = 0.0f;
		if (Forward.Size() < MinSegmentLength)
		{
			return;
		}
		Forward.Normalize();
	}
	else
	{
		Forward = FVector::ForwardVector;
	}

	FVector Right = FVector::CrossProduct(GroundNormal, Forward).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	const FLinearColor Col(0.0f, 0.0f, 0.0f, FMath::Clamp(Alpha, 0.0f, 1.0f));

	if (Last.bValid)
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(Last.Center - Last.Right * Width * 0.5f);
		Vertices.Add(Last.Center + Last.Right * Width * 0.5f);
		Vertices.Add(Loc - Right * Width * 0.5f);
		Vertices.Add(Loc + Right * Width * 0.5f);
		for (int32 i = 0; i < 4; ++i)
		{
			Normals.Add(GroundNormal);
			UVs.Add(FVector2D::ZeroVector);
			Colors.Add(Col);
		}
		Triangles.Add(Base + 0); Triangles.Add(Base + 2); Triangles.Add(Base + 1);
		Triangles.Add(Base + 1); Triangles.Add(Base + 2); Triangles.Add(Base + 3);
		RebuildSection();
	}

	Last.Center = Loc;
	Last.Right = Right;
	Last.bValid = true;
}

void UGCSkidmarkComponent::EndSkid(int32 WheelId)
{
	if (FLastPoint* Last = LastPoints.Find(WheelId))
	{
		Last->bValid = false;
	}
}

void UGCSkidmarkComponent::ClearAll()
{
	Vertices.Reset();
	Normals.Reset();
	UVs.Reset();
	Colors.Reset();
	Triangles.Reset();
	LastPoints.Reset();
	ClearAllMeshSections();
	bSectionCreated = false;
}

void UGCSkidmarkComponent::RebuildSection()
{
	// The index buffer grows as segments are appended, and UpdateMeshSection cannot
	// resize it - so recreate the section. (Calls are already throttled by MinSegmentLength.)
	if (Vertices.Num() == 0 || Triangles.Num() == 0)
	{
		return;
	}
	if (bSectionCreated)
	{
		ClearMeshSection(0);
	}
	CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
	bSectionCreated = true;
}

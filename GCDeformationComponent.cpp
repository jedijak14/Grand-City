#include "Components/GCDeformationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"

UGCDeformationComponent::UGCDeformationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGCDeformationComponent::InitializeFromStaticMesh(UStaticMeshComponent* Source, UProceduralMeshComponent* TargetProcMesh)
{
	if (!Source || !TargetProcMesh || !Source->GetStaticMesh())
	{
		return false;
	}

	ProcMesh = TargetProcMesh;
	UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(Source, 0, ProcMesh, false);

	const FProcMeshSection* Section = ProcMesh->GetProcMeshSection(0);
	if (!Section || Section->ProcVertexBuffer.Num() == 0)
	{
		// Mesh was probably imported without CPU access. Keep the static mesh visible.
		ProcMesh->SetVisibility(false);
		return false;
	}

	const int32 NumVerts = Section->ProcVertexBuffer.Num();
	OriginalPositions.Reset(NumVerts);
	CurrentPositions.Reset(NumVerts);
	Normals.Reset(NumVerts);
	UVs.Reset(NumVerts);
	Colors.Reset(NumVerts);
	Tangents.Reset(NumVerts);
	for (const FProcMeshVertex& V : Section->ProcVertexBuffer)
	{
		OriginalPositions.Add(V.Position);
		CurrentPositions.Add(V.Position);
		Normals.Add(V.Normal);
		UVs.Add(V.UV0);
		Colors.Add(V.Color);
		Tangents.Add(V.Tangent);
	}
	Indices = Section->ProcIndexBuffer;

	// Physics stays on the vehicle's collision body; this mesh is purely visual.
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Source->SetVisibility(false);
	Source->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bInitialized = true;
	return true;
}

void UGCDeformationComponent::ApplyDentLocal(const FVector& LocalPoint, const FVector& LocalImpactDirection, float Radius, float Depth)
{
	if (!bInitialized || Radius <= 0.0f || Depth <= 0.0f)
	{
		return;
	}

	const FVector Dir = LocalImpactDirection.GetSafeNormal();
	bool bChanged = false;
	for (int32 i = 0; i < CurrentPositions.Num(); ++i)
	{
		const float Dist = FVector::Distance(CurrentPositions[i], LocalPoint);
		if (Dist >= Radius)
		{
			continue;
		}
		// Smooth crater falloff: deepest at the centre, zero at the rim.
		const float Falloff = 0.5f + 0.5f * FMath::Cos((Dist / Radius) * PI);
		FVector NewPos = CurrentPositions[i] + Dir * (Depth * Falloff);
		const FVector TotalOffset = NewPos - OriginalPositions[i];
		if (TotalOffset.Size() > MaxVertexDisplacement)
		{
			NewPos = OriginalPositions[i] + TotalOffset.GetSafeNormal() * MaxVertexDisplacement;
		}
		CurrentPositions[i] = NewPos;
		bChanged = true;
	}

	if (bChanged)
	{
		DamageLevel = FMath::Clamp(DamageLevel + Depth / 220.0f, 0.0f, 1.0f);
		RefreshMeshSection();
	}
}

void UGCDeformationComponent::ApplyDentWorld(const FVector& WorldPoint, const FVector& WorldImpactDirection, float Radius, float Depth)
{
	if (!bInitialized || !ProcMesh)
	{
		return;
	}
	const FVector LocalPoint = ProcMesh->GetComponentTransform().InverseTransformPosition(WorldPoint);
	const FVector LocalDir = ProcMesh->GetComponentTransform().InverseTransformVectorNoScale(WorldImpactDirection);
	const float LocalRadius = Radius / FMath::Max(0.01f, ProcMesh->GetComponentScale().GetMax());
	const float LocalDepth = Depth / FMath::Max(0.01f, ProcMesh->GetComponentScale().GetMax());
	ApplyDentLocal(LocalPoint, LocalDir, LocalRadius, LocalDepth);
}

void UGCDeformationComponent::RepairAll()
{
	if (!bInitialized)
	{
		return;
	}
	CurrentPositions = OriginalPositions;
	DamageLevel = 0.0f;
	RefreshMeshSection();
}

void UGCDeformationComponent::RefreshMeshSection()
{
	if (!ProcMesh)
	{
		return;
	}
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(CurrentPositions, Indices, UVs, Normals, Tangents);
	ProcMesh->UpdateMeshSection_LinearColor(0, CurrentPositions, Normals, UVs, LinearColors, Tangents);
}

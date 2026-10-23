#include "Components/GCDeformationComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
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

	bInitialized = false;
	DamageLevel = 0.0f;
	MeshSections.Reset();
	ProcMesh = TargetProcMesh;
	ProcMesh->ClearAllMeshSections();
	UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(Source, 0, ProcMesh, false);

	const int32 NumSections = ProcMesh->GetNumSections();
	if (NumSections <= 0)
	{
		// The source may have been imported without CPU access. Keep its static mesh visible.
		ProcMesh->SetVisibility(false);
		return false;
	}

	MeshSections.Reset(NumSections);
	for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
	{
		const FProcMeshSection* ProcSection = ProcMesh->GetProcMeshSection(SectionIndex);
		if (!ProcSection || ProcSection->ProcVertexBuffer.Num() == 0 || ProcSection->ProcIndexBuffer.Num() == 0)
		{
			MeshSections.Reset();
			ProcMesh->ClearAllMeshSections();
			ProcMesh->SetVisibility(false);
			return false;
		}

		FGCDeformationMeshSection Section;
		Section.SectionIndex = SectionIndex;
		const int32 NumVertices = ProcSection->ProcVertexBuffer.Num();
		Section.OriginalPositions.Reserve(NumVertices);
		Section.CurrentPositions.Reserve(NumVertices);
		Section.Normals.Reserve(NumVertices);
        Section.StructuralHardness.Reserve(NumVertices);
		Section.UVs.Reserve(NumVertices);
		Section.Colors.Reserve(NumVertices);
		Section.Tangents.Reserve(NumVertices);

		for (const FProcMeshVertex& Vertex : ProcSection->ProcVertexBuffer)
		{
			Section.OriginalPositions.Add(Vertex.Position);
			Section.CurrentPositions.Add(Vertex.Position);
			Section.Normals.Add(Vertex.Normal);
            Section.StructuralHardness.Add(bUseVertexRedAsHardnessMask ? static_cast<float>(Vertex.Color.R) / 255.0f : 0.5f);
			Section.UVs.Add(Vertex.UV0);
			Section.Colors.Add(Vertex.Color);
			Section.Tangents.Add(Vertex.Tangent);
		}

		Section.Indices.Reserve(ProcSection->ProcIndexBuffer.Num());
		for (const uint32 Index : ProcSection->ProcIndexBuffer)
		{
			Section.Indices.Add(static_cast<int32>(Index));
		}
		MeshSections.Add(MoveTemp(Section));
	}

	// The procedural copy carries the material sections; physics remains on the vehicle's
	// collision body. Hide the original only after every procedural section was copied.
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

	const FVector Direction = LocalImpactDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const float RadiusSquared = Radius * Radius;
	const float DisplacementLimit = FMath::Max(1.0f, MaxVertexDisplacement);
	float MaximumAppliedDisplacement = 0.0f;

	for (FGCDeformationMeshSection& Section : MeshSections)
	{
		bool bSectionChanged = false;
		for (int32 VertexIndex = 0; VertexIndex < Section.CurrentPositions.Num(); ++VertexIndex)
		{
			const FVector& CurrentPosition = Section.CurrentPositions[VertexIndex];
			const float DistanceSquared = FVector::DistSquared(CurrentPosition, LocalPoint);
			if (DistanceSquared >= RadiusSquared)
			{
				continue;
			}

			// Only deform the surface facing the impact. Otherwise a local crash can move
			// vertices on the far side of a closed body through the cabin and opposite panel.
			const float Facing = VertexIndex < Section.Normals.Num()
				? -FVector::DotProduct(Section.Normals[VertexIndex].GetSafeNormal(), Direction)
				: 0.0f;
			if (Facing < 0.15f)
			{
				continue;
			}

			const float Distance = FMath::Sqrt(DistanceSquared);
			const float RadialFalloff = 0.5f + 0.5f * FMath::Cos((Distance / Radius) * PI);
            const float Hardness = Section.StructuralHardness.IsValidIndex(VertexIndex) ? Section.StructuralHardness[VertexIndex] : 0.5f;
            const float MaterialCompliance = bUseVertexRedAsHardnessMask ? FMath::Lerp(1.65f, 0.45f, Hardness) : 1.0f;
			const float AppliedDepth = Depth * RadialFalloff * FMath::Clamp(Facing, 0.0f, 1.0f) * MaterialCompliance;
			FVector NewPosition = CurrentPosition + Direction * AppliedDepth;
			const FVector TotalOffset = NewPosition - Section.OriginalPositions[VertexIndex];
			if (TotalOffset.SizeSquared() > FMath::Square(DisplacementLimit))
			{
				NewPosition = Section.OriginalPositions[VertexIndex]
					+ TotalOffset.GetSafeNormal() * DisplacementLimit;
			}

			const float AppliedDisplacement = FVector::Distance(CurrentPosition, NewPosition);
			if (AppliedDisplacement > KINDA_SMALL_NUMBER)
			{
				Section.CurrentPositions[VertexIndex] = NewPosition;
				MaximumAppliedDisplacement = FMath::Max(MaximumAppliedDisplacement, AppliedDisplacement);
				bSectionChanged = true;
			}
		}

		if (bSectionChanged)
		{
			RefreshMeshSection(Section);
		}
	}

	if (MaximumAppliedDisplacement > KINDA_SMALL_NUMBER)
	{
		DamageLevel = FMath::Clamp(DamageLevel + MaximumAppliedDisplacement / 220.0f, 0.0f, 1.0f);
	}
}

void UGCDeformationComponent::ApplyDentWorld(const FVector& WorldPoint, const FVector& WorldImpactDirection, float Radius, float Depth)
{
    if (Radius <= 0.0f || Depth <= 0.0f)
    {
        return;
    }
    if (bInitialized && ProcMesh)
    {
        const FVector LocalPoint = ProcMesh->GetComponentTransform().InverseTransformPosition(WorldPoint);
        const FVector LocalDirection = ProcMesh->GetComponentTransform().InverseTransformVectorNoScale(WorldImpactDirection);
        const float LargestScale = FMath::Max(0.01f, ProcMesh->GetComponentScale().GetAbsMax());
        ApplyDentLocal(LocalPoint, LocalDirection, Radius / LargestScale, Depth / LargestScale);
    }
    ApplyStructuralStress(WorldPoint, WorldImpactDirection, Radius, Depth);
}

void UGCDeformationComponent::ApplyStructuralStress(const FVector& WorldPoint, const FVector& WorldDirection, float Radius, float Depth)
{
    const FVector Direction = WorldDirection.GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return;
    }
    for (FGCDetachableVehiclePart& Part : DetachableParts)
    {
        if (Part.bDetached || !IsValid(Part.Mesh))
        {
            continue;
        }
        const float InfluenceRadius = FMath::Max(Radius, 90.0f);
        const float Distance = FVector::Distance(Part.Mesh->GetComponentLocation(), WorldPoint);
        if (Distance >= InfluenceRadius)
        {
            continue;
        }
        const float Falloff = 1.0f - Distance / InfluenceRadius;
        Part.AccumulatedStress += Depth * StructuralStressPerDentCm * Falloff;
        if (Part.AccumulatedStress < FMath::Max(1.0f, Part.DetachStressThreshold))
        {
            continue;
        }

        UStaticMeshComponent* DetachedMesh = Part.Mesh;
        DetachedMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        DetachedMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
        DetachedMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        DetachedMesh->SetNotifyRigidBodyCollision(true);
        DetachedMesh->SetSimulatePhysics(true);
        DetachedMesh->AddImpulseAtLocation(Direction * PartDetachImpulse, WorldPoint, NAME_None);
        Part.bDetached = true;
    }
}

void UGCDeformationComponent::RepairAll()
{
    if (bInitialized)
    {
        for (FGCDeformationMeshSection& Section : MeshSections)
        {
            Section.CurrentPositions = Section.OriginalPositions;
            RefreshMeshSection(Section);
        }
    }
    for (FGCDetachableVehiclePart& Part : DetachableParts)
    {
        if (!IsValid(Part.Mesh))
        {
            continue;
        }
        if (Part.bDetached)
        {
            Part.Mesh->SetSimulatePhysics(false);
            Part.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (IsValid(Part.OriginalParent))
            {
                Part.Mesh->AttachToComponent(Part.OriginalParent, FAttachmentTransformRules::KeepWorldTransform);
                Part.Mesh->SetRelativeTransform(Part.InitialRelativeTransform);
            }
        }
        Part.AccumulatedStress = 0.0f;
        Part.bDetached = false;
    }
    DamageLevel = 0.0f;
}

void UGCDeformationComponent::RefreshMeshSection(FGCDeformationMeshSection& Section)
{
	if (!ProcMesh || Section.SectionIndex == INDEX_NONE)
	{
		return;
	}
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
		Section.CurrentPositions, Section.Indices, Section.UVs, Section.Normals, Section.Tangents);
	ProcMesh->UpdateMeshSection(Section.SectionIndex, Section.CurrentPositions, Section.Normals,
		Section.UVs, Section.Colors, Section.Tangents);
}

void UGCDeformationComponent::BeginPlay()
{
    Super::BeginPlay();
    AActor* Owner = GetOwner();
    OwnerCollisionComponent = Owner ? Cast<UPrimitiveComponent>(Owner->GetRootComponent()) : nullptr;
    if (IsValid(OwnerCollisionComponent))
    {
        OwnerCollisionComponent->OnComponentHit.AddDynamic(this, &UGCDeformationComponent::OnOwnerComponentHit);
        OwnerCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &UGCDeformationComponent::OnOwnerComponentBeginOverlap);
    }
    for (FGCDetachableVehiclePart& Part : DetachableParts)
    {
        if (IsValid(Part.Mesh))
        {
            Part.OriginalParent = Part.Mesh->GetAttachParent();
            Part.InitialRelativeTransform = Part.Mesh->GetRelativeTransform();
            Part.AccumulatedStress = 0.0f;
            Part.bDetached = false;
        }
    }
}

void UGCDeformationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(OwnerCollisionComponent))
    {
        OwnerCollisionComponent->OnComponentHit.RemoveDynamic(this, &UGCDeformationComponent::OnOwnerComponentHit);
        OwnerCollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UGCDeformationComponent::OnOwnerComponentBeginOverlap);
    }
    OwnerCollisionComponent = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UGCDeformationComponent::OnOwnerComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!IsValid(HitComponent) || !IsValid(OtherActor) || OtherActor == GetOwner() || !IsValid(OtherComponent))
    {
        return;
    }
    const FVector RelativeVelocity = HitComponent->GetComponentVelocity() - OtherComponent->GetComponentVelocity();
    ProcessOwnerImpact(Hit, RelativeVelocity);
}

void UGCDeformationComponent::OnOwnerComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bFromSweep || !IsValid(OverlappedComponent) || !IsValid(OtherActor) || OtherActor == GetOwner())
    {
        return;
    }
    const FVector OtherVelocity = IsValid(OtherComponent) ? OtherComponent->GetComponentVelocity() : OtherActor->GetVelocity();
    const FVector RelativeVelocity = OverlappedComponent->GetComponentVelocity() - OtherVelocity;
    ProcessOwnerImpact(SweepResult, RelativeVelocity);
}

void UGCDeformationComponent::ProcessOwnerImpact(const FHitResult& Hit, const FVector& RelativeVelocity)
{
    const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    if (ImpactNormal.IsNearlyZero())
    {
        return;
    }
    const float ClosingSpeedCmPerSec = -FVector::DotProduct(RelativeVelocity, ImpactNormal);
    if (ClosingSpeedCmPerSec < 350.0f)
    {
        return;
    }

    float ImpactMassKg = 1400.0f;
    if (IsValid(OwnerCollisionComponent) && OwnerCollisionComponent->IsSimulatingPhysics())
    {
        ImpactMassKg = FMath::Max(1.0f, OwnerCollisionComponent->GetMass());
    }
    const float SpeedMPerSec = ClosingSpeedCmPerSec / 100.0f;
    const float ImpactEnergyKJ = 0.5f * ImpactMassKg * SpeedMPerSec * SpeedMPerSec / 1000.0f;
    const float Severity = FMath::Clamp(FMath::Sqrt(ImpactEnergyKJ / 180.0f), 0.05f, 1.0f);
    const float Radius = FMath::Lerp(55.0f, 170.0f, Severity);
    const float Depth = FMath::Lerp(4.0f, 36.0f, Severity);
    const FVector ImpactPoint = Hit.ImpactPoint.IsNearlyZero() && IsValid(OwnerCollisionComponent)
        ? OwnerCollisionComponent->GetComponentLocation() : Hit.ImpactPoint;
    ApplyDentWorld(ImpactPoint, -ImpactNormal, Radius, Depth);
}

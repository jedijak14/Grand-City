#include "Components/GCHealthComponent.h"
#include "Core/GCGameUserSettings.h"
#include "GameFramework/Actor.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UGCHealthComponent::UGCHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGCHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UGCHealthComponent::HandleDamage);
	}
}

void UGCHealthComponent::Heal(float Amount)
{
	if (IsDead())
	{
		return;
	}
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UGCHealthComponent::Revive()
{
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UGCHealthComponent::HandleDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (bGodMode || Damage <= 0.0f || IsDead())
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (BloodFX && GetWorld())
	{
		if (const UGCGameUserSettings* Settings = UGCGameUserSettings::GetGCSettings())
		{
			if (Settings->bBloodEnabled)
			{
				const FVector Loc = DamagedActor ? DamagedActor->GetActorLocation() + FVector(0, 0, 120.0f) : FVector::ZeroVector;
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BloodFX, Loc);
			}
		}
	}

	if (IsDead())
	{
		OnDeath.Broadcast(DamageCauser);
	}
}

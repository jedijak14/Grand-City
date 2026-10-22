#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GCHealthComponent.generated.h"

class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCOnDeath, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGCOnHealthChanged, float, NewHealth, float, MaxHealth);

UCLASS(ClassGroup = (GrandCity), meta = (BlueprintSpawnableComponent))
class GRANDCITY_API UGCHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGCHealthComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bGodMode = false;

	/** Optional Niagara blood burst, spawned on damage when enabled in settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|FX")
	UNiagaraSystem* BloodFX = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FGCOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FGCOnHealthChanged OnHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Revive();

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const { return Health <= 0.0f; }

protected:
	UFUNCTION()
	void HandleDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);
};

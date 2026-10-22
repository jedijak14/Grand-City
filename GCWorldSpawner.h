#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCWorldSpawner.generated.h"

class AGCPedestrian;
class AGCVehiclePawn;

/**
 * Keeps the sandbox alive around the player: traffic cars (AI-driven),
 * pedestrians, and police that scale with wanted stars. Density comes from settings.
 * Place ONE of these in the map.
 */
UCLASS()
class GRANDCITY_API AGCWorldSpawner : public AActor
{
	GENERATED_BODY()

public:
	AGCWorldSpawner();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AGCPedestrian> PedestrianClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AGCVehiclePawn> TrafficVehicleClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AGCVehiclePawn> PoliceVehicleClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnMinRadius = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnMaxRadius = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float DespawnRadius = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float ManageIntervalSeconds = 1.0f;

private:
	UPROPERTY()
	TArray<AActor*> SpawnedPeds;

	UPROPERTY()
	TArray<AActor*> SpawnedTraffic;

	UPROPERTY()
	TArray<AActor*> SpawnedPoliceCars;

	UPROPERTY()
	TArray<AActor*> SpawnedPolicePeds;

	FTimerHandle ManageTimer;

	void ManagePopulation();
	bool FindGroundSpawn(const FVector& AroundLocation, FVector& OutLocation) const;
	void CleanupList(TArray<AActor*>& List, const FVector& PlayerLocation);
	void SpawnPedestrian(const FVector& PlayerLocation);
	void SpawnTrafficCar(const FVector& PlayerLocation);
	void SpawnPoliceCar(const FVector& PlayerLocation);
	void SpawnPolicePed(const FVector& PlayerLocation);
};

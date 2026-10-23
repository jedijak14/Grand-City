#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GCPlayerController.generated.h"

class AGCVehiclePawn;
class UGCMainMenuWidget;
class UGCSettingsWidget;
class UGCLoadingScreenWidget;
class UGCIdentitySetupWidget;
class UGCIdentityCardWidget;
class UGCPoliceActionWheelWidget;
class UGCBoardingPassWidget;
class AGCDayNightManager;

/**
 * Player controller: wanted-decay visibility checks + dev console commands.
 * Press ~ in game and type e.g.  GCSpawnCar   GCSetWanted 4   GCSetTime 22.5   GCSetJob President
 */
UCLASS()
class GRANDCITY_API AGCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGCPlayerController();

	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Dev")
	TSubclassOf<AGCVehiclePawn> DefaultVehicleClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGCMainMenuWidget> MainMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGCSettingsWidget> SettingsClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGCLoadingScreenWidget> LoadingClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGCIdentitySetupWidget> IdentitySetupClass;

	/** Main menu is opt-in for test maps: run GCMenu, or enable this in a BP child. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	bool bShowMainMenuOnStart = false;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

    void ShowPoliceActionWheel(bool bVehicleMode);
    void HidePoliceActionWheel();
    void ShowBoardingPass(bool bPaperCopy);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLoadingScreen(float Seconds = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMenus();

	// ------------------------------------------------ dev console commands (Exec)
	UFUNCTION(Exec)
	void GCGiveMoney(int32 Amount = 5000);

	UFUNCTION(Exec)
	void GCSetWanted(int32 Stars = 3);

	UFUNCTION(Exec)
	void GCSpawnCar();

	UFUNCTION(Exec)
	void GCTeleport(float X = 0.0f, float Y = 0.0f, float Z = 500.0f);

	UFUNCTION(Exec)
	void GCSetTime(float Hour = 12.0f);

	UFUNCTION(Exec)
	void GCGod();

	/** Jobs are milestone 3, but the role flag (incl. President) is settable now as requested. */
	UFUNCTION(Exec)
	void GCSetJob(FString JobName);

	UFUNCTION(Exec)
	void GCRepairVehicle();

	UFUNCTION(Exec)
	void GCMenu();

	UFUNCTION(Exec)
	void GCSettingsMenu();

	UFUNCTION(Exec)
	void GCLoadingTest(float Seconds = 4.0f);

	UFUNCTION(Exec)
	void GCHideMenus();

private:
	float WantedCheckTimer = 0.0f;
    float AutosaveTimer = 0.0f;
    TWeakObjectPtr<AGCDayNightManager> CachedDayNightManager;
	bool bPauseMenuOpen = false;
	void UpdateWantedVisibility(float DeltaTime);
    void EnsurePersonalVehicles();
	void TogglePauseMenu();
    void OpenPhoneBoardingPass();
    void PrintBoardingPass();
	void ShowPauseMenu();

	UPROPERTY(Transient)
	UGCMainMenuWidget* MainMenuWidget = nullptr;

	UPROPERTY(Transient)
	UGCSettingsWidget* SettingsWidget = nullptr;

	UPROPERTY(Transient)
	UGCLoadingScreenWidget* LoadingWidget = nullptr;

	UPROPERTY(Transient)
	UGCIdentitySetupWidget* IdentitySetupWidget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UGCIdentityCardWidget> IdentityCardWidget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UGCPoliceActionWheelWidget> PoliceActionWheelWidget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UGCBoardingPassWidget> BoardingPassWidget = nullptr;

	UGCIdentitySetupWidget* EnsureIdentitySetup();
    UGCIdentityCardWidget* EnsureIdentityCard();
    void ToggleIdentityCard();
	void ShowIdentitySetup();

	bool bReturnToMainOnSettingsClose = false;
	FTimerHandle LoadingTimer;
	float LoadingElapsed = 0.0f;
	float LoadingDuration = 4.0f;

	UGCMainMenuWidget* EnsureMainMenu();
	UGCSettingsWidget* EnsureSettings();
	UGCLoadingScreenWidget* EnsureLoading();
	void SetMenuInput(bool bUI);

	UFUNCTION()
	void HandleMenuPlay();

	UFUNCTION()
	void HandleIdentitySetupComplete();

    UFUNCTION()
    void HandlePoliceActionSelected(FName ActionID);

	UFUNCTION()
	void HandleMenuSettings();

	UFUNCTION()
	void HandleSettingsClosed();

	UFUNCTION()
	void TickLoading();

	UFUNCTION()
	void HideLoadingScreen();
};

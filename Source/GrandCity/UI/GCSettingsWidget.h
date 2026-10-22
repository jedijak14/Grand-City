#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSettingsClosed);

/** Native settings panel backed by the project's persistent UGameUserSettings subclass. */
UCLASS()
class GRANDCITY_API UGCSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Grand City|Settings")
	FGCSettingsClosed OnClosed;

	UFUNCTION(BlueprintCallable, Category = "Grand City|Settings")
	void LoadFromSettings();

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> BloodCheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> WavesCheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> GrassCheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> ClothCheckBox;

	UPROPERTY(Transient)
	TObjectPtr<USlider> TrafficDensitySlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> PedestrianDensitySlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> DayLengthSlider;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton;

	bool bLoadingSettings = false;

	UFUNCTION()
	void HandleSettingChanged(bool bIsChecked);

	UFUNCTION()
	void HandleTrafficDensityChanged(float Value);

	UFUNCTION()
	void HandlePedestrianDensityChanged(float Value);

	UFUNCTION()
	void HandleDayLengthChanged(float Value);

	UFUNCTION()
	void HandleBackClicked();
};

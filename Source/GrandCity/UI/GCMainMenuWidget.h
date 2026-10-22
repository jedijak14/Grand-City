#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCMainMenuWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCMainMenuAction);

/** Lightweight native main menu; no designer-created UMG assets are required. */
UCLASS()
class GRANDCITY_API UGCMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Grand City|Menu")
	FGCMainMenuAction OnPlayRequested;

	UPROPERTY(BlueprintAssignable, Category = "Grand City|Menu")
	FGCMainMenuAction OnSettingsRequested;

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleSettingsClicked();
};

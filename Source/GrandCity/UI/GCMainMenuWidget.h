#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCMainMenuAction);

/** Cinematic native story menu and pause overlay; uses no designer-created UMG asset. */
UCLASS()
class GRANDCITY_API UGCMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Gridlock|Menu")
	FGCMainMenuAction OnPlayRequested;

	UPROPERTY(BlueprintAssignable, Category = "Gridlock|Menu")
	FGCMainMenuAction OnSettingsRequested;

	void SetPauseMode(bool bInPauseMode);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ExploreTile;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MainActionLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NavLabel;

	bool bPauseMode = false;

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleSettingsClicked();
};

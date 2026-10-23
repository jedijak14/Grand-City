#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCIdentitySetupWidget.generated.h"

class UButton;
class UEditableTextBox;
class UComboBoxString;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCIdentitySetupComplete);

/** First-run player profile form; generates a session citizen ID after the player enters a display name. */
UCLASS()
class GRANDCITY_API UGCIdentitySetupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Gridlock|Identity")
	FGCIdentitySetupComplete OnIdentitySetupComplete;

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> NameField;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> DateOfBirthField;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> SexChoice;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> LicenseChoice;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UFUNCTION()
	void HandleCreateIdentity();
};

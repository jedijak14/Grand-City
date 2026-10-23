#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GCPoliceActionWheelWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGCPoliceActionSelected, FName, ActionID);

/** Hold-E contextual police wheel. Every action is routed through evidence/CP checks. */
UCLASS()
class GRANDCITY_API UGCPoliceActionWheelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Police|UI")
    FGCPoliceActionSelected OnActionSelected;

    void ConfigureForVehicle(bool bInVehicleMode);

protected:
    virtual void NativeConstruct() override;

private:
    void RebuildWheel();
    bool bVehicleMode = false;
    bool bBuilt = false;
};

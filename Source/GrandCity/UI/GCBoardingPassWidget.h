#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/GCFlightTypes.h"
#include "GCBoardingPassWidget.generated.h"

/** Native phone/paper presentation of the player's persisted airline boarding pass. */
UCLASS()
class GRANDCITY_API UGCBoardingPassWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void ConfigurePass(const FGCFlightTicket& InTicket, bool bInPaperCopy);

protected:
    virtual void NativeConstruct() override;

private:
    FGCFlightTicket Ticket;
    bool bPaperCopy = false;
    bool bConfigured = false;
    bool bBuilt = false;

    void Rebuild();
};

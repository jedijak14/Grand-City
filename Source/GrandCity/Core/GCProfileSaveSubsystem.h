#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GCProfileSaveSubsystem.generated.h"

class AGCPlayerState;

/** Local single-player save service. Critical transactions save immediately; the controller also autosaves periodically. */
UCLASS()
class GRANDCITY_API UGCProfileSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static const FString& GetSlotName();

    bool LoadPlayerState(AGCPlayerState* PlayerState, bool& bOutHadSave);
    bool SavePlayerState(const AGCPlayerState* PlayerState);
    bool DeleteProfileSave();
};

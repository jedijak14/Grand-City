#include "Core/GCGameMode.h"
#include "Character/GCCharacter.h"
#include "Core/GCPlayerController.h"
#include "Core/GCPlayerState.h"
#include "UI/GCHUD.h"

AGCGameMode::AGCGameMode()
{
	DefaultPawnClass = AGCCharacter::StaticClass();
	PlayerControllerClass = AGCPlayerController::StaticClass();
	PlayerStateClass = AGCPlayerState::StaticClass();
	HUDClass = AGCHUD::StaticClass();
}

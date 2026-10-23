# Citizen ID and witness-driven crime

## Citizen identity

On first run, Gridlock asks for a display name, date of birth, optional sex field, and driver-license class. The form validates the entered date and age (16–110) before issuing a session citizen ID. Press **J** to show the runtime-generated citizen ID card; press **J** again or **Escape** to close it. The portrait is a procedural illustration, not a captured player photo. Profile data and property ownership are currently held in `AGCPlayerState` for the current game session and are not saved between runs.

## Witness model

`UGCCrimeSystemSubsystem` applies witness visibility and sound-radius checks before a crime can add wanted stars. A clear pedestrian sightline can witness an incident; sound-only reports are reduced by a blocking visibility trace and are queued for a short delay. A police witness raises wanted status immediately. Homicide, detected pickpocketing, store robbery, illegal property claims, vehicle theft, and unsuppressed firearm noise use the same reporting path. Suppressed gunfire does not submit the unsuppressed noise event.

## Property doors

`AGCPropertyDoorActor` checks the property ID against the player's deed list and moves the character to an editor-assigned destination marker. Stolen deeds currently grant the same access as purchased deeds. The starter-city editor script adds a paired greybox entrance and exit for Pacific View; the destination is a blockout, not a furnished interior or persistent housing system.

## Verification

The Python starter-map script is syntax-checked in this workspace. No Unreal Engine 5.8 build or gameplay session is available here, so the C++ changes and editor-map generation still require verification in the Unreal Editor.

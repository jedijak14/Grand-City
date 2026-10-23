# Police bodycam prototype

When the player's current job is Police, F6 toggles a separate first-person camera attached near the character's upper torso. The HUD adds a red recording indicator, elapsed timecode, and viewfinder corner marks; F6 returns to the normal camera. Non-police characters receive an access-denied subtitle. Use the existing `GCSetJob Police` developer command to test the access check.

This is a local camera-mode prototype, not persistent video recording, evidence playback, a police-computer CCTV system, or a complete police job. It has not been compiled or tested in Unreal; verify camera activation, mesh visibility, HUD safe areas, and input behavior in-editor.

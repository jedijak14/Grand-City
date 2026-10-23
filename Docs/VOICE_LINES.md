# Voice lines — script, files & wiring

Original synthesized voice performances generated for this project (not ripped recordings).
Audio files live in `AudioVoice/`; run `Tools/ImportModels_Editor.py` to import them to
`/Game/Audio/Voice`, then assign them in Blueprint children on the `Voice` component's
**Lines** array with the exact LineIDs and subtitles below.

## Check-in agent → `BP_CheckInDesk` (child of `GCComputerTerminalActor`)

| LineID | File | Script |
|---|---|---|
| `CheckInGreeting` | `checkin_greeting.wav` | "Hello, and welcome to Gridline Air! I can book your flight and check you in right here. Today's departures are on the screen, and I'll get your boarding pass printed in no time." |
| `BookingConfirmed` | `checkin_booking_confirmed.wav` | "You're booked! Shall I check you in now? Just press E, and I'll print your boarding pass with your seat and gate." |
| `CheckInComplete` | `checkin_complete.wav` | "All checked in! Your seat and gate are on your boarding pass. Head through when you're ready, and have a great flight!" |
| `PaymentDeclined` | `checkin_payment_declined.wav` | "I'm sorry, that payment didn't go through. The fare is on the screen if you'd like a different flight or class." |
| `WrongAirport` | `checkin_wrong_airport.wav` | "That ticket departs from a different airport — check the airport code on your booking." |
| `AlreadyCheckedIn` | `checkin_already_checked_in.wav` | "You're already checked in. Head through to the gate and board the aircraft when you're ready." |
| `GateAnnouncement` | `gate_announcement.wav` | "Now boarding at the gate. Please have your boarding pass ready, and welcome aboard." |

## Cabin crew → `BP_Airliner` (child of `GCAirlinerPawn`)

| LineID | File | Script |
|---|---|---|
| `WelcomeAboard` | `crew_welcome_aboard.wav` | "Welcome aboard Gridline Air! Please find your seat, stow your carry-on bags, and make yourselves comfortable. We'll be on our way shortly." |
| `SafetyBriefing` | `crew_safety_briefing.wav` | "Cabin crew, doors to automatic. Ladies and gentlemen, please fasten your seatbelts, stow your tray tables, and ensure your seat backs are upright for takeoff." |
| `CaptainClimb` | `captain_climb.wav` | "Good day from the flight deck. We're climbing smoothly to our cruising altitude. The weather en route is clear, so sit back and enjoy the flight." |
| `CruiseService` | `crew_cruise_service.wav` | "We've reached cruising altitude. Your seatback screen has movies, music, games, and a live flight map. We'll be coming through with refreshments shortly." |
| `Descent` | `captain_descent.wav` | "This is your captain. We're beginning our descent. Cabin crew, please prepare the cabin for arrival, and passengers, please return to your seats and fasten your seatbelts." |
| `FinalApproach` | `captain_final_approach.wav` | "Cabin crew, prepare for landing. We'll be on the ground in just a few minutes." |
| `Landed` | `crew_landed.wav` | "Welcome to our destination! Please remain seated with your seatbelt fastened until the aircraft comes to a complete stop at the gate." |
| `GateArrival` | `crew_gate_arrival.wav` | "We've arrived at the gate. Thank you for flying Gridline Air. Press E to head into the terminal." |

## NPCs → `BP_Ped` / `BP_PolicePed` (children of `GCPedestrian`)

Prefix rule: civilians randomly play `Ambient*`, officers play `Police*`, only when the
player is within ~11 m, with per-NPC cooldowns.

| LineID | File | Script |
|---|---|---|
| `AmbientGreeting1` | `npc_ambient_greeting1.wav` | "Hey there! Beautiful day in the city, isn't it? Careful crossing near downtown though — traffic's been wild lately." |
| `AmbientComment1` | `npc_ambient_comment1.wav` | "I swear the boats down at the marina get faster every summer." |
| `AmbientComment2` | `npc_ambient_comment2.wav` | "Heard there's a flight sale at the airport today. Might finally visit the North Island." |
| `PoliceStop1` | `police_stop1.wav` | "Police! Stop right there and put your hands where I can see them!" |
| `PolicePursuit1` | `police_pursuit1.wav` | "Suspect is on the move — all units, converge on my position!" |
| `CivilianScream` | To be recorded/imported | Short nonverbal alarm or “Help! Somebody call the police!”; used when a civilian witnesses a killing or catches a pickpocket |
| `PoliceCrimeWitness` | To be recorded/imported | Short police alarm callout for a witnessed crime |

The existing lines have recordings. The new crime-reaction lines are hooks only: no scream/panic recording is included yet. Assign the line IDs in pedestrian Blueprint `Voice` arrays, or set `PanicScreamSound` for a nonverbal scream clip. If unassigned, a subtitle fallback is used. Player `HeavyBreathingSound` is an optional assignable loop on the character Blueprint; no breathing recording is included yet.

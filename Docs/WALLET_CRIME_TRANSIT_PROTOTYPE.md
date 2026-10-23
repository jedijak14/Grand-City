# Wallet, crime, jobs, and transit prototype

## Wallet and jobs

Cash remains the physical wallet balance. Bayline Tap, CoastPass, and Harbor Card are fictional tap-to-pay options backed by a separate shared digital balance; Bayline Credit is a revolving credit line with a $5,000 limit. Press P to cycle the selected tender; priced interactions that call `AGCPlayerState::SpendMoney` use it. Press K to pay $500 of credit debt from the digital balance. Job roles receive a small direct-deposit paycheck every 120 seconds. The role can currently be set with `GCSetJob`; job-specific task loops, applications, and progression are not implemented yet.

## Crime and witnesses

Press G near a civilian to attempt a pickpocket. Crouching and approaching from behind lowers the chance of being noticed; a fast, upright, or front-facing approach is likely to get caught. A successful attempt steals the target's carried cash once. A detected attempt causes an alarm and adds wanted status.

Killing an NPC no longer automatically adds wanted stars. Nearby living pedestrians must have line of sight to the incident; witnesses flee or raise an alarm, with police witnesses adding more wanted stars. Vehicle impacts can damage or temporarily knock down pedestrians; some civilians carry a coffee prop-state that triggers optional spill FX/audio when they fall. Navmesh wandering plus RVO avoidance is enabled for pedestrians. These are prototype behaviors; collision, recovery poses, coffee effects, detection distances, and path navigation need Unreal tuning.

## Buses

The greybox city adds three linked bus stops and a static coach silhouette. Within a stop trigger, Tab selects a linked destination and E pays the fare with the selected wallet before a short fade/transfer to the other stop. This is a transit interaction prototype, not a moving/AI-driven bus, scheduled service, or passenger seating system.

## Audio

Pedestrians expose an optional `PanicScreamSound` slot and use `CivilianScream` / `PoliceCrimeWitness` voice-line IDs, with subtitle fallback. Player characters can be given a `HeavyBreathingSound` asset in a Blueprint child; no scream or breathing recordings are included/assigned in this source checkout, so import and assign suitable original audio before expecting those sounds to play. No Unreal audio playback has been tested here.

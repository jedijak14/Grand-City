# Moving elevator prototype

`AGCElevatorActor` is a one-car/one-shaft lift. Two `GCLocationActor` landing stations share the
car and point at each other; pressing the interaction key calls the lift and associates the caller
with that trip. If needed, the car first travels to the caller. It opens its split doors and waits
for that player to physically enter the cabin before starting the dwell, closing, and travelling to
the linked destination. A bounded boarding timeout leaves the car safely open if the caller does
not board. Its collision floor is a movable component so grounded characters can ride it; split
doors animate on the car, and an overlap sensor reopens them if a pawn obstructs closing. A
world-space cabin display combines a live floor/direction readout with a timed carousel of three
fictional hotel campaigns: Tidehouse Hotels, Meridian House, and Astara Grand. Each ad has its own
logo and property image, rather than reusing the reference loyalty-program marks. The starter-map
builder imports this art into `/Game/HotelRewards` so Unreal can package it, and creates a lobby/roof
pair with an open guide-post shaft.

This is a functional prototype, not production elevator engineering or finished architectural art.
UE compilation and gameplay tests must confirm moving-base carry, overlap ordering at landings,
collision behavior, and door timing. It currently supports a single car and single active request;
call queuing, certified safety interlocks, emergency behavior, accessibility controls, audio, and
finished cab/shaft assets remain to be built; the prototype doorway sensor itself still needs UE
collision and timing validation.

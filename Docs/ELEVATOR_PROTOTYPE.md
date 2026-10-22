# Moving elevator prototype

`AGCElevatorActor` is a one-car/one-shaft lift. Two `GCLocationActor` landing stations share the
car and point at each other; pressing the interaction key calls the lift. Split doors open at the
caller, hold for a configurable boarding window, close, then the car moves vertically to the other
landing. Its collision floor is a movable component so grounded characters can ride it; the doors
animate on the car. If the car is on the other floor, it first travels to the caller; it then opens and holds for the boarding window before closing and serving the linked destination. The starter-map builder creates a lobby/roof pair and an open guide-post shaft.

This is a functional prototype, not production elevator engineering or finished architectural art.
UE compilation and gameplay tests must confirm moving-base carry, overlap ordering at landings,
collision behavior, and door timing. It currently supports a single car and single active request;
call queuing, obstruction sensing, emergency behavior, accessibility controls, audio, and finished
cab/shaft assets remain to be built.

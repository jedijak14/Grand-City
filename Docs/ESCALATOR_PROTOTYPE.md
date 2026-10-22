# Moving escalator prototype

`AGCEscalatorActor` builds a circulating closed tread path from a configurable ramp run/rise and
updates instanced tread transforms at 30 Hz. Walking `ACharacter` riders in the ride corridor are
temporarily moved in `MOVE_Flying` mode because Unreal's walking mode discards vertical input;
their original movement mode and fly speed are restored when they leave the corridor or the actor
ends play. Pawn collision on the moving treads is overlap-only so the capsule does not snag on each
riser. Treads use the engine cube by default and can be replaced with a properly pivoted tread mesh;
the starter greybox builder places two copies beside its airport test markers with simple blockout
rails.

This establishes interaction and motion, not a finished terminal asset. UE compilation, tread
collision stability, character movement/step-off behavior, frame cost, and a polished escalator
model/material/handrail set still need validation and art production in Unreal. The generated
starter map remains a greybox.

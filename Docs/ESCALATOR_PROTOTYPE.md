# Moving escalator prototype

`AGCEscalatorActor` builds a circulating closed tread path from a configurable ramp run/rise,
updates instanced tread transforms at 30 Hz, and pushes walking `ACharacter` riders along the ramp
while they are inside its ride trigger. Treads use the engine cube by default and can be replaced
with a properly pivoted tread mesh; the starter greybox builder places two copies beside its
airport test markers with simple blockout rails.

This establishes interaction and motion, not a finished terminal asset. UE compilation, tread
collision stability, character movement/step-off behavior, frame cost, and a polished escalator
model/material/handrail set still need validation and art production in Unreal. The generated
starter map remains a greybox.

# Homes and electronics prototype

`AGCRealEstateActor` exposes a property listing and price. Press E while near the assigned realtor to purchase the deed if the wallet covers it. The existing weapon/damage loop can kill the realtor; after that, E can seize the listing without payment, record it as stolen property, and add three wanted stars. PlayerState tracks owned/stolen property IDs for the session. This is game-only crime simulation; it is not a real-world guide. The starter-city map adds one plainly labeled blockout home and realtor.

`AGCDeviceStoreActor` provides a Tab-browsable, E-to-purchase device catalog with fictional makes and operating systems. It includes two phones, a tablet, and a desktop; phone purchases update the active phone model/OS in PlayerState, while every item is added to the owned-device list. This is an inventory/gameplay prototype, not yet a full electronics storefront, device-use simulation, persistent save, or in-game 3D product display.

Unreal compilation and play testing remain required. The property and device records currently live in PlayerState for the current session; they do not yet persist across save/load.

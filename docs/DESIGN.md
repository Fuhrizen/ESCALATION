# Design: map command and controlled escalation

Version 0.2 changes the default loop to nation preview → income construction → research → facility-based unit production → visible movement → conventional capture. The map occupies the screen between two compact HUD bars. Management panels exist only when opened; contextual land/building/unit windows appear on selection.

## Campaign and economy

The alternate-history campaign starts in 1920 with modern geometry. 24 generated regions in each of two active nations receive simulation; a deterministic land-site planner places facilities and four infantry formations per side. Rival formations and regional resistance remain static.

At 1×, one day takes 0.25 real seconds. 1920–2055 takes about 3h 25m; conquest can finish sooner. The 2–4 hour design goal is not measured match balance.

All costs/outputs are fictional currency units. Construction pays its full cost upfront. Normal sites take 35 days; missile sites 120. Cancelling unfinished construction or unit production refunds 50% once. Regions allow eight sites and five road levels. Each road level adds 15% regional building income and 20% movement speed. Economy research improves output and baseline national income; Logistics improves movement. Research centres turn funded upkeep into research points.

| Building | Currency cost | Gross daily income / role |
| --- | ---: | --- |
| Farm | 650 | 8 |
| Commercial district | 1500 | 18 |
| Industrial district | 2200 | 25 |
| Power station | 1200 | 12; utility income in this simplified model |
| Research centre | 2000 | Base 6 RP × funding/research modifiers |
| Army base | 1600 | Ground-unit production |
| Arms plant | 1800 | Ground-unit production |
| Airbase | 2800 | Air-unit production; air research gate |
| Shipyard | 3500 | Naval production / amphibious access; coast and naval research required |
| Missile site | 18000 | Missile production / strategic delivery; distinct research gates |
| Radar station | 2200 | Visual range marker; no detection/fog simulation yet |
| Command centre | 500 | Administrative site |

Baseline national income is 20/day; upkeep includes operational buildings, units and research funding. Income is reduced by lost stability. The earlier extraction/refining resource economy remains in the headless core for regression and old snapshots, but new graphical campaigns use currency only.

## Research and units

The tree has 112 projects: eight fields × fourteen decade tiers, 1920–2050. Edges show preceding-tier and explicit cross-field requirements. Decade gates impose linear escalation; specialization grants 35% progress in one branch. A 12-job queue persists progress; the active project can be cancelled. Economy, research and logistics affect their respective rates; military branches unlock unit families and increase abstract combat support. Dates are scenario gates, not precise invention claims.

Nine unit families have separate costs, production times, markers and movement classes. Ground production occurs at army bases/arms plants; air at airbases; naval at coastal shipyards; missile batteries at missile sites. Research gates advanced production. Unfinished units cannot move. Formation capacity is 120 living player units. Ground units follow owned-region adjacency plus a target frontier; researched, operational shipyards enable abstract amphibious transfer. Air and sea orders use direct strategic movement, without obstacle/navigation simulation.

An invasion requires declared war. Arriving ground forces are required for capture; nearby assigned air/naval forces add abstract support. Regional resistance governs progress and attrition. Ownership, regional economy and strategic frontier indicators change after capture. Conquering all rival regions wins. The single active offensive target and static defender are deliberate POC constraints.

## Strategic weapons and next work

Warheads require nuclear and research branches and cost 75,000. Nuclear delivery additionally requires intercontinental missile research and an operational missile site. Release is explicitly confirmed. The rival's fixed second strike disables industry, destroys much of the treasury, reduces stability and ends the campaign. This is scripted MAD, not a model of interception, fallout or survivable delivery systems.

Next iterations need playtesting, terrain/port-aware navigation, distinct weapon ranges and air missions, detection/fog of war, dynamic defenders, continuous fronts and expanded diplomacy. Geometry/texture LOD and regional/spatial caches should precede full-world activation. This project implements original mechanics inspired by public descriptions, not every ICBM: Escalation system.

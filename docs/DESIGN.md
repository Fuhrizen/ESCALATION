# Design: direct map command and controlled escalation

Version 0.3 changes the loop to nation preview → allocate regional output → research or military production → direct movement orders → local combat and town occupation. The map occupies the screen between compact HUD bars. Construction, research and forces panels open on demand; selecting land, buildings or units opens contextual controls.

## Campaign and production

The alternate-history campaign starts in 1920 using modern reference geography. Two active nations receive 24 generated regions each, starting military/industrial facilities and four infantry formations per side. Other nations remain static. At 1× the deterministic simulation advances four days per real second; render interpolation smooths movement between daily steps.

The extraction chains, treasury payments, tax buildings and manually upgraded roads are retired. Controlled regions contribute production work from development and operational industrial sites. A shared research/production slider divides this output. All active player construction, compatible facility unit orders and warhead production share the production portion equally, redistributing spare work when a job finishes. A site cannot produce units while it is under construction, disabled, destroyed or outside player control. Pending units at a seized base are cancelled. Cancellation redirects future production; work already spent is lost.

If one side has no eligible jobs, its allocation redirects to the other side. Research needs an operational research centre. Laboratories and science research improve research conversion; industrial research improves regional output. Allocation changes update estimates without advancing simulation.

| Active facility | Work | Role |
| --- | ---: | --- |
| Army base | 80 | Ground formations |
| Arms plant | 100 | Ground formations, output, warhead manufacturing |
| Airbase | 120 | Aircraft production |
| Shipyard | 160 | Naval production and abstract embark capability; coastal placement |
| Missile site | 180 | Missile batteries and strategic delivery |
| Research centre | 120 | Research conversion and nuclear science support |
| Radar station | 90 | Map range marker; radar detection remains future work |
| Command centre | 40 | Administrative map site |
| Industrial district | 140 | Regional output |

Regions support eight facilities. Every valid queue is admitted without a treasury payment. Pending jobs compete for work, so spamming construction slows unit replacement and early research investment limits immediate force buildup.

## Research

The 112-project catalog retains stable IDs for save compatibility, with eight military/industry/science/support fields. Prerequisites include platform progressions and separate support branches. Research advances to the highest completed tier in its field; specialty investment grants 35% faster progress for that field.

Years are reference eras, never locks. Effective work is:

`baseWork × (1 + 0.12 × yearsAhead + 0.002 × yearsAhead²)`

The cost is frozen when queued and persisted with progress. A player can rush a branch after satisfying prerequisites, trading a large amount of shared national effort for earlier military capability. The UI shows the rush multiplier and queue work/ETA. These formulas are original tuning, not verified ICBM balance data.

## Geography and command

Pinned Natural Earth v5.1.2 populated places and generalized strategic roads are bundled offline. City selection favors population and administrative coverage with spacing to reduce clutter. Imported country affiliation controls assignment; coordinates outside the simplified 110m coastline are moved to the nearest in-country triangle edge. Regions with no imported city receive an explicitly labeled strategic objective. No fictional real-city names are generated.

Ground formations within 0.10 latitude degrees (longitude scaled by latitude) of imported road geometry gain exactly 15% speed. Support research provides a separate movement modifier. Roads are generalized strategic vectors, not street navigation. A 5-degree spatial index limits nearby road queries and map drawing.

Camera targets ease exponentially. WASD/arrows pan in screen-relative geographic space; middle/right dragging pans the map or rotates the globe. Pointer-focused wheel zoom is eased, with dateline wrapping and an orthographic anchor solve. Typing and captured UI input suppress camera commands. Unit rendering interpolates the prior/current deterministic positions and uses the same positions for picking.

## Tactical combat and occupation

Declare war, select formations, and right-click a destination. Orders specify geography rather than an abstract offensive target. Ground and naval movement validate routes against actual map land/sea; the synthetic theatre link cannot by itself cross ocean. Operational naval infrastructure and naval research provide the prototype's abstract embark capability. Air units use direct strategic movement.

Nearby hostile units fight automatically using weapon class, range, health and branch modifiers. Aircraft and ships support combat without occupying cities. Infantry, armour, artillery and special forces apply local occupation pressure. A contested town cannot finish capture while opposing occupying forces remain. Region ownership changes only when all contained towns/cities/objectives belong to one side. Capturing every rival region wins; losing every home region defeats the player.

The basic enemy reacts to real incursions and recaptures lost settlements. It uses the same geographic movement planner. It is a defensive AI, not a full strategic economy or diplomacy agent. A signed unit pressure field drives land-masked color paint and a marching contour; it is separate from authoritative region ownership.

## Strategic weapons and scope

Warheads require nuclear/science technology, an operational arms plant and laboratory, and 1,200 shared production work. Queued plus available warheads are limited to 20. Delivery requires missile research, an operational missile site and an explicit confirmation. The prototype retains its scripted second strike and mutual-destruction outcome; it does not simulate missile flight, interception or fallout.

This revision implements an original ICBM-inspired foundation. Complete reference unit/loadout catalogs, air missions, carrier aviation, detection/fog, diplomacy, espionage, interception and full-world simultaneous simulation remain outside current parity. Desktop interaction and balance require playtesting; smoke screenshots and deterministic tests do not establish match quality or reference-game equivalence.

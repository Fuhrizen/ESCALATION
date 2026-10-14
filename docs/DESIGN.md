# Design: paced combat and predictable command

Version 0.4's loop is choose a nation → weight research versus construction → fabricate → place ready infrastructure → produce formations → move, fight and occupy. Research, construction, military commands and inspection have stable screen locations. The player sees durations, percentages and states rather than a work-point economy.

## Interface and assets

The essential top HUD shows identity, peace/war, date, pause/speed and options. The bottom rail opens construction, research, forces and log. A fixed right inspector retains the selected formation/building/region; another hovered entity gets a compact preview without displacing selection or commands. Unit actions precede detailed statistics. Construction has Catalog, Queue and Ready tabs with visible counts.

Research uses a stable full-width command surface. Field navigation and allocation stay above a left programme/technology inspector and a horizontally advancing spline canvas. A foundation opens two actual sibling prerequisite chains; selection inspects a card and Queue is explicit. Search, Fit, Current and middle-drag/wheel browsing are available. Large text does not require a different movable-window arrangement.

Licensed DejaVu Sans regular/bold replace the tiny bitmap typography. One original geometric icon vocabulary supplies map markers, catalog cards and inspector portraits. Cyan/amber/green/coral states are reinforced with words and progress, not color alone. Selected/hovered weapon circles are geodesic projections of the exact mechanical range. Radar is labeled a graphical coverage preview; it does not silently promise unimplemented detection. Industrial effects highlight the affected region.

## Manufacture and allocation

The weight slider divides national research and construction/production capacity. Idle allocation redirects to eligible active jobs, and the UI explains this. There is no treasury or work-currency payment. Manufacturing and technology values remain internal tuning constants; prospective and active ETAs use current output, allocation, eligibility, backlog and doctrine. Estimates assume those inputs remain unchanged.

Building manufacture is FIFO. It competes with active units and queued warheads, with equal stream sharing and spare capacity conserved at completion events. A completed building adds a token to Ready inventory. It has no regional output, radar preview, production access or site-slot effect until placed. Placement validates the token, technology, controlled land, capacity and coast before consuming anything. Invalid placement retains the token and mode. Right click/Escape cancels placement; Shift repeats while inventory remains.

Unit manufacturing remains tied to operational compatible facilities. Disabled/seized/destroyed sources cannot complete queued units. Warheads require operational arms manufacturing and research infrastructure. Cancellation loses completed effort and redirects future capacity. Ready inventory and FIFO progress persist in version-4 snapshots.

Research retains 112 stable IDs and prerequisite logic for compatibility. The reference year never locks a project. Early research multiplies duration by `1 + .12 × yearsAhead + .002 × yearsAhead²`, frozen at queue admission. A specialization grants 35% faster progress in its field. The graph makes either sibling path available from its prerequisites; it does not promise automatic prerequisite purchasing.

## Tactical pacing

Each deterministic tick represents 0.25 real seconds at 1×. The date still advances one scenario day per tick; travel/combat are intentionally compressed strategy-game values, not real historical speeds. Displayed timings are baseline 1×; faster game speed shortens real waiting.

| Formation | Strategic km/s at 1× | Weapon range km | Base damage/s |
| --- | ---: | ---: | ---: |
| Infantry | .65 | 3 | 1.3 |
| Armour | 1.0 | 5 | 1.8 |
| Artillery | .55 | 30 | 1.0 |
| Fighter | 8 | 35 | 1.6 |
| Bomber | 5 | 20 | 1.5 |
| Destroyer | 1.4 | 45 | 1.5 |
| Submarine | 1.1 | 20 | 1.6 |
| Special forces | .8 | 2 | 1.2 |
| Missile battery | .6 | 120 | 1.2 |

`unit_stats.cpp` owns these shared capabilities and geodesic distance. Infantry takes about 154 seconds to cross 100 km off-road. Support adds 3% travel speed per tier; imported roads retain the separate 15% ground bonus. Weapon damage integrates elapsed time and a `.35 + .65 × health/100` firepower factor, allowing an equivalent engagement to resolve rather than asymptotically preserving both units.

The highest-population settlement in each region is its main objective; a stable source index breaks ties. Ground occupants must be within 5 km and inside that region. A main objective takes at least 150 seconds at 1×, other towns 90. Occupation strength is capped so stacked formations cannot skip those minimums. Opposing occupiers freeze progress; defenders recover abandoned progress. Every town/objective, including the main city, must belong to the new owner before the region changes hands.

Defensive AI uses the shared movement planner and actual weapon standoff against troops. Retaking a town instead requires entering the occupation radius and correct region; artillery cannot stop at its much longer weapons radius. Pursuit clears when the incursion disappears. Capturing all rival regions wins; losing all home regions defeats the player.

## Map and controls

Pinned Natural Earth sources provide 15,734 major roads with hierarchy and 600 named major river segments. Roads have smooth logarithmic zoom fades and rounded path joins; rivers are decorative and affect no mechanics. The 21 explicit compact territory mappings leave 155 playable groups. Parent names remain readable, child geometry/population/cities join the parent, and internal former-country borders are suppressed. Grouping is a gameplay choice, not a historical border claim.

WASD/arrows pan immediately with normalized diagonals. Middle/right drag track directly with a small smoothing remainder; wheel zoom remains eased and pointer anchored. Settings expose pan, drag, zoom, UI scale and map-layer toggles. Generic keyboard-navigation capture no longer blocks the map; text entry, focus loss and modals do. Pointer gestures remember their furthest travel and retain ownership across docks, preventing a drag-away-and-back from producing an order or inventory deployment. Opening pause cancels gestures and pending zoom.

## Persistence and scope

Version 4 includes fabricated building progress and ready counts. Older unfinished placed sites migrate to one fabrication job each, preserving their completion fraction. Destroyed historical placeholders preserve producer indices. Expanded country groups may invalidate a campaign whose player/opponent or partition changed; load validates a candidate transactionally.

The world still renders 177 source records while only the player and one rival receive detailed economic/tactical simulation. Full air missions, loadouts, detection/fog, interception, diplomacy and full-world strategic AI remain further work. The terrain texture/coastlines remain generalized world-view assets; this iteration does not claim street-scale navigation or exact reference-game parity. Nuclear release retains explicit confirmation and the prototype's scripted second-strike ending.

Research sources and interface acceptance scenarios are in [UX_REDESIGN.md](UX_REDESIGN.md); numerical tuning here is original, not extracted reference-game data.

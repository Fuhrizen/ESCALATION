# Design: industrial preparation, controlled escalation

## Player promise

The player controls a nation, rather than an isolated arsenal. War draws down stocks produced in peace; research changes the ways those stocks can be created and used. A nation that overcommits without maintaining power and transport will stall before territory is secured.

## Campaign structure

The technical prototype starts in 1920 using modern borders. The intended long campaign progresses toward the 2050s, with a 2–4 hour baseline duration and faster optional speeds. Each decade exposes another linear tier in eight research branches. A permanent selected specialization improves that branch's research progress by 35%; it never blocks access to the rest of the tree.

A daily simulation tick takes 0.25 real seconds at 1x. 135 years × 365 days × 0.25 seconds = 12,318.75 seconds, or 3h 25m 19s. The calendar intentionally uses fixed 365-day years. Prototype conquest can finish sooner. Future scenarios need measured playtests, escalating rival defences and objective pacing to meet the duration target.

## First playable loop

Nation preview -> take command paused -> inspect reserves -> choose research -> build and connect sites -> resume -> manage shortages -> recruit -> declare war -> capture a supplied frontier -> extend transport -> continue.

Static defence makes the prototype useful for observing the player's economy. Enemy AI, scripted aggression and enemy economic automation are excluded; rival facilities exist as capture targets. Starting-site placement is deterministic and based on generated resource/development scores, occupancy and valid land samples.

## Economy

All quantities are fictional game units. Modern country GDP provides only a bounded starting tax modifier; the model is not an economic forecast.

| Facility | Inputs / constraints | Output per simulated day at base efficiency |
| --- | --- | --- |
| Mine | Power, supply; local ore quality | 3 × ore quality ore |
| Oil well | Power, supply; local oil quality | 2 × oil quality crude oil |
| Steelworks | 2 ore; power and supply | 1.4 steel |
| Refinery | 1.5 crude oil; power and supply | 1.2 fuel |
| Arms factory | 0.6 steel, 0.25 fuel; power and supply | 1.4 supplies + 0.45 equipment |
| Power plant | Connected supply | 18 grid capacity |
| Laboratory | Power, supply, funding, treasury upkeep | Base 4 RP × funding factor |
| Supply depot | Connected supply | Capital/distribution site |
| Shipyard | Fleet logistics, 1 steel, 0.5 fuel | 0.8 equipment; enables amphibious supply |
| Missile silo | Intercontinental missiles; supplied construction | Strategic delivery capability |

Extraction feeds refining before factories run in a stable daily ordering. Inventories are bounded; no recipe can consume more than available stocks. Grid shortage scales production. Bankrupt countries operate at reduced throughput and lose stability. Upkeep includes facilities, brigades and research spending.

Each enabled route starts at eight throughput units. Upgrades add four. Logistics research adds 15% capacity per tier. A facility's inbound capacity is the widest path from the capital, bounded by the weakest route on that path. Multiple sites in one sector share its inbound capacity. National inventories remain pooled: this is an intentionally simple transport abstraction, with per-edge shared-flow scheduling deferred.

Construction costs treasury and 30 steel upfront. Normal facilities take 35 supplied days; silos take 120. Disconnected construction pauses. Facility and route toggles let players shed demand or test interruptions. Newly captured sectors begin without a player's supply route.

## Research

112 projects cover eight fields and fourteen decade gates (1920 through 2050). Each project requires the preceding project in its branch. Nuclear deterrence additionally requires Computing centres in the Research branch. Research is queued, has persistent progress, and can be cancelled. Dates are scenario progression gates, not assertions about the precise historical invention of technologies. Future-era titles are speculative.

Effects are implemented as branch levels. Economy increases industrial output and income; Research increases lab output; Logistics increases capacity. Army improves brigade strength/attrition; Air, Naval and Special Operations add combat support. Ballistic capability unlocks warhead stockpiling and delivery. Units are abstract at this stage; an explicit unit/prototype catalog will replace the support modifiers later.

## Conventional war

Brigades cost $1,000 and 20 equipment. An offensive targets one adjacent enemy sector connected to a supplied friendly sector. Defenders are static regional resistance values. Daily progress uses brigade strength, researched support, fuel, supplies and route capacity. Attrition reduces brigades. Captured sites join the player's economy but need supplied routes to function. Naval research and a connected, completed shipyard allow a landing when no supplied land adjacency exists.

The front overlay shows enemy/friendly sector adjacencies. It moves after capture. It is not a continuous tactical front with unit manoeuvre, terrain or separate formations. Conquering all rival sectors wins the POC; exhausting forces loses it.

## Strategic weapons

Warhead production requires 1950-era Ballistic and Research capabilities, $25,000, 400 steel and 250 fuel. Delivery needs a completed, enabled, connected silo, unlocked in the 1960 tier, costing $18,000 and 120 construction days.

The POC's rival has a fixed retaliatory deterrent. A player must explicitly confirm that strategic release triggers a scripted second strike, destroys most reserves, disables industry, reduces stability and forces, and ends the campaign in mutual destruction. This makes conventional victory economically preferable. It is a deliberately coarse rule: delivery/interception simulation, mobile launchers, survivable second-strike forces, arms control, escalation thresholds and persistent fallout are future work.

## Incremental roadmap

1. Validate this economy loop with desktop playtests and production-rate telemetry.
2. Add explicit unit families, equipment templates, maintenance and facility-specific build queues.
3. Replace pooled stocks with depot inventories and shared-capacity cargo scheduling; introduce terrain/port constraints.
4. Add continuous fronts and formation-level orders, then optional enemy AI.
5. Activate additional nations in budgets; add diplomacy, trade and deterrence models.
6. Benchmark 177-nation simulation, add geometry LOD and high-resolution administrative data only where needed.

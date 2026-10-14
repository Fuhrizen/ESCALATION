# World, naval and frontline update

## Work segments and dependencies

Renderer/map presentation and UI tools were implemented in parallel. Naval research informed separate platform and weapon implementations. Persistent ownership and combat performance shared the simulation header and were integrated before aggregate save/load, native input and OpenGL validation.

## Reference research

Official ICBM: Escalation naval diary (17 November 2023):
https://www.matrixgames.com/news/icbm-escalation-dev-diary-3

Cruisers emphasize land-attack and anti-ship guided missiles with limited sonar/ASW. Destroyers are versatile submarine escorts. Attack submarines are hunter-killers; cruise missile submarines emphasize cruise missile magazines. These roles informed the appended Cruiser and MissileSubmarine classes and sonar-gated target acquisition. Their numeric stats are gameplay approximations, not extracted original game values. Sonar governs acquisition, not map visibility. Carriers, SSBNs, transport ships and the entire original game ruleset are not implemented by this update.

Official world diary (30 May 2024):
https://www.slitherine.com/news/icbm-escalation-dev-diary-7

The source game couples regional economics, territory capture and population centers. This project retains its imported city population references and existing region inspector. Small procedural city clusters now appear in campaign views; landmark/city culling and faster label sorting reduce presentation overhead. Geography uses the existing elevation/coast atlas plus illustrative vegetation, mountain and tundra textures; this is not a new surveyed terrain dataset.

## Controls

- Bottom-right **Measure km**: click a start point, move to preview, then click an endpoint. Distances and the route use great-circle geometry, including the dateline. Click again to begin another measurement. Escape or right-click cancels.
- Research canvas: wheel scrolls vertically; Shift-wheel scrolls horizontally; Ctrl-wheel zooms around the cursor; right/middle drag pans. Fit supports much larger trees.
- City selection retains source population, damage and ownership statistics.

## Territory and performance

Ground formations exert local occupation influence on sparse 0.1-degree geographic cells. Occupied cells retain ownership after departure, pause and save/load until opposing formations retake them. Region economics still use settlement occupation; local ground capture does not automatically annex a whole region. Save version 9 adds occupied cells and preserves old unit IDs.

Frontline sampling now invalidates on camera or ownership/campaign revision instead of wall-clock time. The cached ownership field avoids per-sample scans of troops. Combat builds spatial buckets once per tick and searches conservative range envelopes, while city occupation groups formations by region.

Simulation still advances at fixed ticks and rendering still draws frames for camera motion, projectiles and UI; this change removes unnecessary checks rather than switching the whole client to event-only drawing. Full-world multi-faction campaigns remain beyond the present two-nation simulation.

## Original data

Provided technology, unit and weapon records can be mapped to schemas and used for game-stat fidelity. Readable source files or representative samples are most useful. Exact behavior depends on whether the files encode it; undocumented binary formats require investigation. Public dev diaries alone do not establish exact combat values.

## Validation (14 October 2026)

Clean Debug client build: all five CTest suites passed (projection, simulation, tactical, native input, real OpenGL smoke). Optimized Release core build: all three suites passed. After adding an explicit globe scene to the smoke sequence, both client/input checks passed again. Rendered map, research canvas and globe images were inspected. Release diagnostic: 100 formations 0.49 ms/tick, 500 formations 2.96 ms/tick, 1,000 formations 6.46 ms/tick on this environment. See frontline-performance.md for scenario limitations and the earlier baseline comparison.

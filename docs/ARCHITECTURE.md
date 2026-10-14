# Architecture and performance

## Boundaries

`escalation_core` is independent of a window system and OpenGL. `World` imports map data, triangulates polygons with holes and computes shared-edge land neighbours. `Simulation` owns campaign state, production, research, routes, combat and persistence. `Renderer` owns GPU resources. The desktop client owns Dear ImGui panels, input, camera state and the real-time accumulator.

The headless executable links only the core. Core tests can run without desktop packages, network downloads or a GPU. Camera and tactical tests also link the dependency-free core. GLFW and Dear ImGui are fetched at release tags; picojson, earcut and stb_image are vendored with notices and licences. SurfaceAssets loads two bundled textures: the relief raster and flag atlas.

## Geometry

`data/world.geojson` is a compact derivative of Natural Earth v5.1.2's 1:110m admin-0 country GeoJSON. The pipeline preserves original coordinates and geometry, retaining only code, name, population, GDP and label coordinates. Polygons/multipolygons and holes are retained. Antimeridian-crossing rings are unwrapped before triangulation and queries are wrapped into a polygon's longitude range.

Country polygons are triangulated once. The renderer subdivides long triangle edges for globe curvature, uploads immutable buffers once and transforms lon/lat in the vertex shader. The same geometry supports equirectangular 2D and orthographic globe rendering. A small GPU palette texture changes country/sector colour without rebuilding world vertices. Flat mode repeats the mesh at longitude offsets -360, 0 and +360 to preserve seam continuity.

Active countries are subdivided into 24 gameplay sectors each. Deterministic area-weighted land samples and farthest-point selection generate seed sites; original triangles are clipped against Voronoi halfplanes. This preserves coastlines, holes and disconnected land. Neighbour relations come from shared Voronoi edges. A closest-sector link establishes the prototype theatre between the two nations. Ground movement cannot cross ocean; the deferred naval system provides no landing capability.

Only 48 regions receive economic and war simulation. All other countries remain static map features. Sector geometry and buffers are regenerated on campaign creation/load, not per frame. Country picking uses point-in-polygon tests; active sector picking uses clipped convex patches. Higher-resolution map data will need a spatial index and multiple geometry LODs. Country picking normalizes explicit gameplay territory aliases; merged geometry is appended once at load. The map has 177 source records and 155 playable groups.

Relative elevation is sampled by geographic coordinates in the fragment shader for shaded relief, with political color blended above it. This changes appearance only. The bundled 2048×1024 raster and 110m coastlines have finite detail at 512× zoom. Flags are baked into one 1536×864 atlas, with source SVGs and mapping retained; no per-flag runtime downloads or separate texture bindings are needed.

## Simulation timing and storage

Daily ticks are deterministic. The client accumulates real time at 0.25 seconds per day, clamps an individual frame's contribution and limits catch-up to avoid an unbounded frame stall. Rendering remains independent of time speed. Pausing halts simulation.

Campaign data uses vectors for regions, settlements, facilities, units and paths, fixed arrays for compatibility stocks/levels, and a research deque. Active national output is allocated once per daily tick between research conversion and shared production work. Region ownership is derived from settlement control after local combat. The enemy uses the same terrain-sampled visibility-graph movement planner as player orders.

Geography imports are cached in World. Roads have a 5-degree spatial index covering each segment's bounding cells, including dateline crossings; nearby queries return deduplicated candidates. Rendering uses zoom-dependent road and city overlays, and signed local unit pressure for frontline paint. Higher-resolution geometry still needs additional LOD and broader spatial caches.

Version-4 saves add fabrication queues and ready inventory alongside settlement identity/position/population/ownership/capture, frozen research job cost and strategic production work. Unit orders include geometric waypoints. Earlier saves migrate inactive economic facilities and production work. Loading builds a candidate campaign and validates fields before replacing the running state. Geometry derives from bundled world data; writes use temporary files and retain the previous backup during replacement.

## Drawing budget

The renderer batches world land, borders, ocean, graticules and sector overlays into a handful of buffers/draws. It does not issue one GL call per country or facility. Flat mode repeats world/overlay draws three times. Native UI and unit/building/frontier annotations are batched by Dear ImGui. Zoomed marker positions use the same projection as map picking; depth layers do not enlarge x/y coordinates. The native HUD leaves the map as the default full play area.

The POC prioritizes reliable OpenGL 3.3 hardware coverage rather than compute shaders or engine-scale abstraction. It has no frame-rate-independent full-world claim. Rendering on a headless software driver validates shaders/layout, not target GPU performance.

## Full-planet scaling path

- Keep inactive nations asleep and advance low-frequency national summaries until they enter a player's theatre.
- Cache supply reachability per nation and allocate shared edge capacity when cargo simulation is introduced.
- Partition fronts/spatial queries; maintain adjacency updates locally on capture.
- Move expensive geometry baking to an asset pipeline. Use 110m/50m/10m LODs and administrative detail only for visible theatres.
- Preserve deterministic fixed-step state and separate immutable map data from mutable simulation data.
- Benchmark CPU cost, GPU submission, memory and worst-case tick bursts separately on representative hardware.
- Consider worker threads for independent regional jobs after deterministic ordering and dependency boundaries are established.

No 177-nation simulation or large-scale tactical benchmark is claimed in this iteration.

## Command and pacing redesign

The App implementation is split into `ui_hud.inl`, `ui_panels.inl`, `ui_research.inl` and `ui_map.inl`, included inside its class so common selection, camera and campaign state remain shared. `ui_icons.hpp` supplies exact scalable vector glyphs for units and facilities. Bundled regular/bold fonts have explicit redistribution notices.

`unit_stats.cpp` is the single source for kilometres, strategic travel speeds, weapon ranges and DPS. Movement integrates `.25` seconds per deterministic tick; combat uses the same geodesic distance. Occupation minimums use real seconds at 1× rather than region-sized geographic rates. Positive low-health firepower ensures symmetric engagements eventually resolve.

Fabrication is a serial building stream sharing production with unit and warhead streams. Completion events conserve residual capacity and immediately activate the next FIFO item. ETA queries use the same eligibility rules, queued backlog and doctrine multiplier. They assume allocation/output remain unchanged. Ready inventory never contributes output or occupies a map slot. Placement and its ghost use one validation method. Legacy placed jobs migrate into the stream while retaining historical site-vector indices.

Map-owned pointer gestures retain ownership across docks and record peak travel, preventing an excursion-and-return from becoming a click. Focus loss/modal opening cancels the gesture and pending zoom. Native ImGui event tests exercise the actual App controls without creating a rendering context; OpenGL smoke remains a separate genuine EGL test.

## Ground deployment and capabilities (0.6.0)

Save schema 6 persists Unit.deployed. Manufacturing jobs and completed ready units keep stable unit-record indices, but only deployed records participate in map interaction, movement, combat and occupation. Capability APIs expose real unit/site effects to the paired inspector. Specific completed technologies drive damage, protection, movement, range and equipment. Air/naval numeric enum slots remain migration tombstones; no active gameplay uses them. Research visibility gates each UI surface and the canvas derives layout from prerequisite depth and branch lanes.

## 0.7.0 geographic and weapon extensions

The renderer samples a mipmapped packed surface atlas (relative elevation, illustrative coastal distance, land mask). `tools/prepare_surface.py` derives it from bundled height and country geometry; `data/assets.json` retains provenance. City and landmark geometry is original screen-projected isometric artwork, with detail fading by zoom. City selection uses campaign settlements first, then the global reference gazetteer.

Naval technology IDs append after the stable 112 ground technologies. Ships use ocean routing and coastal shipyard production/deployment rather than land occupation. Weapon profiles drive targeting, separate range rings and inspection; stable weapon slots preserve ammunition through upgrades. Engagement records persist projectile travel and guided targeting. Save version 7 preserves weapon cooldowns, magazines, reserves, port service and in-flight engagements; previous ground saves extend their technology arrays.

## 0.8.0 command and strike integration

The camera covers the whole game surface; HUD hit regions protect the transparent header, left inspector and right controls. Map overlays clip below the header so geographic labels cannot overwrite command text. Point-targeting state belongs to App and cancels through Escape/right click; simulation APIs reject invalid launches without consuming rounds. Brief operation notifications expose validation feedback.

The ownership contour combines signed distances to controlled campaign cells with army pressure, refreshed at 8 Hz and on camera/ownership changes. Planned routes preserve exact movement geometry while queued segments use screen-space dash cadence. Strategic sites create owner/source-indexed engagements with no unit defender index; ballistic height is presentation, while flight timers and area impacts are simulation state. Save version 8 persists impact lifetime and site reloads.

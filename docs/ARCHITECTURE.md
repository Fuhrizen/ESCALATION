# Architecture and performance

## Boundaries

`escalation_core` is independent of a window system and OpenGL. `World` imports map data, triangulates polygons with holes and computes shared-edge land neighbours. `Simulation` owns campaign state, production, research, routes, combat and persistence. `Renderer` owns GPU resources. The desktop client owns Dear ImGui panels, input, camera state and the real-time accumulator.

The headless executable links only the core. Core tests can run without desktop packages, network downloads or a GPU. GLFW and Dear ImGui are fetched at release tags; picojson, earcut and stb_image are vendored with notices and licences. SurfaceAssets loads two bundled textures: the relief raster and flag atlas.

## Geometry

`data/world.geojson` is a compact derivative of Natural Earth v5.1.2's 1:110m admin-0 country GeoJSON. The pipeline preserves original coordinates and geometry, retaining only code, name, population, GDP and label coordinates. Polygons/multipolygons and holes are retained. Antimeridian-crossing rings are unwrapped before triangulation and queries are wrapped into a polygon's longitude range.

Country polygons are triangulated once. The renderer subdivides long triangle edges for globe curvature, uploads immutable buffers once and transforms lon/lat in the vertex shader. The same geometry supports equirectangular 2D and orthographic globe rendering. A small GPU palette texture changes country/sector colour without rebuilding world vertices. Flat mode repeats the mesh at longitude offsets -360, 0 and +360 to preserve seam continuity.

Active countries are subdivided into 24 gameplay sectors each. Deterministic area-weighted land samples and farthest-point selection generate seed sites; original triangles are clipped against Voronoi halfplanes. This preserves coastlines, holes and disconnected land. Neighbour relations come from shared Voronoi edges. A closest-sector link establishes the prototype theatre between the two nations. Disconnected sectors require naval landing capability.

Only 48 regions receive economic and war simulation. All other countries remain static map features. Sector geometry and buffers are regenerated on campaign creation/load, not per frame. Country picking uses point-in-polygon tests; active sector picking uses clipped convex patches. Higher-resolution map data will need a spatial index and multiple geometry LODs. Country picking normalizes explicit gameplay territory aliases; merged geometry is appended once at load. The map has 177 source records and 170 playable groups.

Relative elevation is sampled by geographic coordinates in the fragment shader for shaded relief, with political color blended above it. This changes appearance only. The bundled 2048×1024 raster and 110m coastlines have finite detail at 512× zoom. Flags are baked into one 1536×864 atlas, with source SVGs and mapping retained; no per-flag runtime downloads or separate texture bindings are needed.

## Simulation timing and storage

Daily ticks are deterministic. The client accumulates real time at 0.25 seconds per day, clamps an individual frame's contribution and limits catch-up to avoid an unbounded frame stall. Rendering remains independent of time speed. Pausing halts simulation.

Campaign data uses vectors for regions, facilities, units and routes, fixed arrays for stocks/levels, and a small research deque. The default economy accumulates currency income/upkeep per operational site, advances construction/production and uses roads for regional upgrades. Legacy production recipes run in an explicit stable order. Connected capacity uses a widest-path search over enabled player-owned routes. This small POC computes paths on demand; full-world simulation should cache a graph snapshot and invalidate it on topology/ownership changes rather than repeat path searches for every site.

Version-2 saves contain nation codes, date, economy mode, technology flags, research progress, regional ownership/roads, placed-site coordinates/cancellation, unit production/health/positions/waypoints/orders and legacy route state. Loading builds a candidate campaign and validates it before replacing the active campaign. Geometry is regenerated from bundled world data and deterministic nation indexes; changed map versions require a save migration. Writes use a temporary file and retain a previous `.bak` snapshot during replacement.

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

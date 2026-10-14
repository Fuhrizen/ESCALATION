# ESCALATION

A native C++17 / OpenGL 3.3 strategy prototype about building a nation before committing it to war. Inspired by the conventional/nuclear escalation of **ICBM: Escalation** and the map-led national management of **State of Command**, with original code and presentation.

Version 0.6.0 focuses on **readable command, deliberate combat pacing and manufacture-before-placement**. Units use shared kilometre-based capabilities; research and construction are controlled by a weight slider and completion estimates. Unit and building inspection uses paired bottom-right capability and selection boxes. Completed units and buildings appear in the permanent icon rail; circular controls open dedicated pages. Major roads, decorative rivers, proportional typography and original vector icons are bundled offline.

![Native map and national command](docs/map.png)

[More native screenshots](docs/SCREENSHOTS.md)

## Playable now

- 177 source map records, 155 playable territory groups after 21 explicit compact-territory merges and Antarctica exclusion. Grouped land, population, cities and production belong to the parent nation; old internal country borders are removed from display.
- Responsive WASD/arrows, direct middle/right drag and pointer-anchored wheel zoom; pause/options with saved pan, drag, zoom, UI scale and map-layer preferences.
- Fixed unit/building/region inspector, compact hover previews, essential HUD, and separate construction Catalog, Queue and Ready tabs. Licensed DejaVu Sans fonts and a shared original vector-icon vocabulary improve readability.
- A branching spline research view with field navigation, two actual sibling prerequisite paths, node inspection, explicit queue action, search, Fit/Current and canvas panning. Years increase early research duration; they never block technology.
- Research/construction weights affect completion times. Building fabrication completes into inventory; placing a ready item validates controlled land, technology, capacity and coast before consuming it. Ready buildings have no map benefits until placed.
- Slower tactical movement and local weapon ranges shared by combat and range visuals. At 1×, infantry crosses 100 km in about 154 seconds off-road. Base infantry weapon range is 3 km.
- Main-city occupation takes at least 150 real seconds at 1×; other towns take at least 90. Stacking formations cannot bypass these minimums. Every regional settlement, including its main city, must be controlled. Defenders contest occupation and react to incursions.
- 15,734 imported major road segments, with smoothly faded hierarchy and rounded geographic paths; nearby ground movement retains its 15% bonus. 600 named major river segments are decorative.
- Selected/hovered weapon ranges match simulation kilometres. Radar coverage is explicitly a graphical preview; industrial-site inspection highlights the affected region.
- Version-6 saves retain ready unit deployment, branching research, fabrication inventories and queued movement. Core, native input and genuine OpenGL rendering checks run through CTest.

## Build on Bazzite with Podman

Run as your normal user with your installed Podman. No Compose plugin, Docker daemon, host package installation, bind mounts, persistent build volumes or privileged containers are required.

```sh
git pull
sh ./build.sh linux
./dist-linux/escalation
```

Cross-compile Windows from the same Bazzite PC, or build both:

```sh
sh ./build.sh windows
sh ./build.sh all
```

The first build needs internet access for `docker.io/library/ubuntu:24.04`, Ubuntu packages and the pinned GLFW / Dear ImGui source tags. Output is x86-64. Linux uses the X11 backend, including XWayland in a Wayland desktop, and requires a working OpenGL 3.3 host driver. The package uses Ubuntu 24.04's glibc baseline; older distributions are not targeted. Run the extracted executable on the host, keeping `data/` next to it.

The build compiles **during image construction** using an explicit `Containerfile` and repository context. The Linux image build runs core/projection tests, a real software-OpenGL smoke check and a relocated-package data lookup check. Windows cross-compilation validates package files but does not execute Windows binaries. Only successful builds are extracted: the script creates an unstarted temporary container, copies `/artifacts` to a staging directory, validates the package, removes that container and publishes the host-owned output. Neither source nor output is bind-mounted into a container; SELinux relabelling is unnecessary.

```text
dist-linux/escalation
dist-linux/escalation_headless
dist-linux/data/
dist-windows/escalation.exe
dist-windows/escalation_headless.exe
dist-windows/data/
```

Rebuilds keep the last successful package in a `dist-<platform>.previous-<timestamp>-<pid>` directory. Failed builds/copies leave the existing output intact. Incomplete extraction staging directories are retained for inspection. Build images remain local; no executables are uploaded to GitHub. Artifact images are for extraction, not for running the graphical application inside a container.

The default compile parallelism is two jobs. For a complete rebuild without image-layer reuse:

```sh
CLEAN_BUILD=1 JOBS=2 sh ./build.sh linux
```

`CONTAINER_ENGINE=docker sh ./build.sh linux` selects direct Docker commands if explicitly desired. Podman is the default. The old Compose files and container entrypoint build scripts have been removed. Existing volumes from that earlier workflow are not used or deleted.

Validate the host orchestration separately with `python3 tests/test_build_pipeline.py`. Actual Podman/Bazzite execution must be confirmed on a host with Podman; this assistant workspace has neither Podman nor Docker installed.

## Native development

Install CMake 3.20+, a C++17 compiler and Git. On Debian/Ubuntu the desktop build additionally needs `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`. A desktop session and OpenGL 3.3 are required to play.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/escalation
```

The simulation can build without fetching any dependencies or requiring graphics:

```sh
cmake -S . -B build-core -DESCALATION_BUILD_CLIENT=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
./build-core/escalation_headless FRA 49275
```

A compiler-only validation path is also provided (see the validation record for the optional sanitizer path):

```sh
python3 tools/verify.py
```

For Linux render smoke tests without a desktop, install `libegl1 libgl1-mesa-dri`, build with `-DESCALATION_NULL_PLATFORM=ON`, and run `ctest`. This creates an actual EGL OpenGL context, renders ten UI/projection states and checks GL errors. The normal desktop build can also run `./build/escalation --smoke build/smoke`. PPM screenshots are generated locally, not published as releases.

## First campaign

1. Select a nation and take command. Campaigns open paused.
2. Open Research (`R`), select a technology card, inspect its prerequisites/effect and queue it. Adjust the research/construction weight to prioritize the programme. Estimates are shown at 1×.
3. Open Build (`B`). Queue a facility in Catalog, inspect its fabrication in Queue, then use Place from Ready. Valid placement consumes one completed building; invalid placement keeps inventory. Shift repeats while tokens remain.
4. Click an operational military facility to queue compatible units. Construction and unit/warhead manufacturing share the same production allocation.
5. Select units or drag a formation box. Shift adds/removes units; right-click orders and Shift-right-click queues destinations. Declare war before entering hostile land, then occupy enemy towns with ground formations. The main city has the longest minimum occupation.
6. Select units or buildings to inspect their capabilities and commands in the paired bottom-right boxes. Range previews use the same statistics as combat. Use Focus/Fit explicitly; inspection never moves the camera automatically.
7. Open Options to change camera responsiveness, UI scale or map layers. The menu pauses the campaign and suppresses map commands.
8. Strategic research unlocks queued warheads. Delivery requires missile research and an operational missile site; explicitly confirmed nuclear release retains the prototype's scripted second-strike ending.

| Control | Action |
| --- | --- |
| Left click / Shift click | Select / add or remove formation member |
| Drag box | Select formations |
| Right click | Order / cancel placement |
| WASD / Arrow keys | Responsive camera pan |
| Middle drag / Right drag | Pan/rotate, including dragging across docks |
| Mouse wheel | Anchored eased map zoom; research canvas scrolls horizontally |
| B / R / U | Construction / research / forces |
| G / H / F | Projection / fit nation / focus selected unit |
| Space | Tactical pause / resume |
| Escape | Cancel active placement/order mode, otherwise pause/options |
| F5 / F9 | Save / load newest save |

Saves use `%APPDATA%/Escalation` on Windows, or `$XDG_DATA_HOME/escalation` / `~/.local/share/escalation` on Linux. Saves are versioned JSON snapshots; invalid saves are rejected without replacing a running campaign. `ESCALATION_DATA` can override the map-data directory.

## Scope and next work

The default clock advances four simulated days per real second: 1920 to 2055 takes about **3 hours 25 minutes at 1x**. Early conquest can end the POC sooner; 2–4 hour match balance is a design target, not a validated playtime claim.

The military and city-capture systems are an original implementation of the requested foundation. This build does **not** claim full ICBM: Escalation feature or balance parity. Detailed air missions, equipment loadouts, radar/fog, missile interception, carriers, diplomacy and full-world simultaneous AI remain future work.

Subregions are clipped gameplay Voronoi cells, not historical provinces. Generalized Natural Earth roads are strategic data, not street-level navigation. Imported city coordinates/population are modern references; regions without a source city use clearly labeled strategic objectives. The 2048×1024 relief raster is visual elevation, not movement terrain. Deep zoom enlarges the 1:110m coastlines.

Version-5 saves add queued movement orders; version-4 added fabrication queues and ready inventory. Earlier unfinished sites migrate into one fabrication job each with preserved completion fraction; historical placeholders keep unit producer indices stable. Expanded territory grouping can invalidate older campaigns whose selected nation/opponent or regions changed. Keep backups; a rejected load leaves the running campaign intact.

Brief historical introductions are curated for eight nations; others use clearly labelled scenario introductions. Natural Earth GDP/population are modern reference data. Flag aliases and territorial grouping are explicit gameplay choices, not diplomatic claims. Full-world simultaneous simulation and 2–4 hour balance remain targets requiring benchmarks and playtests.

See [design](docs/DESIGN.md), [architecture](docs/ARCHITECTURE.md), [research](docs/RESEARCH.md), and [validation](docs/VALIDATION.md).

### Command and display additions (0.5.0)

The top bar shows a blue Research / orange Build weight slider with percentages and the full campaign date. Options provide automatic UI scaling or a saved manual scale. Dates retain the established 365-day campaign calendar.

| Input | Action |
| --- | --- |
| Shift + right-click | Append a destination to selected units’ queues |
| Ctrl + number / number | Assign / recall a control group |
| Shift + number | Add a control group to the selection |
| Double-click a unit | Select visible friendly units of that type |
| Ctrl + A | Select visible friendly units |
| X | Stop selected units and clear queued movement |

Selected routes include routed queued legs and approximate arrival times at 1x speed. Roads affect the estimate; future combat and ownership changes can delay arrival. Attack ranges use translucent geodesic areas; simplified NATO-inspired frames identify unit roles. Roads and rivers use bounded geographic smoothing and fade into the map across zoom levels.

Release numbering and the Windows/Linux major-release-only build workflow are described in [RELEASES.md](docs/RELEASES.md). X increases require explicit user approval; this feature pass remains **0.6.0**.

### Approved interface and ground foundation (0.6.0)

The approved Concept A layout replaces the bottom bar and wide construction drawer with a permanent slim deployment rail, icon quantity badges and hover names. Four labelled circles open Research, Production, Forces and Log pages. Ready items are completed units/buildings only; production queues live on their page. Click a ready icon and then controlled land to deploy. Invalid placement keeps inventory; Shift repeats and right-click/Escape cancels.

Research has genuine multi-level forks. Locked cards, connections, search results and details are hidden until all prerequisites complete. Revealed nodes keep stable positions. Unit and building capability boxes show simulation-derived effects, acquired equipment and current stats. MANPADS equipment can be researched, but anti-air targeting remains pending the later aircraft system.

Air/naval production, unit combat and sea transport have been removed from active gameplay. Strategic missiles and nuclear deterrence remain. Version-6 migration retires previous air/naval assets and research, keeps completed ground/strategic research and restores necessary prerequisite ancestry. Legacy numeric IDs remain reserved solely to read older saves. Manufactured units remain off-map until deployed and cannot move, fight or capture cities while in inventory.

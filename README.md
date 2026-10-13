# ESCALATION

A native C++17 / OpenGL 3.3 strategy prototype about building a nation before committing it to war. Inspired by the conventional/nuclear escalation of **ICBM: Escalation** and the map-led national management of **State of Command**, with original code and presentation.

Version 0.2 focuses on **map-led command, research and a direct currency economy**. The map fills the play area; the HUD opens temporary panels, and selecting land, buildings or units opens contextual controls. The complete Natural Earth world is rendered, while only your nation and one rival receive simulation. Rival forces are static; a deterministic planner places starting sites.

![Native map and national command](docs/map.png)

[More native screenshots](docs/SCREENSHOTS.md)

## Playable now

- Country selection directly on land, flag-backed nation previews, reference statistics, history and specialization.
- 177 source map features, 170 playable territory groups after six explicit small-state merges and Antarctica exclusion. These are fictional gameplay groups using modern borders.
- Globe and flat projections, pointer-focused wheel zoom up to 512×, close-up unit/building symbols and shaded relief from real Natural Earth III elevation imagery.
- 24 clipped gameplay subregions per active nation, changing ownership and frontier indicators.
- A connected, scrollable **112-node research tree**, eight fields, 1920–2050 decade gates, branch/cross-branch prerequisites, funding, queue cancellation and optional specialization auto-queue.
- Direct map construction, currency costs, daily income buildings, upkeep, cancellation refunds and five road-upgrade levels per region.
- Nine visible unit families: infantry, armour, artillery, fighters, bombers, destroyers, submarines, special forces and missile batteries. Facility-specific production, research unlocks, movement orders, box selection, strength bars and contextual controls.
- Conventional regional capture and expensive strategic warheads with an explicit scripted mutual-destruction consequence.
- Pause/time speeds, shortcuts, tooltips, saves, annual autosaves and save-on-exit. All art/data needed to play is bundled; assets are not downloaded at runtime.

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

1. Start game, click a nation on the map, review its preview and take command. The campaign opens paused.
2. Open Research (`R`), click an available node and set research funding. Space starts time.
3. Open Build (`B`), choose a building, then click controlled land. Shift repeats placement; right click cancels it. Normal construction takes 35 days.
4. Farms, commercial districts, industrial districts and power stations generate currency. Click a region to improve roads and its income.
5. Click an army base, arms plant, airbase or coastal shipyard to produce compatible units. Some unit families require research. Units appear on the map while being produced.
6. Select units individually or with a drag box. Right click to move; declare war through Forces (`U`) before ordering an invasion. Capture needs arriving ground forces; air/naval units provide abstract support.
7. Strategic warheads cost $75,000 and require nuclear/research technology. Delivery needs further research and an operational missile site. The confirmation explains the POC's fixed second strike.

| Control | Action |
| --- | --- |
| Left click land | Preview nation / open regional orders |
| Left click symbol | Building production / unit controls |
| Drag box / Shift click | Select formation / add unit |
| Right click | Order selected units / cancel construction |
| Right drag | Rotate globe / pan map |
| Mouse wheel | Zoom toward pointer, up to 512× |
| B / R / U | Construction / research / forces panel |
| G / H / F | Projection / fit nation / focus selected unit |
| Space | Pause / resume |
| F5 / F9 | Save / load newest save |
| Escape | Close context, then pause menu |

Saves use `%APPDATA%/Escalation` on Windows, or `$XDG_DATA_HOME/escalation` / `~/.local/share/escalation` on Linux. Saves are versioned JSON snapshots; invalid saves are rejected without replacing a running campaign. `ESCALATION_DATA` can override the map-data directory.

## Scope and next work

The default clock advances four simulated days per real second: 1920 to 2055 takes about **3 hours 25 minutes at 1x**. Early conquest can end the POC sooner; 2–4 hour match balance is a design target, not a validated playtime claim.

The nine unit families have production, movement and abstract combat support. This is an original prototype inspired by public descriptions of the reference games, **not full ICBM: Escalation feature parity**. It does not implement detailed air missions, carriers, interception, fog of war, diplomacy, terrain-aware navigation, continuous tactical fronts or autonomous enemy orders.

Subregions are clipped gameplay Voronoi cells, not historical provinces. Ground orders follow an owned-region graph; sea and air movement are direct strategic paths. Frontier indicators update after capture but do not solve a continuous tactical frontage. The 2048×1024 relief raster is visual relative elevation, not metre-scale terrain or landcover. Deep zoom enlarges the 1:110m coastlines; higher-detail geometry/texture LOD is future work.

Version-2 saves persist placed sites, roads, unit production, positions and orders. Core compatibility retains the earlier resource-chain economy; the graphical client upgrades restored campaigns to the currency economy. Changed territory groups or research prerequisites can invalidate some old snapshots; keep their backups.

Brief historical introductions are curated for eight nations; others use clearly labelled scenario introductions. Natural Earth GDP/population are modern reference data. Flag aliases and territorial grouping are explicit gameplay choices, not diplomatic claims. Full-world simultaneous simulation and 2–4 hour balance remain targets requiring benchmarks and playtests.

See [design](docs/DESIGN.md), [architecture](docs/ARCHITECTURE.md), [research](docs/RESEARCH.md), and [validation](docs/VALIDATION.md).

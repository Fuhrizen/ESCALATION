# ESCALATION

A native C++17 / OpenGL 3.3 strategy prototype about building a nation before committing it to war. Inspired by the conventional/nuclear escalation of **ICBM: Escalation** and the map-led national management of **State of Command**, with original code and presentation.

Version 0.3 replaces the currency/resource loop with **shared research and military production**, automatic local combat and settlement-based occupation. Camera movement and zoom are eased, and real roads and populated places are bundled offline. The complete Natural Earth world is rendered; one nation and one rival receive detailed simulation.

![Native map and national command](docs/map.png)

[More native screenshots](docs/SCREENSHOTS.md)

## Playable now

- Nation selection, flags, reference statistics, globe/flat projections and bundled shaded relief.
- Smooth camera panning with WASD/arrows and middle/right drag; eased pointer-focused zoom up to 512×. Unit rendering interpolates daily simulation positions.
- 24 clipped gameplay regions per active nation, imported population centers, decluttered city labels and strategic road lines. Ground units on an imported road receive a 15% movement bonus.
- Regional output shared between research and construction/unit production. Multiple production jobs split the available work; idle allocations redirect to the active side. The old extraction and currency economy is retired.
- Prerequisite research with a saved work cost: early technology is always accessible once its prerequisites are met, with an increasing cost premium for research ahead of its reference era.
- Military facilities, contextual unit production, nine differentiated unit families, and strategic warheads that consume shared production work. Nuclear release retains the confirmed scripted second-strike outcome.
- Move forces directly into hostile areas after declaring war. Local units fight automatically; a region changes hands when every town/city/objective in it is held. A reactive enemy defends incursions and attempts to retake occupied settlements.
- Land-clipped unit pressure and frontline graphics, pause/speed controls, saves, autosaves and save migration.

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
2. Open Research (`R`), queue a project and choose the research/production allocation. Early projects show their additional work cost. Space starts time.
3. Open Build (`B`), choose a military/industrial facility, then click controlled land. Shift repeats placement; right click cancels placement. Construction and queued units consume the same production allocation.
4. Select an operational army base, arms plant, airbase or coastal shipyard to queue compatible units. No treasury or hidden resource inventory is required.
5. Select units or drag a selection box; right click controlled land to move. Declare war through Forces (`U`) before entering hostile territory. Move to enemy settlements to capture them. Every regional objective must be held before ownership transfers.
6. Use the pressure overlay and city control markers to inspect occupation and enemy counterattacks. Roads improve nearby ground movement by 15%.
7. Nuclear/science research unlocks warhead production at an operational arms plant and laboratory. Delivery also requires missile research and a missile site. Release is explicitly confirmed and ends this prototype campaign through its scripted second strike.

| Control | Action |
| --- | --- |
| Left click land/symbol | Nation, region, building or unit context |
| Drag box / Shift click | Select formation / add unit |
| Right click | Order selected units / cancel construction |
| WASD / Arrow keys | Smooth camera pan |
| Middle drag / Right drag | Smooth globe rotation / map pan |
| Mouse wheel | Eased zoom toward pointer |
| B / R / U | Construction / research / forces |
| G / H / F | Projection / fit nation / focus selected unit |
| Space | Pause / resume |
| F5 / F9 | Save / load newest save |
| Escape | Close context, then pause menu |

Saves use `%APPDATA%/Escalation` on Windows, or `$XDG_DATA_HOME/escalation` / `~/.local/share/escalation` on Linux. Saves are versioned JSON snapshots; invalid saves are rejected without replacing a running campaign. `ESCALATION_DATA` can override the map-data directory.

## Scope and next work

The default clock advances four simulated days per real second: 1920 to 2055 takes about **3 hours 25 minutes at 1x**. Early conquest can end the POC sooner; 2–4 hour match balance is a design target, not a validated playtime claim.

The military and city-capture systems are an original implementation of the requested foundation. This build does **not** claim full ICBM: Escalation feature or balance parity. Detailed air missions, equipment loadouts, radar/fog, missile interception, carriers, diplomacy and full-world simultaneous AI remain future work.

Subregions are clipped gameplay Voronoi cells, not historical provinces. Generalized Natural Earth roads are strategic data, not street-level navigation. Imported city coordinates/population are modern references; regions without a source city use clearly labeled strategic objectives. The 2048×1024 relief raster is visual elevation, not movement terrain. Deep zoom enlarges the 1:110m coastlines.

Version-3 saves persist settlement ownership/capture and frozen research costs alongside units, buildings and production. Earlier snapshots are migrated to the new economy and capture objectives; changed geography or research prerequisites can invalidate a snapshot. Loading validates a candidate before replacing the running campaign.

Brief historical introductions are curated for eight nations; others use clearly labelled scenario introductions. Natural Earth GDP/population are modern reference data. Flag aliases and territorial grouping are explicit gameplay choices, not diplomatic claims. Full-world simultaneous simulation and 2–4 hour balance remain targets requiring benchmarks and playtests.

See [design](docs/DESIGN.md), [architecture](docs/ARCHITECTURE.md), [research](docs/RESEARCH.md), and [validation](docs/VALIDATION.md).

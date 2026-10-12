# ESCALATION

A native C++17 / OpenGL 3.3 strategy prototype about building a nation before committing it to war. Inspired by the conventional/nuclear escalation of **ICBM: Escalation** and the map-led national management of **State of Command**, with original code and presentation.

This first iteration focuses on **nation selection, research, industry and logistics**. It renders the complete Natural Earth world in flat-map and globe projections, but simulates only your nation and one neighbouring rival. There is no enemy decision-making AI: a deterministic planner places starting facilities; the war model uses static regional defence.

![Native map and national command](docs/map.png)

[More native screenshots](docs/SCREENSHOTS.md)

## Playable now

- Main menu, searchable country selection, map picking, nation preview, reference statistics and specialization selection.
- 177 Natural Earth map features; 176 playable selections, excluding Antarctica. Modern borders are used in a fictional campaign starting in 1920.
- 24 clipped Voronoi subregions per active nation, control overlays, sector picking, facility markers, supply links and front indicators that move as sectors change hands.
- 112 research projects: 14 eras in each of Army, Air, Naval, Special Operations, Ballistic, Economy, Research and Logistics. Year gates, prerequisites, a cancellable queue and a specialization bonus.
- Extraction -> refining -> equipment/supplies, fuel use, power limits, facility construction, research funding, upkeep, treasury and stockpiles.
- Explicit supply links with capacity upgrades, disabled-route bottlenecks, disconnected construction and supply-dependent offensives.
- Brigade recruitment, war declaration, territorial capture, victory, and a deliberately costly scripted mutual-destruction outcome for nuclear use.
- Pause, four time speeds, keyboard navigation, country search, tooltips, manual saves, annual autosaves and save-on-exit.

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

For Linux render smoke tests without a desktop, install `libegl1 libgl1-mesa-dri`, build with `-DESCALATION_NULL_PLATFORM=ON`, and run `ctest`. This creates an actual EGL OpenGL context, renders eight UI/projection states and checks GL errors. The normal desktop build can also run `./build/escalation --smoke build/smoke`. PPM screenshots are generated locally, not published as releases.

## First campaign

1. Start game, select a nation and choose a specialization. The campaign opens paused.
2. Research **Industrial electrification** or **National laboratories**. Space starts time; G switches projections.
3. Click a controlled sector. Build facilities in Industry, then connect the sector to your capital in Supply. Construction and production require a working route; industry also needs power and input stocks.
4. Upgrade busy routes, expand extraction/refining before adding factories, and watch resource changes. Research funding trades treasury upkeep for research speed.
5. Stockpile fuel, supplies and equipment. Recruit brigades in War, declare war and select an enemy frontier sector for an offensive.
6. After capture, connect the new sector before advancing. Naval research plus a supplied shipyard enables landings on disconnected islands.
7. Nuclear weapons require multiple research branches, costly warhead production, and a supplied silo. The release dialogue explains that this POC's fixed second strike ends the campaign in mutual destruction.

| Control | Action |
| --- | --- |
| Left click | Preview nation / select sector |
| Right drag | Rotate globe / pan map |
| Mouse wheel | Zoom |
| G | Map / globe |
| H | Centre on your nation |
| Space | Pause / resume |
| F5 / F9 | Save / load newest save |
| Escape | Pause menu |

Saves use `%APPDATA%/Escalation` on Windows, or `$XDG_DATA_HOME/escalation` / `~/.local/share/escalation` on Linux. Saves are versioned JSON snapshots; invalid saves are rejected without replacing a running campaign. `ESCALATION_DATA` can override the map-data directory.

## Scope and next work

The default clock advances four simulated days per real second: 1920 to 2055 takes about **3 hours 25 minutes at 1x**. Early conquest can end the POC sooner; 2–4 hour match balance is a design target, not a validated playtime claim.

The current military model has abstract brigades. Army research upgrades their strength; Air, Naval and Special Operations research adds aggregate support; Economy, Research and Logistics alter their respective simulation rates; Ballistic research gates nuclear capability. Separate aircraft, ships, special-forces and missile unit rosters, moving tactical units, diplomatic systems, historical borders, persistent contamination, and full-world simulation are future iterations.

Subregions are gameplay Voronoi cells clipped to country land, **not historical provinces**. Cross-border fronts are sector indicators rather than a continuous tactical frontage solver. Routes represent a pooled strategic distribution network with bottleneck capacity, not per-resource physical cargo movement. Rival retaliation is a fixed scripted rule, not enemy AI. Brief historical introductions are curated for eight nations; other countries have a clearly labelled scenario introduction. Natural Earth population/GDP values are modern reference data, not live or 1920 estimates.

See [design](docs/DESIGN.md), [architecture](docs/ARCHITECTURE.md), [research](docs/RESEARCH.md), and [validation](docs/VALIDATION.md).

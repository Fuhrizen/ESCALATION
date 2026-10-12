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

## Local builds with Docker Compose

Install Docker with Compose v2. On Windows, use Docker Desktop's **Linux containers**. The first build needs internet access for the builder images and the pinned GLFW / Dear ImGui source dependencies.

```sh
docker compose run --build --rm linux
docker compose run --build --rm windows
```

The Linux service compiles and runs the simulation tests. The Windows service cross-compiles x86-64 Windows executables with MinGW; it does not run Windows executables in the Linux builder.

Outputs stay on your machine:

The builder copies the repository into its image at `/source`; it does not rely on a host source bind mount. Run these commands from the repository root and keep `--build` so source changes are included. Only build outputs are bind-mounted to `dist/`.

```text
dist/linux/escalation
dist/linux/escalation_headless
dist/linux/data/
dist/windows/escalation.exe
dist/windows/escalation_headless.exe
dist/windows/data/
```

Launch the executable on the corresponding host OS. Keep its `data/` directory beside it. Docker builds the game; it does not launch the desktop UI. Linux binaries target Ubuntu 24.04 or compatible systems, rather than a universal Linux ABI. The Windows client requires a working OpenGL 3.3 GPU driver. No GitHub Actions or release upload workflow is included.

Build caches use named Docker volumes. To adjust build parallelism, set `JOBS` (default 4). To discard build caches, use `docker compose down --volumes`; local `dist/` outputs remain.

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

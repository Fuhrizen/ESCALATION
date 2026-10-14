# Tactical overhaul work segments

Each segment follows design/research, development, then independent review. The source is shared with explicit file ownership so developers do not overwrite each other.

| Segment | Design and implementation | Dependencies | Review and validation |
| --- | --- | --- | --- |
| Camera and command UI | Eased camera targets, keyboard/mouse pan, pointer anchored zoom, render interpolation; contextual construction, research and forces controls | Camera can start independently; allocation, city and pressure views consume core interfaces | Projection/anchor/smoothing tests; native OpenGL screenshots; input gating inspection |
| Geography | Pinned Natural Earth populated places and roads, reproducible reductions, offline loading and attribution | Independent import; settlement initialization and road speed depend on loaded data | Source/output hashes, country mapping and coverage audit |
| Strategic economy | Regional output divided between research and production; work queues, expensive early tech, save migration | Agreed core fields; final objectives consume geography; production actions share queue contract | Allocation, shared budget, prerequisite/rush, cancellation and save tests |
| Tactical combat | Geographic movement, local attacks, settlement capture, all-objective region ownership, reactive defense and pressure field | Geography for roads/settlements; production actions use economy contract | Automatic fighting, contested capture, partial-region ownership, road speed, AI and navigation tests |
| Integration | Build targets, package required-data checks, headless reporting and documentation | All four segments | Core/projection/tactical tests, build pipeline tests and real software OpenGL smoke |

The four implementation segments run concurrently after contracts are agreed. The independent reviewer checks cross-segment behavior as changes arrive; final build/render validation and publication follow integration.

## Reference fidelity

This change implements the requested foundation with original code and numerical tuning. Public publisher diaries verify map-level command, regional GDP/cities, zoom-dependent information and investment in military technology. They do not provide a complete technology tree, build catalog or internal numerical formulas. Exact 1:1 feature/balance parity is not claimed. Air missions, equipment loadouts, radar detection/fog, interception, diplomacy, carriers and full-world simultaneous AI remain further work.

## Pacing and command redesign (0.4)

Primary-source research and a concrete interaction brief preceded development. Separate developers owned tactical stats/movement, core fabrication/research, camera/docked command surfaces, and geography; the root owned shared vector assets and map rendering. Independent reviewers identified and corrected weapon-radius town recapture, FIFO carryover/ETA eligibility, gesture ownership/modal cancellation and cramped/duplicated UI. Integration added native ImGui event tests and multiple-resolution OpenGL screenshot checks.

Shared `UnitStats`, geodesic ranges, construction inventory APIs, completion estimates and predictable dock fields were agreed before implementation. Camera and geography could run independently. Combat/UI range previews depended on shared stats; construction/graph UI depended on fabrication/ETA contracts. Final rendering, save migration checks and publication follow all segment reviews.

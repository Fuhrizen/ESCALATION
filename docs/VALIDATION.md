# Validation record

Validated 14 October 2026 on Linux x86-64, GCC 13, CMake 4.4.4, GLFW 3.4, Dear ImGui 1.91.8 and Mesa software OpenGL.

| Check | Result |
| --- | --- |
| Native CMake / CTest | Five suites passed: core, projection, tactical, native input and OpenGL smoke |
| Core/map/research/fabrication/save checks | 13,861 passed |
| Projection and camera/gesture checks | 6,206 passed |
| Tactical movement/combat/occupation checks | 11,108 passed |
| Real App / Dear ImGui input events | 12 integration checks passed |
| ASan + UBSan valid-flow checks | 13,837 core, 11,108 tactical, 6,206 camera checks and 49,275-day headless run passed |
| Build pipeline orchestration | Seven tests passed; required package assets include rivers and licensed fonts |
| Native screenshot smoke | Fourteen states per run, no GL errors; 1280×720 at 125% UI scale and 1920×1080 at 100% additionally rendered and inspected |
| Rendering context | Mesa 25.2.8 OpenGL 4.5 core; shaders target 3.3 |
| Geography | 177 source records, 155 playable groups, 15,734 major roads, 600 named river segments |
| Headless benchmark | 49,275 ticks, approximately 295,000 ticks/s in one optimized two-nation run without active war |

Core tests cover country picking/clipping, flags, all playable campaign objective placement, explicit territory grouping, major-road/rivers hierarchy, dateline indexing, allocation eligibility, fabrication-before-placement, ready inventory without map benefits, FIFO completion carryover, unit/warhead fairness, queued backlog ETAs, coastal validation, cancellations, year-free research, frozen premiums, v4 roundtrips, v2/v3 migrations and malformed-save transactional rejection.

Tactical tests cover 120-second infantry travel, exact 15% road and separate support modifiers, kilometre weapon ranges, eventual symmetric battle resolution, 150-second main objectives, 90-second towns, stacked occupation caps, contested progress, same-region occupation, artillery countercapture, stale pursuit, rejected reorder preservation, terrain/ocean/neutral navigation and saved movement.

Projection/gesture tests cover flat/globe picking at 1×–512×, pointer-anchored easing, datelines, frame-rate independence, camera bounds, drag tracking, peak travel and modal cancellation. Native input checks feed queued ImGui keys/mouse events into the actual `App::controls`: D and arrows work despite generic navigation capture, typing and research suppress hidden map motion, map-owned drags cross docks, excursion/return cannot become a command/deployment, modals cancel gestures/zoom and destroyed selections cease offering orders.

Native rendering was inspected for the branch canvas, readable selected/hover inspector, construction catalog, queue/ready counts, main-city inspection, river/road overlays, options and placement ghost. The inspector avoids duplicate cards and places command actions above statistics. Additional low-resolution/high-scale smoke exposed clipping that was corrected before final captures. The smoke path creates a genuine EGL OpenGL context; it does not mock rendering or use generated concept images.

## Reproduce

```sh
cmake -S . -B build-core -DESCALATION_BUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
python3 tests/test_build_pipeline.py
python3 tools/verify.py --sanitize
```

Native validation:

```sh
cmake -S . -B build-native -DESCALATION_NULL_PLATFORM=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-native --parallel
ctest --test-dir build-native --output-on-failure
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 ./build-native/escalation --smoke build-native/smoke-720 1280 720 1.25
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 ./build-native/escalation --smoke build-native/smoke-1080 1920 1080 1.0
```

Sanitizers exclude deliberate exception/unwinding fixtures and leak enumeration because this restricted workspace cannot expose process/stack metadata. Ordinary core tests include malformed-save cases. Compiler temporary files are isolated in the verification directory.

These results validate deterministic flows and native presentation, not balanced competitive matches or physical-device usability on every desktop. Windows runtime, real rootless Podman image execution, multiple GPUs and long playtests were not performed. Road/rivers are generalized strategic vectors; rivers have no movement/combat effect. The bounded route planner can reject complex feasible detours. Only two nations receive detailed simulation, and complete reference-game catalogs/loadouts, detection, interception and diplomacy remain further work.

## 0.5.0 focused pass

One consolidated native build, tactical queue/save checks and native input integration passed. New cases cover queue append/replacement/completion, invalid-order preservation, stop, save restoration, control-group recall, the full date and automatic scale response. A single 1280×720 render pass at 125% scale covered the changed dock and map surfaces; clipped top-bar/dock content was corrected by increasing their layout heights. The major-release policy was checked against six eligible/ineligible tag/version/history cases; Windows and Linux hosted packaging will first run on a published, explicitly approved vX.0.0 release.

## 0.6.0 approved interface integration

Focused simulation, tactical and native-input suites passed. Coverage includes multi-level branches, hidden prerequisites, actual infantry damage/protection/medicine, retired air/naval production and transport, ready-unit inactivity and deployment, version-5 migration/version-6 inventory roundtrip, and overlay/page input interception. Native renders at 1440×900/100% and 1280×720/125% were reviewed. The compact header was increased to avoid clipping, capability effects wrap, and the research canvas keeps a readable text floor with panning. No broad sanitizer or platform rebuild sweep was repeated; hosted major-release packaging is unchanged.

## 0.6.1 layout correction

The existing native-input suite passed; native renders verify that the shared page gutter clears both navigation columns. Native rendering at 1440×900/100% and 1280×720/125% covers full-page fabrication, intact navigation buttons, borderless inventory tiles, and wrapped grid content. All fourteen documented screenshots are generated from this patch. No simulation or balance changes required a repeated core/combat sweep.

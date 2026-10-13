# Validation record

Validated on 14 October 2026 in a Linux x86-64 workspace with GCC 13, local CMake 4.4.4, GLFW 3.4, Dear ImGui 1.91.8 and Mesa software OpenGL. This records prototype checks, not desktop release qualification.

| Check | Result |
| --- | --- |
| CMake C++17 core build / CTest | Three suites passed |
| Core/world/production/research/save checks | 14,720 passed |
| Projection and camera motion | 6,198 passed |
| Tactical combat/navigation checks | 120 passed |
| ASan + UBSan valid-flow suites | 14,690 core, 6,198 camera and 120 tactical checks passed; 49,275-day headless run passed |
| All playable nation starts | 170 canonical campaigns; every objective lies on the correct in-country region |
| Native client build | Passed with GLFW null platform |
| Actual OpenGL smoke | Ten captured states; no GL errors |
| Render context | Mesa 25.2.8, OpenGL 4.5 core; shaders target OpenGL 3.3 |
| Build orchestration tests | Seven passed; package requires roads, settlements and geography manifest |
| Data regeneration | Geography derivatives reproduced byte-for-byte; source/output/world hashes verified |
| Road index | 5,498 sampled road midpoint queries preserve their originating road; dateline coverage checked |
| Campaign benchmark | 49,275 days; approximately 271,000 ticks/sec, one optimized two-nation run without active war |

Core checks cover real country picking/land partitions, flags, imported cities and grouping, all-region objective placement, regional allocation, finite production sharing and spare-work redistribution, cancellation, source/lab loss, prerequisites, year-free rush and frozen cost, save continuation, malformed snapshots, version-2 queue migration, geometric aircraft orders and strategic production/release consequences.

Tactical checks cover nonteleporting movement, exactly 15% road speed, separate support research speed, peace/unfinished/invalid order rejection, automatic simultaneous local damage, contested-town capture, partial regional occupation, all-town control, captured-base cancellation, type-specific weapons, defensive AI motion, pressure signs, ocean embark gates, naval land rejection, neutral-land detours and ended-campaign/history limits.

Camera checks cover flat/globe picking at 1× through 512×, pan convergence, frame-rate independence, pointer anchor preservation at datelines and positive/negative latitudes, zoom convergence, bounds and focus reset. Physical input behavior is reviewed in code; it still needs desktop playtesting.

Geography review checks pinned v5.1.2 source hashes, modern source-country aliases, deterministic reductions, repeated loading, malformed input rejection and the spatial index. Imported coordinates near simplified coastlines are adjusted in-country for playable objectives. Roads are strategic vectors, not a street-level routing database.

## Reproduce

```sh
cmake -S . -B build-core -DESCALATION_BUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
python3 tests/test_build_pipeline.py
```

Native surfaceless smoke:

```sh
cmake -S . -B build-native -DESCALATION_NULL_PLATFORM=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-native --parallel
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 ./build-native/escalation --smoke build-native/smoke
```

This creates a genuine EGL pbuffer OpenGL context; screenshots are from the application. No mocked renderer, browser or generated concept imagery is used.

The compiler-only path remains `python3 tools/verify.py`; optional `--sanitize` uses ASan/UBSan and excludes deliberate exception/unwinding fixtures in this restricted workspace. Leak enumeration is disabled because process/stack metadata is unavailable. These limitations do not affect the ordinary malformed-save suite.

The benchmark excludes loading, region generation, rendering and active tactical war. It does not establish full-world simultaneous simulation, target GPU frame rates or intended match duration. No Windows runtime, rootless Podman image execution or multi-GPU qualification was performed here. The bounded visibility-graph planner may reject complex feasible detours; generalized geometry and 0.08-degree route samples cannot resolve street-scale obstacles. Exact ICBM feature/balance parity is not established.

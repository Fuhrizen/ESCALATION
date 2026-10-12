# Validation record

Validated on 12 October 2026 in a Linux x86-64 workspace with GCC 13 and Mesa software OpenGL. This records the prototype's actual checks, rather than claiming desktop release qualification.

## Results

| Check | Result |
| --- | --- |
| Native C++17 core compilation | Passed |
| Core simulation / map checks | 857 passed |
| Camera projection / picking roundtrips | 631 passed |
| ASan + UBSan valid-flow checks | 853 core checks and 631 projection checks passed |
| Actual OpenGL shader/UI smoke run | Eight frames captured across menu, selection, globe, flat map, research, industry, supply and war; no GL errors |
| Render context | Mesa 25.2.8, OpenGL 4.5 core; shaders target OpenGL 3.3 |
| Cached world geometry | 91,464 land vertices; 177 map features |
| Campaign benchmark | 49,275 daily ticks (1920–2055), about 159,000 ticks/sec in the optimized two-nation core |
| Compose configuration and shell syntax | Parsed/checked successfully |
| Docker Linux compilation / CTest | Passed on the user’s host (both tests); output installation subsequently hit host bind-mount permissions |
| Docker Windows cross-build / Windows runtime | Not run: Docker and MinGW are not installed in this environment |
| CMake configure/build | Not run here: CMake is not installed; equivalent source/dependency targets were compiled directly with GCC |

The normal suite checks country centres on land, picking, shared land borders, clipped partition area conservation, research prerequisites/queues, construction without routes, recipe starvation, deterministic save restoration, malformed-save rejection, supplied conventional capture, amphibious landing gates and scripted MAD.

The optional `tools/verify.py --sanitize` path disables leak enumeration and excludes deliberate exception/unwinding tests because this restricted workspace cannot expose the process/stack metadata ASan requires. The ordinary suite includes malformed-save tests. Address/undefined checks cover successful simulation, persistence roundtrips, geometry and projection flows. This is not a complete leak or malformed-input sanitizer audit.

## Reproduce

```sh
python3 tools/verify.py
python3 tools/verify.py --sanitize
```

With a normal development toolchain:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/escalation --smoke build/smoke
```

The smoke path uses GLFW's null platform for metadata/input and a genuine EGL pbuffer OpenGL context. It requires Linux EGL and does not depend on a browser, mocked renderer or generated concept art. Screenshots in this repository were captured from this application.

The benchmark excludes map loading, sector generation and rendering. It does not establish full-world simultaneous simulation performance, GPU frame rates, tactical-unit throughput or the intended 2–4 hour match balance. Desktop interaction, multiple GPUs, Windows runtime and both Docker build services still need host validation.

## Host output export fix

The original `/out` host bind mount failed during installation on the user’s host. Both services now install to the `build-output` Docker volume. Build with `docker compose up --build --exit-code-from linux linux`, then export with `docker compose cp linux:/out/linux/. ./dist-linux` (or use the Windows service and `dist-windows`). This also works with a remote Docker daemon and avoids writing to host directories as the container user. The revised export configuration was statically checked here; Docker runtime validation still requires the host.

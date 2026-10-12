#!/bin/sh
set -eu
platform="${1:-linux}"
if [ ! -f /source/CMakeLists.txt ]; then
 printf 'Project source is missing from the builder image. Run docker compose up --build --exit-code-from %s %s from the repository root.\n' "$platform" "$platform" >&2
 exit 1
fi
case "$platform" in
 linux)
  cmake -S /source -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_BASE_DIR=/deps/linux
  cmake --build /build --parallel "${JOBS:-4}"
  ctest --test-dir /build --output-on-failure
  ;;
 windows)
  cmake -S /source -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/cmake/mingw-w64.cmake -DFETCHCONTENT_BASE_DIR=/deps/windows -DBUILD_TESTING=OFF
  cmake --build /build --parallel "${JOBS:-4}"
  ;;
 *) printf 'Unknown platform: %s\n' "$platform" >&2; exit 2 ;;
esac
cmake --install /build --prefix "/out/$platform"
printf 'Build installed in Docker volume at /out/%s. The host build script extracts it automatically.\n' "$platform"

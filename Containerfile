FROM docker.io/library/ubuntu:24.04 AS source
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates git cmake ninja-build build-essential \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt README.md THIRD_PARTY_NOTICES.md ./
COPY cmake ./cmake
COPY include ./include
COPY src ./src
COPY tests ./tests
COPY third_party ./third_party
COPY data ./data

FROM source AS linux-build
ARG JOBS=2
RUN apt-get update && apt-get install -y --no-install-recommends \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgl1-mesa-dev libegl1 libgl1-mesa-dri \
    && rm -rf /var/lib/apt/lists/*
RUN cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/artifacts -DBUILD_TESTING=ON \
    && cmake --build /build --parallel "$JOBS" \
    && ctest --test-dir /build --output-on-failure \
    && LIBGL_ALWAYS_SOFTWARE=1 /build/escalation --smoke /build/render-checks \
    && cmake --install /build \
    && test -x /artifacts/escalation \
    && test -f /artifacts/data/world.geojson \
    && mv /src/data /src/data.original \
    && /artifacts/escalation_headless FRA 0 \
    && mv /src/data.original /src/data

FROM source AS windows-build
ARG JOBS=2
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-mingw-w64-x86-64-posix g++-mingw-w64-x86-64-posix \
    && rm -rf /var/lib/apt/lists/*
RUN cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/src/cmake/mingw-w64.cmake \
    -DCMAKE_INSTALL_PREFIX=/artifacts -DBUILD_TESTING=OFF \
    && cmake --build /build --parallel "$JOBS" \
    && cmake --install /build \
    && test -f /artifacts/escalation.exe \
    && test -f /artifacts/data/world.geojson

FROM scratch AS linux-artifacts
COPY --from=linux-build /artifacts /artifacts
CMD ["/artifacts/escalation"]

FROM scratch AS windows-artifacts
COPY --from=windows-build /artifacts /artifacts
CMD ["/artifacts/escalation.exe"]

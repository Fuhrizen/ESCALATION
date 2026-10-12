FROM ubuntu:24.04 AS base
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates git cmake ninja-build build-essential && rm -rf /var/lib/apt/lists/*
COPY docker/build.sh /usr/local/bin/escalation-build
COPY . /source
WORKDIR /source
RUN chmod +x /usr/local/bin/escalation-build
ENTRYPOINT ["/usr/local/bin/escalation-build"]
FROM base AS linux-builder
RUN apt-get update && apt-get install -y --no-install-recommends libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev && rm -rf /var/lib/apt/lists/*
CMD ["linux"]
FROM base AS windows-builder
RUN apt-get update && apt-get install -y --no-install-recommends gcc-mingw-w64-x86-64-posix g++-mingw-w64-x86-64-posix && rm -rf /var/lib/apt/lists/*
CMD ["windows"]

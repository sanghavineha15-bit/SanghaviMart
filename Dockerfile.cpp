# SanghaviMart on Vercel Container Runtime (single service: API + static frontend).
# Vercel builds this image, pushes it to VCR, and serves it as a Function on $PORT.
# Drogon serves ./public (document_root) at / and the JSON API at /api/v1/*,
# so no rewrites are needed: one container, one domain.
#
# NOTE: the first build compiles Drogon + deps via vcpkg (slow, one-time cost).
# Rebuilds reuse cached layers and only recompile our src/ changes.
FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential git curl zip unzip tar pkg-config python3-pip \
      libsodium-dev libsqlite3-dev libcurl4-openssl-dev uuid-dev ca-certificates \
    && pip3 install --no-cache-dir --break-system-packages cmake ninja \
    && rm -rf /var/lib/apt/lists/*
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /vcpkg \
    && /vcpkg/bootstrap-vcpkg.sh
COPY vcpkg.json /src/vcpkg.json
COPY src/ /src/src/
COPY CMakeLists.txt /src/CMakeLists.txt
RUN cmake -S /src -B /build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DSANGHAVIMART_ENABLE_ASAN_UBSAN=OFF \
      -DSANGHAVIMART_BUILD_TESTS=OFF \
    && cmake --build /build

FROM debian:bookworm-slim AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
      libsodium23 libsqlite3-0 libcurl4 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /build/sanghavimart /opt/sanghavimart/bin/sanghavimart
COPY public/ /opt/sanghavimart/public/
COPY db/ /opt/sanghavimart/db/
COPY config.postgres.json /opt/sanghavimart/config.json
WORKDIR /opt/sanghavimart
# Vercel routes to port 80 by default, or $PORT when set. main.cc honors $PORT.
EXPOSE 80
CMD ["/opt/sanghavimart/bin/sanghavimart"]

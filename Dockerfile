# Multi-stage build: toolchain image compiles the binary; runtime is minimal.
FROM ubuntu:22.04 AS build
RUN apt-get update && apt-get install -y build-essential cmake ninja-build git curl zip unzip tar pkg-config libsodium-dev libsqlite3-dev libcurl4-openssl-dev uuid-dev
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg && /vcpkg/bootstrap-vcpkg.sh
COPY . /src
RUN cmake -S /src -B /build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DSANGHAVIMART_ENABLE_ASAN_UBSAN=OFF && cmake --build /build
FROM debian:bookworm-slim AS runtime
RUN apt-get update && apt-get install -y libsodium23 libsqlite3-0 libcurl4 ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=build /build/sanghavimart /opt/sanghavimart/bin/sanghavimart
COPY --from=build /src/public /opt/sanghavimart/public
COPY --from=build /src/db /opt/sanghavimart/db
COPY --from=build /src/config.json /opt/sanghavimart/config.json
WORKDIR /opt/sanghavimart
EXPOSE 8080
CMD ["/opt/sanghavimart/bin/sanghavimart"]

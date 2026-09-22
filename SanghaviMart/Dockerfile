# syntax=docker/dockerfile:1
FROM mcr.microsoft.com/vcpkg:latest AS builder
WORKDIR /src
COPY SanghaviMart/ ./SanghaviMart/
RUN ./vcpkg/bootstrap-vcpkg.sh && \
    cmake -S SanghaviMart -B /build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=/src/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build /build -j

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y libpq5 libsodium23 libcurl4 openssl && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /build/SanghaviMart ./SanghaviMart
COPY --from=builder /src/SanghaviMart/frontend ./frontend
COPY --from=builder /src/SanghaviMart/static ./static
COPY --from=builder /src/SanghaviMart/db ./db
COPY --from=builder /src/SanghaviMart/config.json ./config.json
EXPOSE 8080
CMD ["./SanghaviMart"]

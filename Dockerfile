FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN cmake -S . -B build-dds -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DJ1939_DDS_ENABLE_CYCLONEDDS=ON \
        -DJ1939_DDS_FETCH_CYCLONEDDS=ON \
    && cmake --build build-dds --parallel \
    && ctest --test-dir build-dds --output-on-failure

ENV LD_LIBRARY_PATH=/workspace/build-dds/lib

ENTRYPOINT ["./build-dds/j1939_dds_gateway"]
CMD ["--replay", "samples/j1939_frames.csv", "--publisher", "stdout"]

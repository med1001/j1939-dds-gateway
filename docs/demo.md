# Demo guide

## 1. Replay without DDS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/j1939_dds_gateway --replay samples/j1939_frames.csv
```

Expected final state:

- engine speed: 2000 rpm;
- vehicle speed: 128 km/h;
- coolant temperature: 90 degrees C.

## 2. DDS publisher/subscriber

Build with Cyclone DDS:

```bash
cmake -S . -B build-dds \
  -DJ1939_DDS_ENABLE_CYCLONEDDS=ON \
  -DJ1939_DDS_FETCH_CYCLONEDDS=ON
cmake --build build-dds --parallel
```

Terminal A:

```bash
./build-dds/telemetry_subscriber --domain 0
```

Terminal B:

```bash
./build-dds/j1939_dds_gateway \
  --replay samples/j1939_frames.csv \
  --publisher dds \
  --domain 0 \
  --source-id airport-tractor-01
```

## 3. Native SocketCAN/J1939

Linux is required for the native `CAN_J1939` sockets.

Some default WSL kernels do not include the `vcan` module. In that case, use a
native Linux host, a compatible custom WSL kernel, or the CSV replay mode.

```bash
./scripts/setup_vcan.sh vcan0
./build/j1939_dds_gateway --interface vcan0
```

In a second terminal:

```bash
./build/j1939_demo_sender vcan0
```

The sender transmits EEC1, CCVS1 and ET1 payloads from source address `0x80`.
Stop the long-running gateway with Ctrl+C.

## Troubleshooting

- `Unknown CAN interface`: run `ip link show` and create `vcan0`.
- `Unknown device type` or `Module vcan not found` under WSL: use native Linux
  or the hardware-free replay mode.
- `Operation not permitted`: the interface setup requires `sudo` or the
  relevant container capabilities.
- `DDS support is not enabled`: rebuild with
  `J1939_DDS_ENABLE_CYCLONEDDS=ON`.
- DDS processes do not discover each other: verify the domain id, multicast,
  firewall and `CYCLONEDDS_URI` configuration.

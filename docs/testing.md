# Testing strategy

## Unit tests

`test_decoder` verifies:

- EEC1 engine-speed scaling;
- CCVS1 wheel-speed scaling;
- ET1 coolant-temperature offset;
- J1939 not-available encodings;
- payload-length validation;
- unsupported PGNs and invalid arguments.

## Pipeline test

`test_replay_pipeline` parses a CSV fixture, forwards every message through the
same callback used by the gateway, and validates the final cumulative state.
It also includes an unsupported PGN to verify that the pipeline continues.

## Process-level integration tests

`scripts/test_dds_roundtrip.sh` launches a DDS subscriber, publishes the sample
trace from a separate gateway process, and verifies the received topic data.

`scripts/test_socketcan_roundtrip.sh` creates a Linux `vcan` interface, launches
the live gateway, sends EEC1, CCVS1 and ET1 through native `CAN_J1939` sockets,
and verifies the three decoded JSON records. The SocketCAN test needs `sudo` to
create the virtual interface.

## Sanitizers

On GCC and Clang, configure with:

```bash
-DJ1939_DDS_ENABLE_SANITIZERS=ON
```

This enables AddressSanitizer and UndefinedBehaviorSanitizer for project
targets. Compiler warnings are errors by default.

## CI

GitHub Actions runs:

- the core build and CTest suite on Ubuntu and Windows;
- an ASan/UBSan build on Ubuntu;
- a complete build that fetches Cyclone DDS and compiles the DDS publisher,
  subscriber and generated IDL types;
- a process-level DDS round trip that publishes the replay trace and verifies
  samples received by a separate subscriber.

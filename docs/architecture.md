# Architecture

## Design goals

The gateway demonstrates embedded-Linux integration skills without coupling
the domain logic to a specific CAN adapter or DDS runtime. Its boundaries are
small C interfaces that can be replaced by test doubles or target-specific
implementations.

## Data path

1. `replay_source.c` parses deterministic CSV records, or
   `linux_j1939_source.c` receives payloads through `CAN_J1939` sockets.
2. `decoder.c` validates the payload length and updates a cumulative
   `vehicle_telemetry_t` state for supported PGNs.
3. `stdout_publisher.c` provides a dependency-free JSON sink.
4. `cyclone_publisher.c` maps the same state to the IDL-generated DDS type and
   publishes it on the `VehicleTelemetry` topic.
5. `telemetry_subscriber.c` demonstrates interoperability through a separate
   DDS process.

## Interfaces

`j1939_message_callback_t` decouples message sources from processing. A source
does not know which PGNs are supported or where decoded data is published.

`telemetry_publisher_t` is a minimal function-table interface. The gateway can
select JSON or DDS at runtime when DDS support is compiled in.

## DDS model

`idl/VehicleTelemetry.idl` defines a final type containing:

- source identity, timestamp, last PGN and J1939 source address;
- validity flag and value for engine speed;
- validity flag and value for vehicle speed;
- validity flag and value for coolant temperature.

The publisher and subscriber use reliable, transient-local QoS with a
keep-last depth of 10. This makes the small demo resilient to brief scheduling
delays and lets a late subscriber receive retained state.

## Extension points

- Add a decoder case and tests for each new PGN/SPN.
- Add a configuration layer for PGN filtering and topic selection.
- Replace the cumulative state with keyed per-ECU instances.
- Add persistence, observability, authentication or DDS Security.
- Add cross-compilation toolchains for a specific ARM/Linux target.

## Production considerations

This repository is an engineering demonstrator, not a certified vehicle
component. A production implementation must address ECU address claiming,
transport-protocol sessions, timing, data freshness, endian rules, licensed
SAE signal definitions, error recovery, cybersecurity, resource limits and the
applicable safety lifecycle.

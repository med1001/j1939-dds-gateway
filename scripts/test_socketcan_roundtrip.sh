#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build}"
interface_name="${2:-vcan0}"
log_dir="${TMPDIR:-/tmp}/j1939-socketcan-roundtrip-$$"
gateway_log="${log_dir}/gateway.log"
gateway_pid=""

cleanup() {
    if [[ -n "${gateway_pid}" ]] && kill -0 "${gateway_pid}" 2>/dev/null; then
        kill -INT "${gateway_pid}" 2>/dev/null || true
        wait "${gateway_pid}" 2>/dev/null || true
    fi
    rm -rf "${log_dir}"
}
trap cleanup EXIT

mkdir -p "${log_dir}"
./scripts/setup_vcan.sh "${interface_name}" >/dev/null

"${build_dir}/j1939_dds_gateway" \
    --interface "${interface_name}" \
    --publisher stdout \
    --source-id socketcan-test >"${gateway_log}" 2>&1 &
gateway_pid=$!
sleep 0.5

"${build_dir}/j1939_demo_sender" "${interface_name}"

for _ in $(seq 1 30); do
    if [[ $(grep -c '"source_id":"socketcan-test"' "${gateway_log}" || true) -ge 3 ]]; then
        break
    fi
    sleep 0.1
done

kill -INT "${gateway_pid}" 2>/dev/null || true
wait "${gateway_pid}" 2>/dev/null || true
gateway_pid=""

cat "${gateway_log}"
grep -q '"pgn":61444' "${gateway_log}"
grep -q '"pgn":65265' "${gateway_log}"
grep -q '"pgn":65262' "${gateway_log}"
grep -q "decoded=3 unsupported=0 invalid=0" "${gateway_log}"

echo "SocketCAN J1939 round-trip integration test passed"

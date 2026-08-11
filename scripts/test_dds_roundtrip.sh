#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build-dds}"
log_dir="${TMPDIR:-/tmp}/j1939-dds-roundtrip-$$"
subscriber_log="${log_dir}/subscriber.log"
gateway_log="${log_dir}/gateway.log"
subscriber_pid=""

cleanup() {
    if [[ -n "${subscriber_pid}" ]] && kill -0 "${subscriber_pid}" 2>/dev/null; then
        kill -INT "${subscriber_pid}" 2>/dev/null || true
        wait "${subscriber_pid}" 2>/dev/null || true
    fi
    rm -rf "${log_dir}"
}
trap cleanup EXIT

mkdir -p "${log_dir}"
export LD_LIBRARY_PATH="${build_dir}/lib:${LD_LIBRARY_PATH:-}"

"${build_dir}/telemetry_subscriber" --domain 7 >"${subscriber_log}" 2>&1 &
subscriber_pid=$!
sleep 1

"${build_dir}/j1939_dds_gateway" \
    --replay samples/j1939_frames.csv \
    --publisher dds \
    --domain 7 \
    --source-id integration-test >"${gateway_log}" 2>&1

for _ in $(seq 1 30); do
    if grep -q "source=integration-test" "${subscriber_log}"; then
        break
    fi
    sleep 0.1
done

kill -INT "${subscriber_pid}" 2>/dev/null || true
wait "${subscriber_pid}" 2>/dev/null || true
subscriber_pid=""

cat "${gateway_log}"
cat "${subscriber_log}"

grep -q "decoded=6 unsupported=0 invalid=0" "${gateway_log}"
grep -q "source=integration-test" "${subscriber_log}"
grep -q "pgn=65262" "${subscriber_log}"

echo "DDS round-trip integration test passed"

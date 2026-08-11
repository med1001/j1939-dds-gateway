#!/usr/bin/env bash
set -euo pipefail

interface_name="${1:-vcan0}"

sudo modprobe vcan 2>/dev/null || true
if ! ip link show "${interface_name}" >/dev/null 2>&1; then
    if ! sudo ip link add dev "${interface_name}" type vcan; then
        echo "Cannot create ${interface_name}; this kernel may not include vcan support." >&2
        exit 1
    fi
fi
sudo ip link set "${interface_name}" up
ip -details link show "${interface_name}"

#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 ISO_IMAGE EXPECTED_OUTPUT TIMEOUT_SECONDS" >&2
    exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
image=$1
expected_output=$2
timeout_seconds=$3

if [[ ! "$timeout_seconds" =~ ^[1-9][0-9]*$ ]]; then
    echo "Invalid QEMU test timeout: $timeout_seconds" >&2
    exit 2
fi
log=$(mktemp)
qemu_pid=
cleanup() {
    if [[ -n "$qemu_pid" ]] && kill -0 "$qemu_pid" 2>/dev/null; then
        kill "$qemu_pid" 2>/dev/null || true
    fi
    if [[ -n "$qemu_pid" ]]; then
        wait "$qemu_pid" 2>/dev/null || true
    fi
    rm -f "$log"
}
trap cleanup EXIT

"$script_dir/run-qemu.sh" "$image" >"$log" 2>&1 &
qemu_pid=$!

for ((tick = 0; tick < timeout_seconds * 10; tick++)); do
    if grep -Fq -- "$expected_output" "$log"; then
        echo "QEMU JVM test passed: $expected_output"
        exit 0
    fi
    if ! kill -0 "$qemu_pid" 2>/dev/null; then
        status=0
        wait "$qemu_pid" || status=$?
        cat "$log" >&2
        echo "QEMU exited before the expected output (status $status)" >&2
        exit 1
    fi
    sleep 0.1
done

cat "$log" >&2
echo "Expected JVM output not found within ${timeout_seconds}s: $expected_output" >&2
exit 1

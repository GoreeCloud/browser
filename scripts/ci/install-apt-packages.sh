#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -eq 0 ]]; then
  echo "usage: install-apt-packages.sh <package> [package ...]" >&2
  exit 64
fi

readonly ATTEMPTS=2
readonly COMMAND_TIMEOUT_SECONDS=240
readonly RETRY_DELAY_SECONDS=5

run_apt() {
  local operation="$1"
  shift
  local attempt status

  for attempt in $(seq 1 "$ATTEMPTS"); do
    echo "GoreeCloud CI apt \${operation}: attempt \${attempt}/\${ATTEMPTS}" >&2
    if sudo timeout --signal=TERM --kill-after=15s "\${COMMAND_TIMEOUT_SECONDS}s" \
      env DEBIAN_FRONTEND=noninteractive \
      apt-get \
        -o Dpkg::Use-Pty=0 \
        -o DPkg::Lock::Timeout=60 \
        -o Acquire::Retries=3 \
        -o Acquire::http::Timeout=20 \
        -o Acquire::https::Timeout=20 \
        "$@"; then
      return 0
    else
      status=$?
    fi

    echo "GoreeCloud CI apt \${operation}: failed with status \${status}" >&2
    if [[ "$attempt" -ge "$ATTEMPTS" ]]; then
      return "$status"
    fi

    # A timed-out package operation can leave dpkg in an interrupted but
    # recoverable state on the disposable CI runner. Bound that repair too;
    # the next apt attempt remains authoritative and must succeed.
    sudo timeout --signal=TERM --kill-after=10s 90s \
      env DEBIAN_FRONTEND=noninteractive dpkg --configure -a || true
    sleep "$RETRY_DELAY_SECONDS"
  done

  return 1
}

run_apt update update
run_apt install install -y "$@"

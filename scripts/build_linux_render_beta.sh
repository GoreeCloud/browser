#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cache_dir="${GOREECLOUD_CEF_CACHE_DIR:-$repo_root/.cache/cef}"
build_dir="${GOREECLOUD_BROWSER_CEF_BUILD_DIR:-$repo_root/build-cef-render}"

for command in python3 cmake ninja; do
  if ! command -v "$command" >/dev/null 2>&1; then
    echo "Missing required command: $command" >&2
    exit 1
  fi
done

cef_root="$(
  python3 "$repo_root/scripts/bootstrap_cef_linux.py" \
    --cache-dir "$cache_dir" \
    --print-root
)"

echo "Using pinned CEF root: $cef_root"
echo "Configuring GoreeCloud Browser render-capable Development build..."

cmake -S "$repo_root" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DGOREECLOUD_ENABLE_CHROMIUM=ON \
  -DGOREECLOUD_ENABLE_CEF=ON \
  -DGOREECLOUD_CEF_ROOT="$cef_root" \
  -DGOREECLOUD_ENABLE_LINUX_GTK_HOST=ON \
  -DGOREECLOUD_ENABLE_CURL_DOWNLOAD_TRANSPORT=ON

cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure

echo
echo "Render-capable Development build completed."
echo "Run it with:"
echo "  $repo_root/scripts/run_linux_render_beta.sh"

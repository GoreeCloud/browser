#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${GOREECLOUD_BROWSER_CEF_BUILD_DIR:-$repo_root/build-cef-render}"
browser="$build_dir/goreecloud-browser"

# CEF's Linux target helpers place the dedicated subprocess executable in the
# configuration output directory. Keep a root-level fallback for compatible
# single-config layouts so the launcher remains portable across generators.
subprocess="$build_dir/Release/goreecloud-browser-subprocess"
if [[ ! -x "$subprocess" && -x "$build_dir/goreecloud-browser-subprocess" ]]; then
  subprocess="$build_dir/goreecloud-browser-subprocess"
fi

if [[ ! -x "$browser" ]]; then
  echo "Render-capable Browser build not found: $browser" >&2
  echo "Run scripts/build_linux_render_beta.sh first." >&2
  exit 1
fi

if [[ ! -x "$subprocess" ]]; then
  echo "CEF subprocess executable not found in the expected build outputs." >&2
  echo "Checked: $build_dir/Release/goreecloud-browser-subprocess" >&2
  echo "Checked: $build_dir/goreecloud-browser-subprocess" >&2
  echo "Run scripts/build_linux_render_beta.sh first." >&2
  exit 1
fi

for required in libcef.so icudtl.dat; do
  if [[ ! -f "$build_dir/$required" ]]; then
    echo "CEF runtime file missing from build output: $required" >&2
    exit 1
  fi
done

if [[ ! -d "$build_dir/locales" ]]; then
  echo "CEF locales directory missing from build output: $build_dir/locales" >&2
  exit 1
fi

if [[ -f "$build_dir/chrome-sandbox" && ! -u "$build_dir/chrome-sandbox" ]]; then
  cat >&2 <<EOF
CEF sandbox note: $build_dir/chrome-sandbox is not setuid.
GoreeCloud Browser will not disable the Chromium sandbox to bypass this.
If CEF reports a Linux sandbox initialization failure, configure the helper
according to the official CEF Linux sandbox instructions, then retry.
EOF
fi

if [[ -f "$build_dir/chrome-sandbox" ]]; then
  export CHROME_DEVEL_SANDBOX="$build_dir/chrome-sandbox"
fi

export GOREECLOUD_BROWSER_RUNTIME_ROOT="$build_dir"
# Use the dedicated subprocess executable for renderer, GPU, utility, and
# other CEF child processes. Re-entering the full Browser executable adds an
# unnecessary host stack frame around CefExecuteProcess; Chromium child
# processes may rotate the stack guard after fork, so returning through that
# pre-fork frame can trip stack-protector checks.
export GOREECLOUD_BROWSER_SUBPROCESS="$subprocess"
export GOREECLOUD_BROWSER_RESOURCES="$build_dir"
export GOREECLOUD_BROWSER_LOCALES="$build_dir/locales"
export LD_LIBRARY_PATH="$build_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

if [[ "$#" -gt 0 && "$1" != --* ]]; then
  export GOREECLOUD_BROWSER_INITIAL_URL="$1"
  shift
fi

# CEF resolves Linux runtime data relative to the loaded runtime/executable
# environment. Run from the verified payload directory so libcef.so,
# icudtl.dat, pak files, snapshots, locales, and the subprocess executable
# share one canonical runtime location.
cd "$build_dir"
exec "$browser" "$@"

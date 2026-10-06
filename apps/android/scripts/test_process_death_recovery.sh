#!/usr/bin/env bash
set -euo pipefail

PACKAGE="io.goreecloud.browser.beta"
ACTIVITY="\${PACKAGE}/io.goreecloud.browser.BrowserActivityV2"
APK="apps/android/app/build/outputs/apk/debug/app-debug.apk"
FIRST_URL="http://127.0.0.1:1/goreecloud-process-first"
SECOND_URL="http://127.0.0.1:1/goreecloud-process-second"
UI_XML="\${RUNNER_TEMP:-/tmp}/goreecloud-browser-process-death-ui.xml"
PREFS_XML="\${RUNNER_TEMP:-/tmp}/goreecloud-browser-first-use.xml"

fail() {
  printf 'process-death smoke: %s\n' "$*" >&2
  exit 1
}

dump_ui() {
  adb shell uiautomator dump /sdcard/goreecloud-window.xml >/dev/null 2>&1 || return 1
  adb exec-out cat /sdcard/goreecloud-window.xml >"$UI_XML"
  test -s "$UI_XML"
}

wait_for_desc() {
  local expected="$1"
  for _ in $(seq 1 80); do
    if dump_ui && python3 - "$UI_XML" "$expected" <<'PY'
import sys
import xml.etree.ElementTree as ET

path, expected = sys.argv[1], sys.argv[2]
root = ET.parse(path).getroot()
if any(node.attrib.get("content-desc") == expected for node in root.iter("node")):
    raise SystemExit(0)
raise SystemExit(1)
PY
    then
      return 0
    fi
    sleep 0.25
  done
  return 1
}

tap_desc() {
  local expected="$1"
  dump_ui || fail "could not dump UI before tapping \${expected}"
  local point
  point="$(python3 - "$UI_XML" "$expected" <<'PY'
import re
import sys
import xml.etree.ElementTree as ET

path, expected = sys.argv[1], sys.argv[2]
root = ET.parse(path).getroot()
for node in root.iter("node"):
    if node.attrib.get("content-desc") != expected:
        continue
    bounds = node.attrib.get("bounds", "")
    match = re.fullmatch(r"\[(\d+),(\d+)\]\[(\d+),(\d+)\]", bounds)
    if not match:
        break
    x1, y1, x2, y2 = map(int, match.groups())
    print((x1 + x2) // 2, (y1 + y2) // 2)
    raise SystemExit(0)
raise SystemExit(1)
PY
)" || fail "could not resolve bounds for \${expected}"
  adb shell input tap $point
}

wait_for_address() {
  local expected="$1"
  for _ in $(seq 1 100); do
    if dump_ui && python3 - "$UI_XML" "$expected" <<'PY'
import sys
import xml.etree.ElementTree as ET

path, expected = sys.argv[1], sys.argv[2]
root = ET.parse(path).getroot()
for node in root.iter("node"):
    if node.attrib.get("content-desc") == "Search or address bar":
        if node.attrib.get("text") == expected:
            raise SystemExit(0)
raise SystemExit(1)
PY
    then
      return 0
    fi
    sleep 0.25
  done
  return 1
}

assert_restored_tabs() {
  dump_ui || fail "could not dump restored UI"
  python3 - "$UI_XML" <<'PY'
import sys
import xml.etree.ElementTree as ET

root = ET.parse(sys.argv[1]).getroot()
tabs = [
    node.attrib.get("content-desc", "")
    for node in root.iter("node")
    if node.attrib.get("content-desc", "").startswith("Tab ")
]
if len(tabs) != 2:
    raise SystemExit(f"expected 2 restored tabs, saw {len(tabs)}: {tabs}")
if not tabs[0].startswith("Tab 1:"):
    raise SystemExit(f"first restored tab order is wrong: {tabs}")
if not tabs[1].startswith("Tab 2:"):
    raise SystemExit(f"second restored tab order is wrong: {tabs}")
if not tabs[1].endswith(", selected"):
    raise SystemExit(f"second restored tab is not selected: {tabs}")
PY
}

test -s "$APK" || fail "debug APK is missing"
adb install -r "$APK" >/dev/null

cat >"$PREFS_XML" <<'EOF'
<?xml version='1.0' encoding='utf-8' standalone='yes' ?>
<map>
    <boolean name="completed" value="true" />
    <boolean name="hints_enabled" value="false" />
</map>
EOF
adb push "$PREFS_XML" /data/local/tmp/goreecloud-browser-first-use.xml >/dev/null
adb shell "run-as \${PACKAGE} mkdir -p shared_prefs"
adb shell "run-as \${PACKAGE} cp /data/local/tmp/goreecloud-browser-first-use.xml shared_prefs/goreecloud-browser-first-use.xml"
adb shell "run-as \${PACKAGE} rm -rf no_backup/normal-session-v1"
adb shell rm -f /data/local/tmp/goreecloud-browser-first-use.xml

adb shell am force-stop "$PACKAGE"
adb shell am start -W -a android.intent.action.VIEW -d "$FIRST_URL" -n "$ACTIVITY" >/dev/null
wait_for_desc "New tab" || fail "Browser did not expose New tab after first launch"
pid_before="$(adb shell pidof "$PACKAGE" | tr -d '\r')"
test -n "$pid_before" || fail "could not resolve Browser PID before process death"

tap_desc "New tab"
sleep 0.5
adb shell am start -W -a android.intent.action.VIEW -d "$SECOND_URL" -n "$ACTIVITY" >/dev/null
wait_for_address "127.0.0.1:1/goreecloud-process-second" ||
  fail "second tab did not reach the expected retry identity"

adb shell am force-stop "$PACKAGE"
for _ in $(seq 1 40); do
  if ! adb shell pidof "$PACKAGE" | grep -q '[0-9]'; then
    break
  fi
  sleep 0.25
done
if adb shell pidof "$PACKAGE" | grep -q '[0-9]'; then
  fail "Browser process remained alive after force-stop"
fi

adb shell am start -W -n "$ACTIVITY" >/dev/null
wait_for_desc "New tab" || fail "Browser did not relaunch after process death"
pid_after="$(adb shell pidof "$PACKAGE" | tr -d '\r')"
test -n "$pid_after" || fail "could not resolve Browser PID after process death"
test "$pid_before" != "$pid_after" ||
  fail "Browser PID did not change across the process-death boundary"

assert_restored_tabs
wait_for_address "127.0.0.1:1/goreecloud-process-second" ||
  fail "selected recovered tab lost its safe retry URL"

printf 'process-death smoke: restored two Normal tabs across PID %s -> %s\n' \
  "$pid_before" "$pid_after"

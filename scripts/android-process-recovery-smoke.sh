#!/usr/bin/env bash
set -euo pipefail

: "${RUNNER_TEMP:?RUNNER_TEMP is required}"

gradle -p apps/android --no-daemon connectedDebugAndroidTest --stacktrace

# connectedDebugAndroidTest may clean or relocate install inputs. Re-assemble the
# exact debug app and test artifacts before the process-boundary proof.
gradle -p apps/android --no-daemon assembleDebug assembleDebugAndroidTest --stacktrace

app_apk="$(find apps/android/app/build/outputs/apk/debug -type f -name '*.apk' -print -quit)"
test_apk="$(find apps/android/app/build/outputs/apk/androidTest/debug -type f -name '*-androidTest.apk' -print -quit)"

if [[ -z "$app_apk" || ! -s "$app_apk" ]]; then
  echo "Android recovery smoke: debug app APK is missing or empty" >&2
  exit 1
fi
if [[ -z "$test_apk" || ! -s "$test_apk" ]]; then
  echo "Android recovery smoke: debug test APK is missing or empty" >&2
  exit 1
fi

adb install -r "$app_apk" >/dev/null
adb install -r "$test_apk" >/dev/null

instrumentation="$(
  adb shell pm list instrumentation |
    sed -n 's/^instrumentation:\([^ ]*\) (target=io.goreecloud.browser.beta)$/\1/p' |
    head -n 1 |
    tr -d '\r'
)"
if [[ -z "$instrumentation" ]]; then
  echo "Android recovery smoke: Browser test instrumentation is unavailable" >&2
  adb shell pm list instrumentation >&2 || true
  exit 1
fi

adb shell am instrument -w \
  -e class io.goreecloud.browser.BrowserProcessRecoveryShellSeedTest \
  -e goreecloud.recovery.seed 1 \
  "$instrumentation"

# Cross the real application-process boundary after durable state has been
# seeded through the JNI bridge, then relaunch the Browser activity.
adb shell am force-stop io.goreecloud.browser.beta
adb shell am start -W \
  -n io.goreecloud.browser.beta/io.goreecloud.browser.BrowserActivityV2

recovery_xml="$RUNNER_TEMP/goreecloud-recovery.xml"
recovered=0
for attempt in $(seq 1 20); do
  if adb shell uiautomator dump /sdcard/goreecloud-recovery.xml >/dev/null 2>&1 &&
     adb shell cat /sdcard/goreecloud-recovery.xml > "$recovery_xml"; then
    if grep -F 'text="example.com/goreecloud-shell-recovery"' "$recovery_xml" >/dev/null &&
       grep -E 'content-desc="Tab 2: [^"]*, selected"' "$recovery_xml" >/dev/null; then
      recovered=1
      break
    fi
  fi
  sleep 0.5
done

if [[ "$recovered" -ne 1 ]]; then
  echo "Android recovery smoke: durable Tab 2 selection was not observed after relaunch" >&2
  if [[ -s "$recovery_xml" ]]; then
    cat "$recovery_xml" >&2
  fi
  exit 1
fi

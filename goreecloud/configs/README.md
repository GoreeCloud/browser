# Gecko development configuration — non-distributable

Two deliberately opt-in Linux/GTK development configurations are retained:

- `mozconfig-linux-dev`: intended full-source compilation with GoreeCloud branding; `./mach configure` reached ALSA development checks after a private LLVM 19 bootstrap but failed because `alsa.pc` was unavailable.
- `mozconfig-linux-artifact-dev`: Mozilla prebuilt Gecko artifact path for frontend, strings and branding experiments. `./mach configure` passed after fixing localization preprocessing; `./mach build` returned no matching artifact for the signed source snapshot's available pushheads.

From the source directory, use `MOZCONFIG=goreecloud/configs/mozconfig-linux-artifact-dev ./mach configure` or the analogous full-source config, with a compatible isolated toolchain and `MOZBUILD_STATE_PATH` set to an authorized development directory.

Neither config is approved for distribution. Both disable the upstream updater because GoreeCloud's own signed update and rollback implementation has not been accepted. These development files must not be silently promoted to production. See `goreecloud/BUILD-VALIDATION-LINUX.md` and `goreecloud/RELEASE-GATES.md`.

Run `python3 goreecloud/tools/generate_linux_branding.py --check` for deterministic asset integrity and `python3 -m unittest discover -s goreecloud/tests -v` for source-level checks.

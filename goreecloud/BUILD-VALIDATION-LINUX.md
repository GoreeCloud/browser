# Linux/GTK Gecko build evidence — October 13, 2026

Scope: GoreeCloud Firefox 158.0 source candidate, nonproduction. This record describes observable configure/build outputs, not release qualification.

## Reproducible source checks

- Generated GTK brand assets: 32 byte-for-byte matched files.
- Python unit and guardrail tests: 16 passed, including exact localization-manifest preprocessor regression check.
- Source-level baseline, licensing-file presence and fail-closed integration status validated.

## Full-source configure

- Original configure with development mozconfig found GoreeCloud's `--with-branding` directory but failed at absent `llvm-objdump`.
- Debian LLVM 19 was extracted into a private project staging directory, not installed system-wide; `llvm-objdump --version` passed.
- Retried full-source configure progressed past LLVM checks but stopped because the VPS lacked `alsa.pc` development metadata.
- Mozilla's Linux build documentation requires at least 30 GB free. This VPS had approximately 16 GB free; full compilation was not attempted.

## Artifact-mode configure

- Added explicit development config `goreecloud/configs/mozconfig-linux-artifact-dev` with `--enable-artifact-builds`, custom branding and `--disable-updater`.
- Extracted Debian Node.js 20 and needed libraries into a separate private staging directory. Both this tool and LLVM 19 were supplied to the process through a local PATH/library search path.
- First artifact configure reached Firefox's resource-packaging backend but failed on the GoreeCloud localization jar manifest: the generated file lacked `#filter substitution` before `@AB_CD@` directives.
- Corrected the generator, regenerated resources, and reran. **Artifact-mode `./mach configure` exited 0**. Firefox read 2,535 `moz.build` files, generated 4,259 backend files and reported `Configure complete!`.
- Local regeneration and 16 unit tests passed after the correction.

## Artifact build attempt

- Executed `./mach build` using the isolated artifact-mode mozconfig and developer toolchain.
- **Result: failure (exit 2)**. Firefox reported `Tried 4 pushheads, no built artifacts found`; its artifact installation stage returned an error.
- The signed source snapshot is not proof that a compatible, matching Mozilla-central artifact is available in Taskcluster. Resolve this upstream-artifact mapping and validate binary/source compatibility before continuing.
- No browser executable, runtime branding acceptance, network privacy trace, compiled Gecko, package or installer was produced or accepted by this attempt.

## Verified limits and next evidence

The source checkout is a verified Firefox 158.0 stable snapshot, and the UI resource configuration is valid in artifact mode. Artifact mode reuses Mozilla-produced compiled libraries and **cannot establish a full independent Gecko compilation**. Firefox source-level and UI-local changes also need runtime testing in a supported device environment.

Next: use a separately isolated build-capable environment with adequate space for a full source build; alternatively obtain immutable, source-compatible signed Mozilla artifacts to validate UI resources in artifact mode. Verify compiled branding, extension compatibility, privacy/network endpoint behavior, accessibility, signing, security updates, CEF profile migration and rollback.

**Release: blocked.** The checked-in development mozconfigs disable the updater and must never be used for distributing browser binaries.

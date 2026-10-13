# Gecko branding — development candidate, not accepted final identity

The build-selectable candidate is generated from `goreecloud/tools/generate_linux_branding.py` into `browser/branding/goreecloud/`. The `goreecloud/configs/` Linux mozconfigs opt into it without modifying Mozilla's original brand resources.

The earlier standalone `goreecloud/branding/locales/` resources are references only; they are not automatically consumed by Firefox's runtime.

Artifact-mode `./mach configure` has been **observed to pass** using the generated GTK brand resources. The localization manifest required an explicit preprocessing fix; a dedicated regression test now prevents removing the directive without a failing check.

The candidate uses original geometric placeholder artwork. It is not the approved final GoreeCloud identity; no Windows/macOS packaging, Glaze consumer acceptance, compiled application, privacy/security runtime acceptance or distribution evidence exists yet. Keep all mandatory Mozilla/third-party attribution.

See `goreecloud/BUILD-VALIDATION-LINUX.md` for the actual configuration/build results and the missing binary artifact blocker.

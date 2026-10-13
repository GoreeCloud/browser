# Gecko development configuration

Use the isolated GTK/Linux candidate only:

`MOZCONFIG=goreecloud/configs/mozconfig-linux-dev ./mach configure`

This source-level opt-in selects the GoreeCloud branding directory rather than modifying Mozilla's official branding. It also disables the Firefox updater **for local development only**, because no GoreeCloud binary-update service has been verified.

**Do not distribute binaries built with this configuration.** Distribution requires accepted security updates, signing, branding review, privacy and security tests, runtime validation, and real-device regression evidence. Other platforms remain unconfigured.

The branding generator is deterministic: run `python3 goreecloud/tools/generate_linux_branding.py --check` to validate committed artwork and resources.

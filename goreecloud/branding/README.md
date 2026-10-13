# GoreeCloud Gecko product identity — development boundary

The build-selectable candidate is generated from `goreecloud/tools/generate_linux_branding.py` into `browser/branding/goreecloud/`. This is a separate branding directory, selected only by the Linux development mozconfig at `goreecloud/configs/mozconfig-linux-dev`.

The earlier standalone `goreecloud/branding/locales/` files are non-runtime concept references; they do not override Gecko's built-in branding automatically.

The generated assets use original geometric placeholder artwork. They are not the approved final GoreeCloud identity, do not establish Glaze conformity, and do not cover Windows or macOS packaging. Mozilla copyright and license notices must be preserved.

On the VPS, `./mach configure` successfully loaded `--with-branding=browser/branding/goreecloud` but stopped before generating a build because `llvm-objdump` was unavailable. No Gecko runtime acceptance has been obtained.

# GoreeCloud Browser — Representative Linux CEF Runtime Evidence — 2026-10-04

**Status:** Verified Development runtime evidence; not Browser production acceptance and not Privacy Shield FR-013 acceptance  
**Browser source revision:** `16d02d80292fea225308b07ff69d947286dbb193`  
**Browser source tree:** `59a60318a3dd30c6f3d07d39d22f0d062b5fa4a4`  
**Observed on:** October 4, 2026  
**Representative target:** Zorin OS 17.3, Linux 6.8.0-138-generic, x86_64, GNOME/Zorin Wayland session

## Exact build identity

The exact current Browser source was checked out in an isolated detached worktree and built through the repository-owned `scripts/build_linux_render_beta.sh` path using the verified pinned CEF cache.

Build configuration:

- Browser: `0.1.0-beta.1`
- Channel: Beta
- CEF: `152.0.6+g708dc14+chromium-152.0.7977.83`
- Chromium: `152.0.7977.83`
- Glaze source mapping: `1.7.0` @ `1a5756daed2294155be2e9972b24f580f6222b7b`
- C++ compiler: GCC 11.4.0
- CMake: 3.31.10
- Ninja: 1.10.1
- Chromium adapter: enabled
- CEF runtime: enabled
- Linux GTK host: enabled
- libcurl download transport: enabled

Pinned CEF provenance:

- archive SHA-256: `daf8c2b6e63787d6a91d666205a8a4521419937eabaf47723738c86aea7135bd`
- `libcef.so` SHA-256: `9575a379b967d8efb42e763aaecebbb39665b3c233016b3f8a5efc47ae89ca2f`

Built artifact identities:

- `goreecloud-browser`
  - size: 1,701,224 bytes
  - SHA-256: `29016e07b63ad4e02f847e725f7f6d53f9377729c2e30970bfb78994707b0ab0`
  - ELF Build ID: `ae4138412d4b62207b62c2ac64aae746d1150116`
- `goreecloud-browser-subprocess`
  - size: 1,077,368 bytes
  - SHA-256: `36cd547c0c69361f3238e053dd9ad36d090e0231058141587129057c85c11af7`
  - ELF Build ID: `70f74e28ea2784cb1231fde0212e60461152d367`

## Build and repository test result

The render-capable build completed successfully. The repository's CTest suite executed against that exact build and reported:

- 13 tests run;
- 13 tests passed;
- 0 tests failed.

This verifies the exact source/build pair for the exercised Development smoke coverage. It does not upgrade unexercised runtime behavior.

## Representative normal-mode runtime observation

The exact artifact was launched in the owner's active GNOME/Zorin Wayland session with:

- `DISPLAY=:0`;
- `WAYLAND_DISPLAY=wayland-0`;
- `XDG_RUNTIME_DIR=/run/user/1000`;
- the build-local CEF runtime payload; and
- a build-local profile cache isolated from the user's ordinary Browser state.

Against `https://example.com/`, the runtime:

- initialized the CEF path successfully;
- initialized `BrowserApplication`;
- reported `Private startup: no`;
- created a CEF Browser in windowless mode at 1280×880;
- created Browser ID 1;
- began the main-frame HTTPS load;
- presented a nonuniform 1280×880 Browser-owned GTK software frame;
- applied the page pointer cursor; and
- completed the main-frame load with HTTP status 200.

The test was bounded by an external timeout. Production approval remained explicitly false.

## Representative private-mode runtime observation

The same exact artifact was launched again on the same target with `--isolated-private`.

The runtime:

- initialized the same exact CEF engine successfully;
- reported `Private startup: yes`;
- created the Browser in windowless mode at 1280×880;
- presented a nonuniform Browser-owned GTK software frame; and
- completed `https://example.com/` with HTTP 200.

This proves that the compiled private-startup path is executable on the representative target. It does **not** by itself prove cross-context cookie/storage isolation, lifecycle cleanup, or Close & Forget behavior.

## Cookie-persistence / private-isolation probe

A deterministic localhost endpoint was used to seed a normal-context cookie and observe request headers.

Observed behavior:

1. During the seeding normal run, the initial request carried no cookie.
2. A same-run favicon request carried the newly set cookie, proving ordinary cookie storage within that live context.
3. After the timed process ended, a second normal run did not return the cookie.
4. The persistent profile Cookie database contained no matching row after the timed lifecycle.

Source inspection confirms the normal application context is configured with persistent storage, while private contexts are configured as ephemeral. However, the runtime probes were terminated by an external timeout rather than a verified graceful Browser shutdown. Therefore these observations **do not establish a persistence defect and do not establish private-isolation acceptance**. A graceful lifecycle test is still required.

## Runtime diagnostics retained as gaps

The representative launches also exposed two environment/runtime diagnostics that must not be hidden:

- the local `chrome-sandbox` helper is not setuid; Browser explicitly refused to disable Chromium sandboxing as a workaround;
- CEF logged that the installed VAAPI version (1.14) is older than the runtime's preferred minimum (1.17);
- CEF/NSS logged `After loading Root Certs, loaded==false` with NSS error code `-8018`, even though the exercised HTTPS page completed with status 200.

These diagnostics require separate disposition before broad Browser production acceptance.

## Privacy Shield FR-013 gap mapping

This record is **not** a Privacy Shield Browser runtime-acceptance JSON record and cannot satisfy FR-013 by itself.

For the ten FR-013 dimensions:

| Dimension | Current evidence state |
| --- | --- |
| Content blocking | **Open.** No exact representative Privacy Shield content-blocking acceptance evidence was produced. |
| Tracking resistance | **Open.** Source defaults exist, but producer-authoritative Privacy Shield runtime behavior is not accepted. |
| URL cleaning | **Open.** No exact representative runtime acceptance evidence was produced. |
| Privacy status accuracy | **Partial.** Browser surfaces and fail-closed authority boundaries exist; exact producer-authoritative runtime status acceptance remains open. |
| User-visible exceptions | **Partial.** Browser permission/settings foundations exist; complete accepted Privacy Shield exception behavior remains open. |
| Private-browsing isolation | **Partial.** Exact private startup and HTTPS rendering are verified; cross-context cookie/storage isolation and graceful cleanup remain unaccepted. |
| Local substitution | **Open.** No exact representative runtime acceptance evidence was produced. |
| Failure modes | **Partial.** Browser preserved the sandbox boundary and surfaced runtime diagnostics; complete Privacy Shield failure-mode acceptance remains open. |
| Accessibility/status accuracy | **Open.** No representative accessibility-service acceptance was produced. |
| Engine boundary | **Partial.** Exact `chromium-cef` source/runtime/artifact identity and representative rendering are verified; independent FR-013 review remains open. |

## Qualification effect

This evidence materially narrows the compiled-Browser gate by establishing exact Browser source/tree, exact build artifact identity, pinned CEF/Chromium identity, successful repository tests, and representative normal/private render execution.

It does **not** create:

- Privacy Shield FR-013 acceptance;
- Browser production approval;
- Privacy Shield production approval;
- Stable or Anchor promotion; or
- authority transfer from Browser to Privacy Shield.

The next FR-013 work must target the still-open dimensions with exact artifact-bound representative evidence, beginning with graceful normal/private lifecycle isolation and the producer-authoritative Privacy Shield runtime behaviors that are not yet implemented or accepted.

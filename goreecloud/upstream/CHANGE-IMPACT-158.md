# Firefox 158 upstream change-impact register

Baseline: Firefox 158.0, published October 13, 2026. This is a review queue, **not evidence of downstream acceptance**.

| Area | Mozilla behavior to review | GoreeCloud acceptance action |
| --- | --- | --- |
| Site data retention | Enterprise `ClearOnShutdown` supersedes `SanitizeOnShutdown` when both are set | Confirm privacy policy and config migration; test preference precedence |
| WebExtensions | `publicSuffix.isKnownSuffix()` rejects invalid hostnames with an error rather than returning `false` | Test site-privacy and extension error handling |
| WebExtensions | `runtime.getVersion()` is available to extensions | Test existing extensions for API compatibility |
| CSS | Typed arithmetic support (such as division of same-type dimensional values) | Add representative web compatibility cases |
| Accessibility | `aria-busy` is supported on Firefox Android | Test Android announcements if an Android browser is built |
| Smart Window | New cross-tab information suggestions for some form-field interactions (progressive rollout) | Inspect data access, privacy defaults and feature removal choice before enabling |
| Security | Firefox 158 is covered by Mozilla Security Advisory MFSA 2026-106 | Review each vulnerability and patch completeness against exact imported source |

Authorities:
- https://www.firefox.com/en-US/firefox/158.0/releasenotes/
- https://developer.mozilla.org/en-US/docs/Mozilla/Firefox/Releases/158
- https://firefox-admin-docs.mozilla.org/release-notes/version/firefox-154...latest/
- https://www.mozilla.org/en-US/security/known-vulnerabilities/firefox/

**Release condition:** Scope and impact must be re-evaluated with each subsequent stable or emergency-dot release. The update watcher tracks versions, not every security advisory or behavioral change.

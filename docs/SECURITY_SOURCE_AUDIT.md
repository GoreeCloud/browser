# GoreeCloud Browser — Security Source Audit

**Status:** Development security-control source  
**Stable authority:** No  
**Workflow:** `.github/workflows/security-source-audit.yml`

## Scope

The Browser source-security audit is a fail-closed exact-source control for two repository risks:

1. high-confidence committed-secret signatures in the current tree and reachable Git history;
2. mutable external GitHub Actions/reusable-workflow references.

The workflow checks out the exact pull-request head or pushed `main` revision with full history, verifies the checked-out SHA, and runs the repository-owned audit script.

## Secret signatures

The current bounded scanner checks high-confidence markers for:

- private-key PEM headers;
- GitHub token prefixes;
- AWS access-key IDs;
- Google API keys;
- Slack token prefixes.

A match in reachable Git history blocks the audit even if the current tree no longer contains the value. Removing a value from the current tree is not treated as remediation of historical exposure.

This scanner is deliberately conservative and is not represented as comprehensive secret detection for every provider or custom credential format.

## CI dependency immutability

Every external `uses:` reference in repository GitHub Actions must use an exact 40-hex commit SHA. Local repository actions may use relative paths.

A mutable tag or branch such as `@v4`, `@main`, or `@latest` fails the audit.

## Explicitly separate Stable gates

A passing source-security audit does **not** establish:

- dependency or container vulnerability acceptance;
- SBOM completeness;
- artifact malware scanning;
- runtime authorization/security acceptance;
- Android/WebView/CEF security acceptance;
- production signing/key custody;
- deployment security;
- Release Candidate or Stable qualification.

Those remain separate controls under the GoreeCloud Stable Release Security Blockers standard.

package io.goreecloud.browser

import java.net.URI

/**
 * Presentation-only page-title boundary for Browser-owned Android chrome.
 *
 * Web content and the rendering engine remain untrusted presentation inputs.
 * Raw addresses, resource paths, control characters, bidirectional formatting
 * controls, and unbounded strings must not become Browser chrome identity.
 */
object PageTitlePresentation {
    const val PRODUCT_TITLE = "GoreeCloud Browser"
    private const val MAX_TITLE_CHARS = 120

    private val absoluteUrl = Regex("^[A-Za-z][A-Za-z0-9+.-]*://")
    private val windowsPath = Regex("^[A-Za-z]:[\\\\/].*")

    fun safe(rawTitle: String?, currentUrl: String): String {
        val fallback = fallbackFor(currentUrl)
        val cleaned = ChromeTextSafety.stripBidirectionalControls(rawTitle.orEmpty())
            .map { if (it.isISOControl()) ' ' else it }
            .joinToString("")
            .replace(Regex("\\s+"), " ")
            .trim()

        if (cleaned.isBlank() || looksLikeEngineOrResourceIdentity(cleaned, currentUrl)) {
            return fallback
        }

        return if (cleaned.length <= MAX_TITLE_CHARS) {
            cleaned
        } else {
            cleaned.take(MAX_TITLE_CHARS - 1).trimEnd() + "…"
        }
    }

    private fun fallbackFor(currentUrl: String): String =
        runCatching { URI(currentUrl).host }
            .getOrNull()
            ?.takeIf { it.isNotBlank() }
            ?: PRODUCT_TITLE

    private fun looksLikeEngineOrResourceIdentity(title: String, currentUrl: String): Boolean {
        if (title.equals(currentUrl, ignoreCase = true)) return true
        if (title.equals(AddressPresentation.condensed(currentUrl), ignoreCase = true)) return true

        val lower = title.lowercase()
        if (
            lower.startsWith("/") ||
            lower.startsWith("\\") ||
            windowsPath.matches(title) ||
            absoluteUrl.containsMatchIn(title) ||
            lower.startsWith("file:") ||
            lower.startsWith("content:") ||
            lower.startsWith("data:") ||
            lower.startsWith("about:") ||
            lower.startsWith("javascript:") ||
            lower.startsWith("resource:") ||
            lower.startsWith("android.resource:") ||
            lower.startsWith("res/") ||
            lower.startsWith("assets/") ||
            lower.startsWith("android_asset/")
        ) {
            return true
        }

        return false
    }
}

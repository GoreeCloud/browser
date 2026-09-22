package io.goreecloud.browser

import java.net.URI

/**
 * Presentation-only address helpers for Browser-owned chrome.
 *
 * The full URL remains the navigation authority. This object only determines
 * what an unfocused omnibox displays. It keeps the host visible from the
 * leading edge, avoids exposing opaque local/resource payloads, strips control
 * characters, and bounds display length without changing the URL that is
 * actually loaded.
 */
object AddressPresentation {
    private const val MAX_DISPLAY_LENGTH = 120

    fun condensed(url: String): String {
        val cleaned = sanitizeText(url)
        if (cleaned.isEmpty()) return ""

        // Handle known opaque/local schemes before URI parsing. Some valid WebView
        // payloads (for example data:text/html with literal markup) are not valid
        // java.net.URI strings and must still receive a safe presentation label.
        when (cleaned.substringBefore(':', missingDelimiterValue = "").lowercase()) {
            "about" -> if (cleaned.equals("about:blank", ignoreCase = true)) return "New tab"
            "data" -> return "Local page"
            "blob" -> return "Site content"
            "file", "content" -> return "Local content"
            "goreecloud" -> return "GoreeCloud"
        }

        val uri = runCatching { URI(cleaned) }.getOrNull()
            ?: return truncate(cleaned)

        val host = uri.host ?: return truncate(cleaned)
        val port = uri.port.takeIf { it >= 0 }?.let { ":$it" }.orEmpty()
        val path = uri.rawPath.orEmpty().takeUnless { it == "/" }.orEmpty()
        val query = uri.rawQuery?.let { "?$it" }.orEmpty()
        return truncate(host + port + path + query)
    }

    private fun sanitizeText(value: String): String =
        value
            .map { character -> if (character.isISOControl()) ' ' else character }
            .joinToString("")
            .trim()

    private fun truncate(value: String): String {
        if (value.length <= MAX_DISPLAY_LENGTH) return value
        return value.take(MAX_DISPLAY_LENGTH - 1) + "…"
    }
}

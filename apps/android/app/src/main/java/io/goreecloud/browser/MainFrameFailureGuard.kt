package io.goreecloud.browser

import java.net.URI

/**
 * Correlates WebView main-frame failure callbacks with the currently active
 * navigation so a late error from an older request cannot replace newer
 * content with Browser's local recovery page.
 */
internal object MainFrameFailureGuard {
    fun shouldPresent(activeUrl: String, failedUrl: String?): Boolean {
        val active = normalizedDocument(activeUrl) ?: return false
        val failed = normalizedDocument(failedUrl.orEmpty()) ?: return false
        return active == failed
    }

    private fun normalizedDocument(value: String): WebDocument? {
        if (!NavigationResolver.isAllowedWebUrl(value)) return null
        val uri = runCatching { URI(value).normalize() }.getOrNull() ?: return null
        val scheme = uri.scheme?.lowercase() ?: return null
        val host = uri.host?.lowercase()?.takeIf { it.isNotBlank() } ?: return null
        val port = when {
            uri.port >= 0 -> uri.port
            scheme == "http" -> 80
            scheme == "https" -> 443
            else -> return null
        }
        return WebDocument(
            scheme = scheme,
            host = host,
            port = port,
            path = uri.rawPath.orEmpty().ifBlank { "/" },
            query = uri.rawQuery,
        )
    }

    private data class WebDocument(
        val scheme: String,
        val host: String,
        val port: Int,
        val path: String,
        val query: String?,
    )
}

package io.goreecloud.browser

import java.net.URI
import java.util.Locale

/**
 * Produces a privacy-safe label for a blocked web-initiated navigation.
 *
 * The full target may contain phone numbers, email addresses, opaque intent
 * extras, tokens, or other sensitive values. Browser-owned recovery UI only
 * exposes the syntactic scheme when it is safe to do so.
 */
internal object BlockedNavigationPresentation {
    private val safeScheme = Regex("^[a-z][a-z0-9+.-]{0,31}$")

    fun label(rawTarget: String): String {
        val scheme = runCatching { URI(rawTarget).scheme }
            .getOrNull()
            ?.lowercase(Locale.ROOT)
            ?.takeIf(safeScheme::matches)
        return scheme?.let { "$it: link" } ?: "Unsupported link"
    }
}

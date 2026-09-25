package io.goreecloud.browser

import java.net.URI
import java.util.Locale

/**
 * Browser-owned disclosure policy for page addresses leaving GoreeCloud Browser.
 *
 * Navigation and focused omnibox editing keep their own authoritative state.
 * Clipboard/share output is available only for a validated HTTP(S) address and
 * removes explicit authority user-info before the value leaves Browser.
 */
object AddressDisclosurePolicy {
    fun forExternalUse(url: String): String? {
        if (url.isBlank() || url != url.trim() || url.any(Char::isISOControl)) return null

        val uri = runCatching { URI(url) }.getOrNull() ?: return null
        val scheme = uri.scheme?.lowercase(Locale.ROOT) ?: return null
        if (scheme != "http" && scheme != "https") return null

        val authority = uri.rawAuthority ?: return null
        val schemeSeparator = url.indexOf("://")
        if (schemeSeparator < 0) return null

        val authorityStart = schemeSeparator + 3
        val authorityEnd = firstAuthorityTerminator(url, authorityStart)
        if (url.substring(authorityStart, authorityEnd) != authority) return null

        val userInfoEnd = authority.lastIndexOf('@')
        val candidate = if (userInfoEnd >= 0) {
            val safeAuthority = authority.substring(userInfoEnd + 1)
            if (safeAuthority.isEmpty()) return null
            url.substring(0, authorityStart) + safeAuthority + url.substring(authorityEnd)
        } else {
            url
        }

        return candidate.takeIf(NavigationResolver::isAllowedWebUrl)
    }

    private fun firstAuthorityTerminator(url: String, authorityStart: Int): Int {
        val candidates = listOf(
            url.indexOf('/', authorityStart),
            url.indexOf('?', authorityStart),
            url.indexOf('#', authorityStart),
        ).filter { it >= 0 }
        return candidates.minOrNull() ?: url.length
    }
}

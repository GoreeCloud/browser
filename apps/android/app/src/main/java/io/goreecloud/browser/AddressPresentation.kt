package io.goreecloud.browser

import java.net.URI

/**
 * Presentation-only address helpers for Browser-owned chrome.
 *
 * The full URL remains the navigation authority. This object only determines
 * what an unfocused omnibox displays and deliberately keeps the canonical host
 * identity visible from the leading edge instead of scrolling to the end of a
 * long path. Internationalized hosts are shown as ASCII A-labels so Browser does
 * not render a Unicode lookalike as if it were an unrelated ASCII origin.
 */
object AddressPresentation {
    fun condensed(url: String): String {
        val uri = runCatching { URI(url) }.getOrNull() ?: return url
        val rawAuthority = uri.rawAuthority ?: return url
        val canonicalAuthority =
            InternationalizedHostPolicy.canonicalAuthority(rawAuthority) ?: return url
        val path = uri.rawPath.orEmpty().takeUnless { it == "/" }.orEmpty()
        val query = uri.rawQuery?.let { "?$it" }.orEmpty()
        val fragment = uri.rawFragment?.let { "#$it" }.orEmpty()
        return canonicalAuthority.authority + path + query + fragment
    }
}

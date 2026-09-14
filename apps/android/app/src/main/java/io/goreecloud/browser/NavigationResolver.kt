package io.goreecloud.browser

import java.net.URI
import java.net.URLEncoder
import java.nio.charset.StandardCharsets
import java.util.Locale

/**
 * GoreeCloud-owned resolution policy for the Android unified address/search bar.
 * Direct URLs remain independent from search. Non-URL input is classified as a
 * Search intent before a remote destination is produced so Privacy Shield and
 * Search capability gates have an explicit pre-delegation decision boundary.
 * Explicitly unsafe or malformed navigation-shaped input is blocked rather than
 * silently reinterpreted as a Search query.
 */
object NavigationResolver {
    const val SEARCH_HOME = "https://search.goreecloud.com/"
    private const val SEARCH_ENDPOINT = "https://search.goreecloud.com/search?q="

    sealed interface Intent {
        data object Home : Intent
        data class Navigate(val url: String) : Intent
        data class Search(val query: String, val url: String) : Intent
        data class Blocked(val input: String) : Intent
    }

    fun classify(rawInput: String): Intent {
        val input = rawInput.trim()
        if (input.isEmpty()) return Intent.Home
        if (input.any(Char::isISOControl)) return Intent.Blocked(input)

        if (hasHttpScheme(input)) {
            val normalized = normalizeWebUrl(input) ?: return Intent.Blocked(input)
            return Intent.Navigate(normalized)
        }

        if (looksLikeHost(input)) {
            val normalized = normalizeWebUrl("https://$input") ?: return Intent.Blocked(input)
            return Intent.Navigate(normalized)
        }

        if (EXPLICIT_SCHEME_REGEX.containsMatchIn(input)) {
            return Intent.Blocked(input)
        }

        return Intent.Search(
            query = input,
            url = SEARCH_ENDPOINT + encodeQuery(input),
        )
    }

    /** Compatibility helper for existing Browser callers. Blocked input resolves to no URL. */
    fun resolve(rawInput: String): String = when (val intent = classify(rawInput)) {
        Intent.Home -> SEARCH_HOME
        is Intent.Navigate -> intent.url
        is Intent.Search -> intent.url
        is Intent.Blocked -> ""
    }

    fun isAllowedWebUrl(url: String): Boolean = normalizeWebUrl(url) != null

    private fun hasHttpScheme(value: String): Boolean =
        value.startsWith("https://", ignoreCase = true) ||
            value.startsWith("http://", ignoreCase = true)

    private fun normalizeWebUrl(raw: String): String? {
        val uri = runCatching { URI(raw.trim()) }.getOrNull() ?: return null
        val scheme = uri.scheme?.lowercase(Locale.ROOT) ?: return null
        if (scheme != "https" && scheme != "http") return null
        if (uri.host.isNullOrBlank()) return null
        if (uri.userInfo != null) return null
        return uri.normalize().toASCIIString()
    }

    private fun looksLikeHost(value: String): Boolean {
        if (value.any(Char::isWhitespace)) return false
        if (value.contains("://")) return false

        val authorityCandidate = value.substringBefore('/').substringBefore('?').substringBefore('#')
        if (authorityCandidate.isEmpty() || authorityCandidate.contains('@')) return false

        if (authorityCandidate.startsWith('[') && authorityCandidate.contains(']')) return true

        val hostPart = authorityCandidate.substringBeforeLast(':', authorityCandidate)
        val portPart = authorityCandidate.substringAfterLast(':', "")
        val hasPort = hostPart != authorityCandidate && portPart.isNotEmpty() && portPart.all(Char::isDigit)
        val host = if (hasPort) hostPart else authorityCandidate

        return host.equals("localhost", ignoreCase = true) ||
            host.contains('.') ||
            IPV4_REGEX.matches(host)
    }

    private fun encodeQuery(value: String): String =
        URLEncoder.encode(value, StandardCharsets.UTF_8.name()).replace("+", "%20")

    private val IPV4_REGEX = Regex("""^(?:\d{1,3}\.){3}\d{1,3}$""")
    private val EXPLICIT_SCHEME_REGEX = Regex("""^[A-Za-z][A-Za-z0-9+.-]*:""")
}

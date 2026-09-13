package io.goreecloud.browser

import java.net.URLEncoder
import java.nio.charset.StandardCharsets

/**
 * GoreeCloud-owned resolution policy for the Android unified address/search bar.
 * Direct URLs remain independent from search. Non-URL input is sent only to
 * GoreeCloud Search; no alternate provider or silent fallback is permitted.
 */
object NavigationResolver {
    const val SEARCH_HOME = "https://search.goreecloud.com/"
    private const val SEARCH_ENDPOINT = "https://search.goreecloud.com/search?q="

    fun resolve(rawInput: String): String {
        val input = rawInput.trim()
        if (input.isEmpty()) return SEARCH_HOME

        if (hasHttpScheme(input)) {
            return InternationalizedHostPolicy.canonicalizeHttpUrl(input) ?: SEARCH_HOME
        }
        // Scheme-less input that places an `@` in the authority position can become URI user-info
        // after Browser prepends HTTPS. Fail closed before either navigation or Search forwarding so
        // credential-shaped text is not copied into history or disclosed as a query.
        if (hasSchemeLessUserInfo(input)) return SEARCH_HOME
        if (looksLikeHost(input)) {
            return InternationalizedHostPolicy.canonicalizeHttpUrl("https://$input") ?: SEARCH_HOME
        }

        return SEARCH_ENDPOINT + encodeQuery(input)
    }

    fun isAllowedWebUrl(url: String): Boolean =
        InternationalizedHostPolicy.canonicalizeHttpUrl(url) != null

    private fun hasHttpScheme(value: String): Boolean {
        return value.startsWith("https://", ignoreCase = true) ||
            value.startsWith("http://", ignoreCase = true)
    }

    private fun hasSchemeLessUserInfo(value: String): Boolean {
        if (value.any(Char::isWhitespace)) return false
        if (value.contains("://")) return false
        val authority = value.substringBefore('/').substringBefore('?').substringBefore('#')
        return '@' in authority
    }

    private fun looksLikeHost(value: String): Boolean {
        if (value.any(Char::isWhitespace)) return false
        if (value.contains("://")) return false

        val authority = value.substringBefore('/').substringBefore('?').substringBefore('#')
        if ('@' in authority) return false
        val hostPart = InternationalizedHostPolicy.canonicalAuthority(authority)?.host ?: return false

        return hostPart.equals("localhost", ignoreCase = true) ||
            hostPart.contains('.') ||
            IPV4_REGEX.matches(hostPart) ||
            (hostPart.startsWith('[') && hostPart.endsWith(']'))
    }

    private fun encodeQuery(value: String): String =
        URLEncoder.encode(value, StandardCharsets.UTF_8.name()).replace("+", "%20")

    private val IPV4_REGEX = Regex("""^(?:\d{1,3}\.){3}\d{1,3}$""")
}

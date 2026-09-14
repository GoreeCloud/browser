package io.goreecloud.browser

import java.net.URLEncoder
import java.nio.charset.StandardCharsets

/**
 * GoreeCloud-owned resolution policy for the Android unified address/search bar.
 * Direct URLs remain independent from search. Non-URL input is classified as a
 * Search intent before a remote destination is produced so Privacy Shield and
 * Search capability gates have an explicit pre-delegation decision boundary.
 */
object NavigationResolver {
    const val SEARCH_HOME = "https://search.goreecloud.com/"
    private const val SEARCH_ENDPOINT = "https://search.goreecloud.com/search?q="

    sealed interface Intent {
        data object Home : Intent
        data class Navigate(val url: String) : Intent
        data class Search(val query: String, val url: String) : Intent
    }

    fun classify(rawInput: String): Intent {
        val input = rawInput.trim()
        if (input.isEmpty()) return Intent.Home

        if (hasHttpScheme(input)) return Intent.Navigate(input)
        if (looksLikeHost(input)) return Intent.Navigate("https://$input")

        return Intent.Search(
            query = input,
            url = SEARCH_ENDPOINT + encodeQuery(input),
        )
    }

    /** Compatibility helper for existing Browser callers. */
    fun resolve(rawInput: String): String = when (val intent = classify(rawInput)) {
        Intent.Home -> SEARCH_HOME
        is Intent.Navigate -> intent.url
        is Intent.Search -> intent.url
    }

    fun isAllowedWebUrl(url: String): Boolean = hasHttpScheme(url)

    private fun hasHttpScheme(value: String): Boolean {
        return value.startsWith("https://", ignoreCase = true) ||
            value.startsWith("http://", ignoreCase = true)
    }

    private fun looksLikeHost(value: String): Boolean {
        if (value.any(Char::isWhitespace)) return false
        if (value.contains("://")) return false

        val hostPart = value.substringBefore('/').substringBefore('?').substringBefore('#')
        if (hostPart.isEmpty()) return false

        return hostPart.equals("localhost", ignoreCase = true) ||
            hostPart.contains('.') ||
            IPV4_REGEX.matches(hostPart) ||
            (hostPart.startsWith('[') && hostPart.endsWith(']'))
    }

    private fun encodeQuery(value: String): String =
        URLEncoder.encode(value, StandardCharsets.UTF_8.name()).replace("+", "%20")

    private val IPV4_REGEX = Regex("""^(?:\d{1,3}\.){3}\d{1,3}$""")
}

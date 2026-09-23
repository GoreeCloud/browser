package io.goreecloud.browser

/**
 * GoreeCloud-owned resolution policy for the Android unified address/search bar.
 * Direct URLs remain independent from search. Non-URL input is classified as a
 * Search intent before any remote request is constructed so Privacy Shield and
 * Search capability gates have an explicit pre-delegation decision boundary.
 * Explicitly unsafe or malformed navigation-shaped input is blocked rather than
 * silently reinterpreted as a Search query.
 */
object NavigationResolver {
    const val SEARCH_HOME = "https://search.goreecloud.com/"

    sealed interface Intent {
        data object Home : Intent

        data class Navigate(val url: String) : Intent {
            override fun toString(): String = "Navigate(url=<redacted>)"
        }

        data class Search(val query: String) : Intent {
            override fun toString(): String = "Search(query=<redacted>)"
        }

        data class Blocked(val input: String) : Intent {
            override fun toString(): String = "Blocked(input=<redacted>)"
        }
    }

    fun classify(rawInput: String): Intent {
        val input = rawInput.trim()
        if (input.isEmpty()) return Intent.Home
        if (input.any(Char::isISOControl)) return Intent.Blocked(input)

        if (hasHttpScheme(input)) {
            val normalized = InternationalizedHostPolicy.canonicalizeHttpUrl(input)
                ?: return Intent.Blocked(input)
            return Intent.Navigate(normalized)
        }

        if (looksLikeHost(input)) {
            val normalized = InternationalizedHostPolicy.canonicalizeHttpUrl("https://$input")
                ?: return Intent.Blocked(input)
            return Intent.Navigate(normalized)
        }

        if (EXPLICIT_SCHEME_REGEX.containsMatchIn(input)) {
            return Intent.Blocked(input)
        }

        return Intent.Search(query = input)
    }

    fun resolve(rawInput: String): String = when (val intent = classify(rawInput)) {
        Intent.Home -> SEARCH_HOME
        is Intent.Navigate -> intent.url
        is Intent.Search -> ""
        is Intent.Blocked -> ""
    }

    fun isAllowedWebUrl(url: String): Boolean =
        InternationalizedHostPolicy.canonicalizeHttpUrl(url) != null

    private fun hasHttpScheme(value: String): Boolean =
        value.startsWith("https://", ignoreCase = true) ||
            value.startsWith("http://", ignoreCase = true)

    private fun looksLikeHost(value: String): Boolean {
        if (value.any(Char::isWhitespace)) return false
        if (value.contains("://")) return false

        val authorityCandidate = value.substringBefore('/').substringBefore('?').substringBefore('#')
        if (authorityCandidate.isEmpty() || authorityCandidate.contains('@')) return false

        val canonical = InternationalizedHostPolicy.canonicalAuthority(authorityCandidate)
            ?: return false
        val host = canonical.host

        return host.equals("localhost", ignoreCase = true) ||
            host.contains('.') ||
            IPV4_REGEX.matches(host) ||
            (host.startsWith('[') && host.endsWith(']'))
    }

    private val IPV4_REGEX = Regex("""^(?:\d{1,3}\.){3}\d{1,3}$""")
    private val EXPLICIT_SCHEME_REGEX = Regex("""^[A-Za-z][A-Za-z0-9+.-]*:""")
}

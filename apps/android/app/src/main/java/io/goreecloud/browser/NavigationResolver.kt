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

        /**
         * The resolved URL remains available to the navigation runtime but is
         * excluded from debug rendering so ordinary logs cannot become a
         * browsing-history store.
         */
        data class Navigate(val url: String) : Intent {
            override fun toString(): String = "Navigate(url=<redacted>)"
        }

        /**
         * Search text remains available to the Search authorization boundary but
         * is excluded from debug rendering so ordinary logs cannot become a
         * search-history store.
         */
        data class Search(val query: String) : Intent {
            override fun toString(): String = "Search(query=<redacted>)"
        }

        /**
         * Blocked raw input may itself contain sensitive typed text. Preserve it
         * for local policy handling while redacting it from diagnostics.
         */
        data class Blocked(val input: String) : Intent {
            override fun toString(): String = "Blocked(input=<redacted>)"
        }
    }

    fun classify(rawInput: String): Intent {
        // Reject controls before trimming: otherwise leading/trailing control
        // characters disappear before navigation/Search classification.
        if (SearchBoundaryTextSafety.containsUnsupportedControl(rawInput)) return Intent.Blocked(rawInput)
        val input = rawInput.trim()
        if (input.isEmpty()) return Intent.Home

        if (hasHttpScheme(input)) {
            val normalized = canonicalizeWebUrl(input) ?: return Intent.Blocked(input)
            return Intent.Navigate(normalized)
        }

        if (looksLikeHost(input)) {
            val normalized = canonicalizeWebUrl("https://$input") ?: return Intent.Blocked(input)
            return Intent.Navigate(normalized)
        }

        if (EXPLICIT_SCHEME_REGEX.containsMatchIn(input)) {
            return Intent.Blocked(input)
        }

        // Query text remains transport-neutral here. A separate accepted Search
        // contract must authorize and construct any remote POST request.
        return Intent.Search(query = input)
    }

    /**
     * Compatibility helper for URL-only callers. Search and blocked input do not
     * resolve to a URL because free-text queries must never be embedded in one.
     */
    fun resolve(rawInput: String): String = when (val intent = classify(rawInput)) {
        Intent.Home -> SEARCH_HOME
        is Intent.Navigate -> intent.url
        is Intent.Search -> ""
        is Intent.Blocked -> ""
    }

    /**
     * Returns the canonical HTTP(S) URL accepted by Browser navigation policy,
     * or null when the input is not safe to disclose or navigate.
     */
    fun canonicalizeWebUrl(url: String): String? =
        InternationalizedHostPolicy.canonicalizeHttpUrl(url)

    fun isAllowedWebUrl(url: String): Boolean = canonicalizeWebUrl(url) != null

    private fun hasHttpScheme(value: String): Boolean =
        value.startsWith("https://", ignoreCase = true) ||
            value.startsWith("http://", ignoreCase = true)

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

    private val IPV4_REGEX = Regex("""^(?:\d{1,3}\.){3}\d{1,3}$""")
    private val EXPLICIT_SCHEME_REGEX = Regex("""^[A-Za-z][A-Za-z0-9+.-]*:""")
}

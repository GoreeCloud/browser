package io.goreecloud.browser

/**
 * Presentation-only state for Android WebView's local find-in-page facility.
 *
 * Search text never leaves the active WebView through this model.
 */
object FindInPageStatus {
    const val MAX_QUERY_LENGTH = 512

    fun label(
        query: String,
        activeMatchOrdinal: Int,
        numberOfMatches: Int,
        isDoneCounting: Boolean,
    ): String {
        if (query.isEmpty()) return "Enter text to find on this page."
        if (!isDoneCounting) return "Searching…"
        if (numberOfMatches <= 0) return "No matches"

        val active = activeMatchOrdinal.coerceIn(0, numberOfMatches - 1)
        return "${active + 1} of $numberOfMatches"
    }
}

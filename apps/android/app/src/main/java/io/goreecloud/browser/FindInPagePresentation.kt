package io.goreecloud.browser

object FindInPagePresentation {
    fun status(
        activeMatchOrdinal: Int,
        numberOfMatches: Int,
        doneCounting: Boolean,
    ): String = when {
        !doneCounting -> "Searching…"
        numberOfMatches <= 0 -> "No matches"
        else -> {
            val displayIndex = activeMatchOrdinal
                .coerceIn(0, numberOfMatches - 1) + 1
            "$displayIndex of $numberOfMatches"
        }
    }
}

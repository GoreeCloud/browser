package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Test

class FindInPagePresentationTest {
    @Test
    fun searchingStateWinsUntilCountingCompletes() {
        assertEquals(
            "Searching…",
            FindInPagePresentation.status(
                activeMatchOrdinal = 0,
                numberOfMatches = 0,
                doneCounting = false,
            ),
        )
    }

    @Test
    fun emptyResultIsExplicit() {
        assertEquals(
            "No matches",
            FindInPagePresentation.status(
                activeMatchOrdinal = 0,
                numberOfMatches = 0,
                doneCounting = true,
            ),
        )
    }

    @Test
    fun matchOrdinalIsPresentedOneBasedAndBounded() {
        assertEquals(
            "2 of 4",
            FindInPagePresentation.status(
                activeMatchOrdinal = 1,
                numberOfMatches = 4,
                doneCounting = true,
            ),
        )
        assertEquals(
            "4 of 4",
            FindInPagePresentation.status(
                activeMatchOrdinal = 99,
                numberOfMatches = 4,
                doneCounting = true,
            ),
        )
    }
}

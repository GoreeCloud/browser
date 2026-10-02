package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Test

class FindInPageStatusTest {
    @Test
    fun statusReflectsEmptySearchingMissingAndMatchedStates() {
        assertEquals(
            "Enter text to find on this page.",
            FindInPageStatus.label("", 0, 0, true),
        )
        assertEquals(
            "Searching…",
            FindInPageStatus.label("privacy", 0, 0, false),
        )
        assertEquals(
            "No matches",
            FindInPageStatus.label("privacy", 0, 0, true),
        )
        assertEquals(
            "2 of 4",
            FindInPageStatus.label("privacy", 1, 4, true),
        )
        assertEquals(
            "4 of 4",
            FindInPageStatus.label("privacy", 99, 4, true),
        )
    }
}

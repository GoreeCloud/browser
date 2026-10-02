package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Test

class PageTextZoomTest {
    @Test
    fun zoomIsBoundedAndMovesInTwentyFivePercentSteps() {
        assertEquals(75, PageTextZoom.normalize(10))
        assertEquals(200, PageTextZoom.normalize(900))
        assertEquals(75, PageTextZoom.decrease(75))
        assertEquals(100, PageTextZoom.increase(75))
        assertEquals(200, PageTextZoom.increase(200))
        assertEquals("125%", PageTextZoom.label(125))
    }
}

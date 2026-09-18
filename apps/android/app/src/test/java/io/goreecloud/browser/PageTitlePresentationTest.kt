package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class PageTitlePresentationTest {
    @Test
    fun normalPageTitleIsPreserved() {
        assertEquals(
            "GoreeCloud News — Today",
            PageTitlePresentation.safe("GoreeCloud News — Today", "https://news.goreecloud.com/today"),
        )
    }

    @Test
    fun rawUrlTitleFallsBackToHost() {
        assertEquals(
            "example.com",
            PageTitlePresentation.safe("https://example.com/raw/resource", "https://example.com/page"),
        )
    }

    @Test
    fun resourcePathTitleFallsBackToHost() {
        assertEquals(
            "example.com",
            PageTitlePresentation.safe("android_asset/error/page.html", "https://example.com/page"),
        )
    }

    @Test
    fun enginePseudoUrlFallsBackToHost() {
        assertEquals(
            "example.com",
            PageTitlePresentation.safe("about:blank", "https://example.com/page"),
        )
    }

    @Test
    fun controlCharactersAreCollapsedBeforePresentation() {
        assertEquals(
            "Example title with controls",
            PageTitlePresentation.safe("Example\u0000title\nwith\tcontrols", "https://example.com/page"),
        )
    }

    @Test
    fun longTitlesAreBounded() {
        val presented = PageTitlePresentation.safe("A".repeat(200), "https://example.com/page")
        assertEquals(120, presented.length)
        assertFalse(presented.contains("https://"))
    }
}

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
    fun bidiFormattingControlsCannotReorderBrowserChrome() {
        val presented = PageTitlePresentation.safe(
            "Invoice\u202Efdp.exe\u2066 verified",
            "https://example.com/page",
        )

        assertEquals("Invoicefdp.exe verified", presented)
        assertFalse(presented.contains('\u202E'))
        assertFalse(presented.contains('\u2066'))
    }

    @Test
    fun longTitlesAreBounded() {
        val presented = PageTitlePresentation.safe("A".repeat(200), "https://example.com/page")
        assertEquals(120, presented.length)
        assertFalse(presented.contains("https://"))
    }
}

package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserLocalPagesTest {
    @Test
    fun pageUnavailableSurfaceIsGenericAndDoesNotExposeEngineErrors() {
        val html = BrowserLocalPages.pageUnavailableHtml("body{font-family:sans-serif}")

        assertTrue(html.contains("<title>Page unavailable</title>"))
        assertTrue(html.contains("use Reload to try again"))
        assertTrue(html.contains("No remote engine or network error details are shown"))
        assertFalse(html.contains("ERR_"))
        assertFalse(html.contains("net::"))
        assertFalse(html.contains("WebView"))
        assertFalse(html.contains("Chromium"))
    }
}

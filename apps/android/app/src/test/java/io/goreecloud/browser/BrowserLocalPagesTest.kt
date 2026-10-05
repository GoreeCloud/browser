package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserLocalPagesTest {
    @Test
    fun localDocumentHeadFailsClosedAgainstActiveOrRemoteContent() {
        val head = BrowserLocalPages.secureHead(
            "body{font-family:sans-serif}",
            "Local page",
        )

        assertTrue(head.contains("Content-Security-Policy"))
        assertTrue(head.contains("default-src 'none'"))
        assertTrue(head.contains("style-src 'unsafe-inline'"))
        assertTrue(head.contains("base-uri 'none'"))
        assertTrue(head.contains("form-action 'none'"))
        assertTrue(head.contains("object-src 'none'"))
        assertTrue(head.contains("name=\"referrer\" content=\"no-referrer\""))
        assertTrue(head.contains("<title>Local page</title>"))
    }

    @Test
    fun pageUnavailableSurfaceIsGenericAndDoesNotExposeEngineErrors() {
        val html = BrowserLocalPages.pageUnavailableHtml("body{font-family:sans-serif}")

        assertTrue(html.contains("<title>Page unavailable</title>"))
        assertTrue(html.contains("default-src 'none'"))
        assertTrue(html.contains("name=\"referrer\" content=\"no-referrer\""))
        assertTrue(html.contains("use Reload to try again"))
        assertTrue(html.contains("No remote engine or network error details are shown"))
        assertFalse(html.contains("ERR_"))
        assertFalse(html.contains("net::"))
        assertFalse(html.contains("WebView"))
        assertFalse(html.contains("Chromium"))
    }

    @Test
    fun blockedNavigationSurfaceExplainsFailureWithoutTargetPayload() {
        val html = BrowserLocalPages.blockedWebNavigationHtml(
            "body{font-family:sans-serif}",
            "mailto: link",
        )

        assertTrue(html.contains("<title>Navigation blocked</title>"))
        assertTrue(html.contains("default-src 'none'"))
        assertTrue(html.contains("name=\"referrer\" content=\"no-referrer\""))
        assertTrue(html.contains("mailto: link"))
        assertTrue(html.contains("Use Back to return"))
        assertFalse(html.contains("private.user@example.com"))
        assertFalse(html.contains("intent://"))
    }
}
